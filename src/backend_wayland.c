/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 * https://github.com/raphaelquintao/qredshift
 * SPDX-License-Identifier: Apache-2.0
 */

#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L

#include "backend_wayland.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "globals.h"
#include "display_servers/qwlr.h"
#include "utils/qcli.h"
#include "utils/x11_utils.h"


enum {
	P_HELP = 0,
	P_VERSION,
	P_INFO,
	P_SEP,
	P_TEMP,
	P_BRIGHT,
	P_GAMMA,
	P_DISPLAY,
	P_SEP2,
	P_INTERP,
	P_WD,
};


static const char *runtime_dir(void) {
	const char *dir = getenv("XDG_RUNTIME_DIR");
	return (dir && dir[0] != '\0') ? dir : "/tmp";
}

static const char *fifo_path(void) {
	static char path[256];
	static int initialized = 0;
	if (!initialized) {
		const char *dir = runtime_dir();
		snprintf(path, sizeof(path), "%s/qredshift-wayland.fifo", dir);
		initialized = 1;
	}
	return path;
}

static void info_reply_path(char *buf, size_t buf_size, pid_t pid) {
	snprintf(buf, buf_size, "%s/qredshift-wayland-info-%d.fifo", runtime_dir(), (int)pid);
}


// --- FIFO client helpers -------------------------------------------------------

static bool is_daemon_running(void) {
	const char *path = fifo_path();
	if (access(path, F_OK) != 0) return false;

	int fd = open(path, O_WRONLY | O_NONBLOCK);
	if (fd < 0) return false;

	return true;
}

static bool try_send_to_daemon(int kelvin, double bright, double gamma, DisplayTarget *targets, int num_targets, int interpolation) {
	const char *path = fifo_path();

	if (access(path, F_OK) != 0) return false;

	int fd = open(path, O_WRONLY | O_NONBLOCK);
	if (fd < 0) return false;

	char line[2048];
	int len = snprintf(line, sizeof(line), "SET %d %f %f %d %d", kelvin, bright, gamma, interpolation, num_targets);

	for (int i = 0; i < num_targets && len < (int)sizeof(line); i++) {
		len += snprintf(line + len, sizeof(line) - (size_t)len,
		                " %s:%d:%f:%f", targets[i].id, targets[i].kelvin, targets[i].bright, targets[i].gamma
		);
	}

	dprintf(fd, "%s\n", line);
	close(fd);
	return true;
}

static bool try_query_daemon_info(void) {
	const char *path = fifo_path();
	if (access(path, F_OK) != 0) return false;

	char reply_path[256];
	info_reply_path(reply_path, sizeof(reply_path), getpid());

	unlink(reply_path);
	if (mkfifo(reply_path, 0600) < 0) {
		perror("mkfifo (info reply)");
		return false;
	}

	int reply_fd = open(reply_path, O_RDONLY | O_NONBLOCK);
	if (reply_fd < 0) {
		perror("open (info reply)");
		unlink(reply_path);
		return false;
	}

	int cmd_fd = open(path, O_WRONLY | O_NONBLOCK);
	if (cmd_fd < 0) {
		close(reply_fd);
		unlink(reply_path);
		return false;
	}
	dprintf(cmd_fd, "INFO %s\n", reply_path);
	close(cmd_fd);

	struct pollfd pfd = {.fd = reply_fd, .events = POLLIN};
	bool got_reply = false;
	char buf[4096];
	size_t total = 0;

	while (poll(&pfd, 1, 2000) > 0) {
		ssize_t n = read(reply_fd, buf + total, sizeof(buf) - 1 - total);
		if (n > 0) {
			total += (size_t)n;
			got_reply = true;
		} else { break; }
	}

	if (got_reply) {
		buf[total] = '\0';
		fputs(buf, stdout);
	}

	close(reply_fd);
	unlink(reply_path);
	return got_reply;
}

// --- FIFO command handler ------------------------------------------------------

static void handle_fifo_line(const char *line) {
	char cmd[16] = {0};

	if (sscanf(line, "%15s", cmd) != 1) return;

	if (strcmp(cmd, "SET") == 0) {
		int kelvin = DEF_KELVIN;
		double bright = DEF_BRIGHT;
		double gamma = DEF_GAMMA;
		int interp = 0;
		int num_targets = 0;
		int consumed = 0;

		if (sscanf(line, "SET %d %lf %lf %d %d%n", &kelvin, &bright, &gamma, &interp, &num_targets, &consumed) < 3) {
			fprintf(stderr, "Ignoring malformed SET command: %s\n", line);
			return;
		}

		kelvin = clamp_int(kelvin, MIN_KELVIN, MAX_KELVIN);
		bright = clamp_float(bright, MIN_BRIGHT, MAX_BRIGHT);
		gamma = clamp_float(gamma, MIN_GAMMA, MAX_GAMMA);

		DisplayTarget targets[MAX_TARGETS];
		int found = 0;

		if (num_targets > 0) {
			const char *cursor = line + consumed;
			for (int i = 0; i < num_targets && i < MAX_TARGETS; i++) {
				char id[64] = {0};
				int t_kelvin = -1;
				double t_bright = -1.0;
				double t_gamma = -1.0;

				while (*cursor == ' ') cursor++;
				int used = 0;
				if (sscanf(cursor, "%63[^:]:%d:%lf:%lf%n", id, &t_kelvin, &t_bright, &t_gamma, &used) != 4) {
					fprintf(stderr, "Ignoring malformed SET target " "in: %s\n", line);
					break;
				}
				cursor += used;

				snprintf(targets[found].id, sizeof(targets[found].id), "%s", id);
				targets[found].kelvin = t_kelvin;
				targets[found].bright = t_bright;
				targets[found].gamma = t_gamma;
				found++;
			}
		}

		printf("%s \n", line);

		qwlr_set_temperature(kelvin, bright, gamma, targets, found, interp);

	} else if (strcmp(cmd, "INFO") == 0) {
		char reply_path[256] = {0};
		if (sscanf(line, "INFO %255s", reply_path) != 1) {
			fprintf(stderr, "Ignoring malformed INFO command: %s\n", line);
			return;
		}
		qwlr_write_info(reply_path);
	} else {
		fprintf(stderr, "Ignoring unknown FIFO command: %s\n", line);
	}
}

// --- Daemon loop

static void handle_signal(int sig) {
	(void)sig;
	qwlr_stop();
}

static int run_daemon(int kelvin, double bright, double gamma, DisplayTarget *targets,
                      int num_targets, int interpolation, int start_only) {
	if (!qwlr_init()) return EXIT_FAILURE;

	if (!start_only)
		qwlr_set_temperature(kelvin, bright, gamma, targets, num_targets, interpolation);

	const char *path = fifo_path();
	unlink(path);
	if (mkfifo(path, 0600) < 0) {
		perror("mkfifo");
		qwlr_cleanup();
		return EXIT_FAILURE;
	}

	int fifo_fd = open(path, O_RDWR | O_NONBLOCK);
	if (fifo_fd < 0) {
		perror("open fifo");
		qwlr_cleanup();
		return EXIT_FAILURE;
	}

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	printf("Wayland daemon started (fifo: %s)\n", path);

	char buf[512];
	size_t buf_len = 0;

	while (qwlr_is_running()) {
		while (qwlr_prepare_read() != 0)
			qwlr_dispatch_pending();
		qwlr_flush();

		struct pollfd fds[2];
		fds[0].fd = qwlr_get_fd();
		fds[0].events = POLLIN;
		fds[1].fd = fifo_fd;
		fds[1].events = POLLIN;

		int ret = poll(fds, 2, -1);
		if (ret < 0) {
			qwlr_cancel_read();
			if (errno == EINTR) continue;
			perror("poll");
			break;
		}

		if (fds[0].revents & POLLIN) qwlr_read_events();
		else qwlr_cancel_read();
		qwlr_dispatch_pending();

		if (fds[1].revents & POLLIN) {
			ssize_t n = read(fifo_fd, buf + buf_len, sizeof(buf) - 1 - buf_len);
			if (n > 0) {
				buf_len += (size_t)n;
				buf[buf_len] = '\0';

				char *line_start = buf;
				char *newline;
				while ((newline = strchr(line_start, '\n')) != NULL) {
					*newline = '\0';
					handle_fifo_line(line_start);
					line_start = newline + 1;
				}

				size_t remaining = buf_len - (size_t)(line_start - buf);
				memmove(buf, line_start, remaining);
				buf_len = remaining;
			} else if (n < 0 && errno != EAGAIN) { perror("read fifo"); }
		}
	}

	if (fifo_fd >= 0) close(fifo_fd);
	unlink(fifo_path());
	qwlr_cleanup();
	printf("Wayland daemon stopped\n");
	return EXIT_SUCCESS;
}

// --- Entry point ---------------------------------------------------------------

int wayland_backend(int argc, char *argv[]) {
	PARAM params[] = {
		{"-h", "Display this help", "", 0},
		{"-v", "Show program version", "", 0},
		{"-i", "Show display info", "", 0},
		{"", "", "", 0},
		{"-t", "Temperature in kelvin from " TOSTRING(MIN_KELVIN) " to " TOSTRING(MAX_KELVIN), TOSTRING(DEF_KELVIN), 0},
		{"-b", "Brightness from " TOSTRING(MIN_BRIGHT) " to " TOSTRING(MAX_BRIGHT), TOSTRING(DEF_BRIGHT), 0},
		{"-g", "Gamma from " TOSTRING(MIN_GAMMA) " to " TOSTRING(MAX_GAMMA), TOSTRING(DEF_GAMMA), 0},
		{"-d", "Target display (repeatable)", "<id[:t=K][:b=B][:g=G]>", 0},
		{"", "", "", 0},
		{"-interp", "Use interpolation method", "", 0},
		{"-wd", "Starts wayland daemon without initial values", "", 0},
	};

	const int params_size = sizeof(params) / sizeof(PARAM);

	if (argc <= 1) {
		print_help(params_size, params, stderr);
		return EXIT_FAILURE;
	}

	parse_args(argc, argv, params_size, params);

	if (params[P_HELP].exists) {
		print_help(params_size, params, stdout);
		return EXIT_SUCCESS;
	}

	if (params[P_VERSION].exists) {
		printf("%s %s\n", APP_NAME, APP_VERSION);
		return EXIT_SUCCESS;
	}

	const int interp = params[P_INTERP].exists;

	if (params[P_INFO].exists) {
		char *de = getenv("XDG_CURRENT_DESKTOP");
		printf("Display Server: %s\n", "wayland");
		printf("Desktop Environment: %s\n", de ? de : "unknown");

		if (try_query_daemon_info()) return EXIT_SUCCESS;

		printf("Mode: Wayland (wlr-gamma-control-unstable-v1)\n");
		printf("No qredshift daemon is currently running.\n");
		printf("Run '%s -wd' to start it.\n", APP_NAME);
		return EXIT_SUCCESS;
	}

	if (!(params[P_TEMP].exists || params[P_BRIGHT].exists || params[P_GAMMA].exists
		|| params[P_DISPLAY].exists || params[P_WD].exists)) {
		printf("Try '%s -h' for more information.\n", APP_NAME);
		return EXIT_FAILURE;
	}

	int kelvin = qatoi(params[P_TEMP].value, DEF_KELVIN);
	double bright = qatof(params[P_BRIGHT].value, DEF_BRIGHT);
	double gamma = qatof(params[P_GAMMA].value, DEF_GAMMA);

	kelvin = clamp_int(kelvin, MIN_KELVIN, MAX_KELVIN);
	bright = clamp_float(bright, MIN_BRIGHT, MAX_BRIGHT);
	gamma = clamp_float(gamma, MIN_GAMMA, MAX_GAMMA);

	DisplayTarget targets[MAX_TARGETS];
	int num_targets = 0;
	for (int i = 1; i < argc && num_targets < MAX_TARGETS; i++) {
		if (strcmp(argv[i], "-d") == 0 && i + 1 < argc)
			parse_display_target(argv[++i], &targets[num_targets++]);
	}

	if (params[P_WD].exists) {
		if (is_daemon_running()) {
			printf("Wayland daemon already running: (fifo: %s)\n", fifo_path());
			return EXIT_FAILURE;
		}
		return run_daemon(kelvin, bright, gamma, targets, num_targets, interp, 1);
	}

	if (try_send_to_daemon(kelvin, bright, gamma, targets, num_targets, interp)) return EXIT_SUCCESS;

	return run_daemon(kelvin, bright, gamma, targets, num_targets, interp, 0);
}
