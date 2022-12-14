/*
 * Copyright (C) 2013 Felix Fietkau <nbd@openwrt.org>
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

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/reboot.h>

#include <libubox/uloop.h>
#include <libubus.h>

#include <limits.h>
#include <stdlib.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <regex.h>
#include <unistd.h>
#include <stdio.h>

#include "../utils/utils.h"
#include "init.h"
#include "../watchdog.h"

unsigned int debug = 0;

static void
signal_shutdown(int signal, siginfo_t *siginfo, void *data)
{
	fprintf(stderr, "reboot\n");
	fflush(stderr);
	sync();
	sleep(2);
	reboot(RB_AUTOBOOT);
	while (1)
		;
}

static struct sigaction sa_shutdown = {
	.sa_sigaction = signal_shutdown,
	.sa_flags = SA_SIGINFO
};

static void
cmdline(void)
{
	char line[20];
	char* res;
	long	r;

	res = get_cmdline_val("init_debug", line, sizeof(line));
	if (res != NULL) {
		r = strtol(line, NULL, 10);
		if ((r != LONG_MIN) && (r != LONG_MAX))
			debug = (int) r;
	}
}

int
main(int argc, char **argv)
{
	pid_t pid;

	if (argv[1] && strcmp(argv[1], "-q") == 0) {
		return kill(1, SIGHUP);
	}

	ulog_open(ULOG_KMSG, LOG_DAEMON, "init");

	sigaction(SIGTERM, &sa_shutdown, NULL);
	sigaction(SIGUSR1, &sa_shutdown, NULL);
	sigaction(SIGUSR2, &sa_shutdown, NULL);

	early();
	cmdline();
	watchdog_init(1);

	pid = fork();
	if (!pid) {
		char *kmod[] = { "/sbin/kmodloader", "/etc/modules-boot.d/", NULL };

		if (debug < 3)
			patch_stdio("/dev/null");

		execvp(kmod[0], kmod);
		ERROR("Failed to start kmodloader\n");
		exit(-1);
	}
	if (pid <= 0) {
		ERROR("Failed to start kmodloader instance\n");
	} else {
		const struct timespec req = {0, 10 * 1000 * 1000};
		int i;

		for (i = 0; i < 1200; i++) {
			if (waitpid(pid, NULL, WNOHANG) > 0)
				break;
			nanosleep(&req, NULL);
			watchdog_ping();
		}
	}
	uloop_init();
	preinit();
	uloop_run();

	return 0;
}
