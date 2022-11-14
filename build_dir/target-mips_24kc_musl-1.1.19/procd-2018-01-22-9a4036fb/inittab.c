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

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <ctype.h>

#include <libubox/utils.h>
#include <libubox/list.h>
#include <libubox/ustream.h>

#include "utils/utils.h"
#include "procd.h"
#include "rcS.h"

#ifndef O_PATH
#define O_PATH		010000000
#endif

#define TAG_ID		0
#define TAG_RUNLVL	1
#define TAG_ACTION	2
#define TAG_PROCESS	3

#define MAX_ARGS	32
#define LINE_LEN	256

#define PROCESS_TERMINATION_TIMEOUT_MS	5000

struct init_action;
char *console = NULL;

struct init_handler {
	const char *name;
	void (*cb) (struct init_action *a);
	int multi;
};

struct init_action {
	struct list_head list;

	char *id;
	char *argv[MAX_ARGS];
	char *line;

	struct init_handler *handler;
	struct uloop_process proc;

	int respawn;
	int crash_count;
	int crash_report_count;

	struct uloop_timeout tout;
	struct timespec start;

	bool dispose;
	int unique_id;

	struct ustream_fd _stdout;
	struct ustream_fd _stderr;
};

static const char *tab = "/etc/inittab";
static char *ask = "/sbin/askfirst";

static LIST_HEAD(actions);

/* Forward declarations */
static void free_action(struct init_action *action);

static void
free_ustream(struct init_action *action)
{
	/* Close _stdout and _stderr streams */
	if (action->_stdout.fd.fd > -1) {
		ustream_free(&action->_stdout.stream);
		close(action->_stdout.fd.fd);
		action->_stdout.fd.fd = -2;
	}
	if (action->_stderr.fd.fd > -1) {
		ustream_free(&action->_stderr.stream);
		close(action->_stderr.fd.fd);
		action->_stderr.fd.fd = -1;
	}
}

static int dev_exist(const char *dev)
{
	int dfd, fd;

	dfd = open("/dev", O_PATH|O_DIRECTORY);

	if (dfd < 0)
		return 0;

	fd = openat(dfd, dev, O_RDONLY);
	close(dfd);

	if (fd < 0)
		return 0;

	close(fd);
	return 1;
}

static void get_action_command_string(char *buffer, int buffer_size, struct init_action *a)
{
	int i, len;
	buffer[0] = '\0';
	for (i = 0; i < MAX_ARGS && a->argv[i]; i++) {
		if (i) {
			len = strlen(buffer);
			strncat(buffer, " ", buffer_size - len - 1);
		}
		len = strlen(buffer);
		strncat(buffer, a->argv[i], buffer_size - len - 1);
	}
}

static bool is_action_line_equal(struct init_action *a, struct init_action *b)
{
	int arga, argb;

	if (strcmp(a->id, b->id))
		return false;
	if (strcmp(a->handler->name, b->handler->name))
		return false;

	arga = argb = 0;
	// ask actions modify the argv to put ask at the beginning
	if (a->argv[0] == ask)
		arga = 1;
	if (b->argv[0] == ask)
		argb = 1;
	for ( ; a->argv[arga] || b->argv[argb]; arga++, argb++) {
		if (!a->argv[arga] || !b->argv[argb])
			return false;
		if (strcmp(a->argv[arga], b->argv[argb]))
			return false;
	}
	return true;
}

static void calculate_action_unique_id(struct init_action *a, struct list_head *list)
{
	int unique_id = 0;
	struct init_action *b;
	list_for_each_entry(b, list, list) {
		if (a == b)
			break;
		if (is_action_line_equal(a, b)) {
			// char line[LINE_LEN];
			// get_action_command_string(line, sizeof(line), a);
			// DEBUG(4, "Action equal '%s' #%d\n", line, b->unique_id);
			unique_id++;
		}
	}
	a->unique_id = unique_id;
}

static void closefd(int fd)
{
	if (fd > STDERR_FILENO)
		close(fd);
}

static void fork_worker(struct init_action *a)
{
	int opipe[2] = { -1, -1 };
	int epipe[2] = { -1, -1 };

	// UI: Relay stdout if the process has no id specified or it's "null"
	if (!a->id || !strcmp(a->id, "null")) {
		if (a->_stdout.fd.fd > -2) {
			if (pipe(opipe)) {
				ULOG_WARN("pipe() failed: %d (%s)\n", errno, strerror(errno));
				opipe[0] = opipe[1] = -1;
			}
		}
		if (a->_stderr.fd.fd > -2) {
			if (pipe(epipe)) {
				ULOG_WARN("pipe() failed: %d (%s)\n", errno, strerror(errno));
				epipe[0] = epipe[1] = -1;
			}
		}
	}
	a->proc.pid = fork();
	if (!a->proc.pid) {
		pid_t p = setsid();
		// UI: If process has not "null" id, fallback to original method
		if (a->id && strcmp(a->id, "null")) {
			if (patch_stdio(a->id))
				ERROR("Failed to setup i/o redirection\n");
		} else {
			int stdin = open("/dev/null", O_RDONLY);
			int _stdout = opipe[1];
			int _stderr = epipe[1];

			closefd(opipe[0]);
			closefd(epipe[0]);

			if (_stdout == -1)
				_stdout = open("/dev/null", O_WRONLY);
			if (_stderr == -1)
				_stderr = open("/dev/null", O_WRONLY);
			if (stdin > -1) {
				dup2(stdin, STDIN_FILENO);
				closefd(stdin);
			}
			if (_stdout > -1) {
				dup2(_stdout, STDOUT_FILENO);
				closefd(_stdout);
			}
			if (_stderr > -1) {
				dup2(_stderr, STDERR_FILENO);
				closefd(_stderr);
			}
		}
		ioctl(STDIN_FILENO, TIOCSCTTY, 1);
		tcsetpgrp(STDIN_FILENO, p);

		execvp(a->argv[0], a->argv);
		ERROR("Failed to execute %s\n", a->argv[0]);
		exit(-1);
	}

	if (a->proc.pid > 0) {
		char command[LINE_LEN];
		get_action_command_string(command, sizeof(command), a);
		DEBUG(4, "Launched new %s pid: %d '%s' #%d\n",
					a->handler->name, (int) a->proc.pid,
					command, a->unique_id);
		uloop_process_add(&a->proc);
		if (opipe[0] > -1) {
			ustream_fd_init(&a->_stdout, opipe[0]);
			closefd(opipe[1]);
		}
		if (epipe[0] > -1) {
			ustream_fd_init(&a->_stderr, epipe[0]);
			closefd(epipe[1]);
		}
		clock_gettime(CLOCK_MONOTONIC, &a->start);
	}
}

static void child_exit(struct uloop_process *proc, int ret)
{
	struct init_action *a = container_of(proc, struct init_action, proc);
	char command[LINE_LEN];
	struct timespec now;
	long uptime;

	get_action_command_string(command, sizeof(command), a);
	clock_gettime(CLOCK_MONOTONIC, &now);
	uptime = now.tv_sec - a->start.tv_sec;

	if (a->dispose) {
		DEBUG(4, "Process '%s' (pid %d, uid %d, pending %d, uptime %ld) terminated with status %d.",
			command, proc->pid, a->unique_id, proc->pending, uptime, ret);
		free_action(a);
	} else {
		const char *path_in_argv = strrchr(a->argv[0], '/');
		const char *process_name = path_in_argv ? path_in_argv + 1 : a->argv[0];
		const char *coredump_cmd_str = "cat /dev/null | /sbin/coredump %s %d %d %d %ld";
		const int coredump_cmd_len = strlen(coredump_cmd_str) - 11 + strlen(process_name) + 48;
		int send_event = 0, send_app_crash_report = 0;
		char sys_cmd[coredump_cmd_len];

		/*
		   App exit is considered a crash when:
		   - UAP/USW is not restarting
		   - child process was not stopped or continued
		   - app was killed by signal different than SIGTERM (i.e. oom SIGKILL or SIGSEGV) or
		     it was not terminated by signal (app closed itself), but exited with non zero exit code
		*/

		const int is_crash = !procd_is_shutdown_state() &&
				     !WIFSTOPPED(ret) && !WIFCONTINUED(ret) &&
				     ((WIFSIGNALED(ret) && WTERMSIG(ret) != SIGTERM) ||
				      (!WIFSIGNALED(ret) && WEXITSTATUS(ret)));

		free_ustream(a);

		if (is_crash) {
			/* Increment crash count for process */
			a->crash_count += 1;

			/* Enable stdout relaying to syslog (stderr is already enabled) */
			a->_stdout.fd.fd = -1;

			if (a->crash_report_count * a->crash_report_count <= a->crash_count)
				send_app_crash_report = 1;

			/* We can't send event when mcad is not working */
			if (strstr(command, "mcad"))
				send_event = 0;

			/* There is no need to send crash report if coredump file was generated (and already sent to analytics) */
			if (WCOREDUMP(ret))
				send_app_crash_report = 0;

			if (send_app_crash_report) {
				snprintf(sys_cmd, sizeof(sys_cmd), coredump_cmd_str, process_name, proc->pid, WTERMSIG(ret), a->crash_count, uptime);
				if (system(sys_cmd) < 0)
					ERROR("Failed to execute %s: %m\n", sys_cmd, errno);
				else
					a->crash_report_count++;
			}
		}

		DEBUG(0, "Process '%s' exited with status %d - scheduling for restart (PID: %d, UID: %d, uptime: %ld, signal: %d, pending: %d, crashes: %d, event: %s, reported: %s).",
		      command, ret, proc->pid, a->unique_id, uptime, WTERMSIG(ret), proc->pending, a->crash_count, send_event ? "yes" : "no", send_app_crash_report ? "yes" : "no");

		uloop_timeout_set(&a->tout, a->respawn);
	}
}

static void sigkill_action_after_timeout(struct uloop_timeout *tout)
{
	struct init_action *action = container_of(tout, struct init_action, tout);

	/* if action still exists, process has to be SIGKILLED */
	if (action) {
		const int pid = action->proc.pid;
		if (pid > 0) {
			char command[LINE_LEN];
			get_action_command_string(command, sizeof(command), action);
			ERROR("Process didn't stop on SIGTERM - sending SIGKILL to pid: %d '%s' #%d\n",
				pid, command, action->unique_id);
			kill(pid, SIGKILL);
			/* action resources are freed in child_exit() after terminaton */
		} else {
			/* should never happen, if so free action resources */
			free_action(action);
		}
	}
}

static void respawn(struct uloop_timeout *tout)
{
	struct init_action *a = container_of(tout, struct init_action, tout);
	fork_worker(a);
}

static void
action_stdio(struct ustream *s, int prio, struct init_action *action)
{
	char *newline, *str, *arg0, ident[32];
	int len;

	arg0 = action->argv[0];
	snprintf(ident, sizeof(ident), "%s[%d]", arg0, action->proc.pid);
	ulog_open(ULOG_SYSLOG, LOG_DAEMON, ident);

	do {
		str = ustream_get_read_buf(s, NULL);
		if (!str)
			break;

		newline = strchr(str, '\n');
		if (!newline)
			break;

		*newline = 0;
		len = newline + 1 - str;

		ulog(prio, "%s\n", str);
		ustream_consume(s, len);
	} while (1);

	ulog_open(ULOG_SYSLOG, LOG_DAEMON, "procd");
}

static void
action_stdout(struct ustream *s, int bytes)
{
	action_stdio(s, LOG_INFO, container_of(s, struct init_action, _stdout.stream));
}

static void
action_stderr(struct ustream *s, int bytes)
{
	action_stdio(s, LOG_NOTICE, container_of(s, struct init_action, _stderr.stream));
}

static void rcdone(struct runqueue *q)
{
	procd_state_next();
}

static void runrc(struct init_action *a)
{
	if (!a->argv[1] || !a->argv[2]) {
		ERROR("valid format is rcS <S|K> <param>\n");
		return;
	}

	/* proceed even if no init or shutdown scripts run */
	if (rcS(a->argv[1], a->argv[2], rcdone))
		rcdone(NULL);
}

static void askfirst(struct init_action *a)
{
	int i;

	if (!dev_exist(a->id) || (console && !strcmp(console, a->id))) {
		DEBUG(4, "Skipping %s\n", a->id);
		return;
	}

	a->tout.cb = respawn;
	for (i = MAX_ARGS - 1; i >= 1; i--)
		a->argv[i] = a->argv[i - 1];
	a->argv[0] = ask;
	a->respawn = 500;

	a->proc.cb = child_exit;
	fork_worker(a);
}

static void askconsole(struct init_action *a)
{
	char line[256], *tty, *split;
	int i;

	tty = get_cmdline_val("console", line, sizeof(line));
	if (tty != NULL) {
		split = strchr(tty, ',');
		if (split != NULL)
			*split = '\0';

		if (!dev_exist(tty)) {
			DEBUG(4, "skipping %s\n", tty);
			return;
		}

		console = strdup(tty);
		a->id = strdup(tty);
	}
	else {
		console = NULL;
		a->id = NULL;
	}

	a->tout.cb = respawn;
	for (i = MAX_ARGS - 1; i >= 1; i--)
		a->argv[i] = a->argv[i - 1];
	a->argv[0] = ask;
	a->respawn = 500;

	a->proc.cb = child_exit;
	fork_worker(a);
}

static void rcrespawn(struct init_action *a)
{
	a->tout.cb = respawn;
	a->respawn = 500;

	// UI: fd == -2 -> disable relaying, -1 enables it
	a->_stdout.fd.fd = -2;
	a->_stdout.stream.string_data = true;
	a->_stdout.stream.notify_read = action_stdout;

	// UI: Always relay stderr to syslog
	a->_stderr.fd.fd = -1;
	a->_stderr.stream.string_data = true;
	a->_stderr.stream.notify_read = action_stderr;

	a->proc.cb = child_exit;
	fork_worker(a);
}

static struct init_handler handlers[] = {
	{
		.name = "sysinit",
		.cb = runrc,
	}, {
		.name = "shutdown",
		.cb = runrc,
	}, {
		.name = "askfirst",
		.cb = askfirst,
		.multi = 1,
	}, {
		.name = "askconsole",
		.cb = askconsole,
		.multi = 1,
	}, {
		.name = "respawn",
		.cb = rcrespawn,
		.multi = 1,
	}, {
		.name = "askconsolelate",
		.cb = askconsole,
		.multi = 1,
	}, {
		.name = "respawnlate",
		.cb = rcrespawn,
		.multi = 1,
	}
};

static int add_action(struct init_action *a, const char *name, struct list_head *to_list)
{
	int i;

	DEBUG(4, "Add action to the list - action %s\n", name);

	for (i = 0; i < ARRAY_SIZE(handlers); i++)
		if (!strcmp(handlers[i].name, name)) {
			a->handler = &handlers[i];
			list_add_tail(&a->list, to_list);
			return 0;
		}
	ERROR("Unknown init handler %s\n", name);
	return -1;
}

static void dispose_action(struct init_action *action)
{
	DEBUG(4, "Delete action from the list - action %s\n", action->handler->name);

	/* Mark action for disposal */
	action->dispose = true;
	list_del(&action->list);
}

static void free_action(struct init_action *action)
{
	/* Cancel any timeout pending and delete the process */
	uloop_timeout_cancel(&action->tout);
	uloop_process_delete(&action->proc);

	free_ustream(action);

	free(action->line);
	free(action);
}

void procd_inittab_run(const char *handler)
{
	struct init_action *a;

	list_for_each_entry(a, &actions, list)
		if (!strcmp(a->handler->name, handler)) {
			if (a->handler->multi) {
				a->handler->cb(a);
				continue;
			}
			a->handler->cb(a);
			break;
		}
}

static void procd_inittab_add(struct list_head *to_list)
{
	FILE *fp = fopen(tab, "r");
	struct init_action *a;
	regex_t pat_inittab;
	regmatch_t matches[5];
	char *line;

	if (!fp) {
		ERROR("Failed to open %s\n", tab);
		return;
	}

	regcomp(&pat_inittab, "([a-zA-Z0-9]*):([a-zA-Z0-9]*):([a-zA-Z0-9]*):(.*)", REG_EXTENDED);
	line = malloc(LINE_LEN);
	a = malloc(sizeof(struct init_action));
	memset(a, 0, sizeof(struct init_action));

	while (fgets(line, LINE_LEN, fp)) {
		char *tags[TAG_PROCESS + 1];
		char *tok;
		int i;
		int len = strlen(line);

		while (isspace(line[len - 1]))
			len--;
		line[len] = 0;

		if (*line == '#')
			continue;

		if (regexec(&pat_inittab, line, 5, matches, 0))
			continue;

		DEBUG(4, "Parsing inittab - %s\n", line);

		for (i = TAG_ID; i <= TAG_PROCESS; i++) {
			line[matches[i].rm_eo] = '\0';
			tags[i] = &line[matches[i + 1].rm_so];
		};

		tok = strtok(tags[TAG_PROCESS], " ");
		for (i = 0; i < (MAX_ARGS - 1) && tok; i++) {
			a->argv[i] = tok;
			tok = strtok(NULL, " ");
		}

		a->argv[i] = NULL;
		a->id = tags[TAG_ID];
		a->line = line;
		a->dispose = false;
		a->unique_id = -1;

		if (tok && i >= (MAX_ARGS - 1)) {
			char command[LINE_LEN];
			get_action_command_string(command, sizeof(command), a);
			ERROR("Too many arguments for process '%s'\n", command);
		}

		if (add_action(a, tags[TAG_ACTION], to_list))
			continue;
		calculate_action_unique_id(a, to_list);
		line = malloc(LINE_LEN);
		a = malloc(sizeof(struct init_action));
		memset(a, 0, sizeof(struct init_action));
	}

	fclose(fp);
	free(line);
	free(a);
	regfree(&pat_inittab);
}

void procd_inittab(void)
{
	procd_inittab_add(&actions);
}

void procd_inittab_reload(int is_late)
{
	LIST_HEAD(reload_actions);
	struct init_action *reload_action, *next_reload_action;
	struct init_action *action, *next_action;
	char command[LINE_LEN];

	DEBUG(2, "Reloading inittab (late:%d)\n", is_late);
	procd_inittab_add(&reload_actions);

	// Remove existing actions not in reload
	list_for_each_entry_safe(action, next_action, &actions, list) {
		bool remove = true;
		list_for_each_entry_safe(reload_action, next_reload_action, &reload_actions, list) {
			if (is_action_line_equal(action, reload_action) &&
					action->unique_id == reload_action->unique_id) {
				remove = false;
				break;
			}
		}
		if (remove) {
			int pid = action->proc.pid;
			/* Mark action for disposal and pop it from the list */
			dispose_action(action);
			/* If process is running, start termination procedure.
			 * Resources are freed in child_exit() after process is terminated.
			 * Otherwise, free resources immediately. */
			if (pid > 0) {
				get_action_command_string(command, sizeof(command), action);
				DEBUG(4, "Killing child pid: %d '%s' #%d\n",
					pid, command, action->unique_id);
				/* Set callback to force kill the process after termination timeout */
				action->tout.cb = sigkill_action_after_timeout;
				/* Fire up termination timeout and send SIGTERM to process */
				uloop_timeout_set(&action->tout, PROCESS_TERMINATION_TIMEOUT_MS);
				kill(pid, SIGTERM);
			} else {
				free_action(action);
			}
		}
	}

	// Remove reloads for existing actions
	list_for_each_entry_safe(reload_action, next_reload_action, &reload_actions, list) {
		list_for_each_entry_safe(action, next_action, &actions, list) {
			if (is_action_line_equal(action, reload_action) &&
					action->unique_id == reload_action->unique_id) {
				dispose_action(reload_action);
				free_action(reload_action);
				break;
			}
		}
	}

	// Add and start new reload actions to the global list
	list_for_each_entry_safe(reload_action, next_reload_action, &reload_actions, list) {
		list_del(&reload_action->list);
		list_add_tail(&reload_action->list, &actions);
		// Only start handler with multi because the non multi are for startup/shutdown
		if (reload_action->handler->multi) {
			if (is_late || !strstr(reload_action->handler->name, "late"))
				reload_action->handler->cb(reload_action);
		}
	}
}
