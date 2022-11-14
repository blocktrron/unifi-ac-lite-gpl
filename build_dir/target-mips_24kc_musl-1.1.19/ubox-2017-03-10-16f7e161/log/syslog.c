/*
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/un.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <regex.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <ctype.h>

#include <libubox/uloop.h>
#include <libubox/usock.h>
#include <libubox/ustream.h>

#include "syslog.h"

#define LOG_DEFAULT_SIZE	(16 * 1024)
#define LOG_DEFAULT_SOCKET	"/dev/log"
#define SYSLOG_PADDING		16

#define KLOG_DEFAULT_PROC	"/proc/kmsg"

#define PAD(x) (x % 4) ? (((x) - (x % 4)) + 4) : (x)

static char *log_dev = LOG_DEFAULT_SOCKET;
static int log_size = LOG_DEFAULT_SIZE;
static struct log_head *log, *log_end, *oldest, *newest;
static int current_id = 0;
static regex_t pat_prio;
static regex_t pat_tstamp;

static struct log_head*
log_next(struct log_head *h, int size)
{
	struct log_head *n = (struct log_head *) &h->data[PAD(size)];

	return (n >= log_end) ? (log) : (n);
}

void
log_add(char *buf, int size, int source)
{
	regmatch_t matches[4];
	struct log_head *next;
	int priority = 0;
	int ret;
	char *c;

	/* bounce out if we don't have init'ed yet (regmatch etc will blow) */
	if (!log) {
		fprintf(stderr, "%s", buf);
		return;
	}

	for (c = buf; *c; c++) {
		if (*c == '\n')
			*c = ' ';
	}

	c = buf + size - 2;
	while (isspace(*c)) {
		size--;
		c--;
	}

	buf[size - 1] = 0;

	/* strip the priority */
	ret = regexec(&pat_prio, buf, 3, matches, 0);
	if (!ret) {
		priority = atoi(&buf[matches[1].rm_so]);
		size -= matches[2].rm_so;
		buf += matches[2].rm_so;
	}

#if 0
	/* strip kernel timestamp */
	ret = regexec(&pat_tstamp,buf, 4, matches, 0);
	if ((source == SOURCE_KLOG) && !ret) {
		size -= matches[3].rm_so;
		buf += matches[3].rm_so;
	}
#endif

	/* strip syslog timestamp */
	if ((source == SOURCE_SYSLOG) && (size > SYSLOG_PADDING) && (buf[SYSLOG_PADDING - 1] == ' ')) {
		size -= SYSLOG_PADDING;
		buf += SYSLOG_PADDING;
	}

	//fprintf(stderr, "-> %d - %s\n", priority, buf);

	/* find new oldest entry */
	next = log_next(newest, size);
	if (next > newest) {
		while ((oldest > newest) && (oldest <= next) && (oldest != log))
			oldest = log_next(oldest, oldest->size);
	} else {
		//fprintf(stderr, "Log wrap\n");
		newest->size = 0;
		next = log_next(log, size);
		for (oldest = log; oldest <= next; oldest = log_next(oldest, oldest->size))
			;
		newest = log;
	}

	/* add the log message */
	newest->size = size;
	newest->id = current_id++;
	newest->priority = priority;
	newest->source = source;
	clock_gettime(CLOCK_REALTIME, &newest->ts);
	strcpy(newest->data, buf);

	ubus_notify_log(newest);

	newest = next;
}

static void
syslog_handle_fd(struct uloop_fd *fd, unsigned int events)
{
	static char buf[LOG_LINE_SIZE];
	int len;

	while (1) {
		len = recv(fd->fd, buf, LOG_LINE_SIZE - 1, 0);
		if (len < 0) {
			if (errno == EINTR)
				continue;

			break;
		}
		if (!len)
			break;

		buf[len] = 0;

		log_add(buf, strlen(buf) + 1, SOURCE_SYSLOG);
	}
}

static void
klog_cb(struct ustream *s, int bytes)
{
	char *newline, *str;
	int len;
	int buflen;

	do {
		str = ustream_get_read_buf(s, &buflen);
		if (!str)
			break;
		newline = strchr(str, '\n');
		if (!newline)
		{
			len = strlen(str);
			if (!len) {
				ustream_consume(s, 1);
				continue;
			}
			if (len == buflen)
				break; // more bytes to come
			len += 1;
			log_add(str, len, SOURCE_KLOG);
			if (len > buflen)
				len = buflen;
			ustream_consume(s, len);
			continue;
		}
		*newline = 0;
		len = newline + 1 - str;
		log_add(str, len, SOURCE_KLOG);
		ustream_consume(s, len);
	} while (1);
}

static struct uloop_fd syslog_fd = {
	.cb = syslog_handle_fd
};

static struct ustream_fd klog = {
	.stream.string_data = true,
	.stream.notify_read = klog_cb,
};

static int
klog_open(void)
{
	int fd;

	fd = open(KLOG_DEFAULT_PROC, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s\n", KLOG_DEFAULT_PROC);
		return -1;
	}
	fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
	ustream_fd_init(&klog, fd);
	return 0;
}

static int
syslog_open(void)
{
	unlink(log_dev);
	syslog_fd.fd = usock(USOCK_UNIX | USOCK_UDP | USOCK_SERVER | USOCK_NONBLOCK, log_dev, NULL);
	if (syslog_fd.fd < 0) {
		fprintf(stderr,"Failed to open %s\n", log_dev);
		return -1;
	}
	chmod(log_dev, 0666);
	uloop_fd_add(&syslog_fd, ULOOP_READ | ULOOP_EDGE_TRIGGER);

	return 0;
}

struct log_head*
log_list(int count, struct log_head *h)
{
	unsigned int min = count;

	if (count)
		min = (count < current_id) ? (current_id - count) : (0);
	if (!h && oldest->id >= min)
		return oldest;
	if (!h)
		h = oldest;

	while (h != newest) {
		h = log_next(h, h->size);
		if (!h->size && (h > newest))
			h = log;
		if (h->id >= min && (h != newest))
			return h;
	}

	return NULL;
}

int
log_buffer_init(int size)
{
	struct log_head *_log = calloc(1, size);

	if (!_log) {
		fprintf(stderr, "Failed to initialize log buffer with size %d\n", log_size);
		return -1;
	}

	if (log && ((log_size + sizeof(struct log_head)) < size)) {
		struct log_head *start = _log;
		struct log_head *end = ((void*) _log) + size;
		struct log_head *l;

		l = log_list(0, NULL);
		while ((start < end) && l && l->size) {
			memcpy(start, l, PAD(sizeof(struct log_head) + l->size));
			start = (struct log_head *) &l->data[PAD(l->size)];
			l = log_list(0, l);
		}
		free(log);
		newest = start;
		newest->size = 0;
		oldest = log = _log;
		log_end = ((void*) log) + size;
	} else {
		oldest = newest = log = _log;
		log_end = ((void*) log) + size;
	}
	log_size = size;

	return 0;
}

void
log_init(int _log_size)
{
	if (_log_size > 0)
		log_size = _log_size;

	regcomp(&pat_prio, "^<([0-9]*)>(.*)", REG_EXTENDED);
	regcomp(&pat_tstamp, "^\[[ 0]*([0-9]*).([0-9]*)] (.*)", REG_EXTENDED);

	if (log_buffer_init(log_size)) {
		fprintf(stderr, "Failed to allocate log memory\n");
		exit(-1);
	}

	syslog_open();
	klog_open();
	openlog("sysinit", LOG_CONS, LOG_DAEMON);
}

void
log_shutdown(void)
{
	if (syslog_fd.registered) {
		uloop_fd_delete(&syslog_fd);
		close(syslog_fd.fd);
	}

	ustream_free(&klog.stream);
	close(klog.fd.fd);
	free(log);
	regfree(&pat_prio);
	regfree(&pat_tstamp);
}
