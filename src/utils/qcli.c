/*
 * Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>
 * This file is part of qredshift.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "qcli.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../globals.h"

DisplayBackend detect_display_backend(void) {
	const char *wayland_display = getenv("WAYLAND_DISPLAY");
	if (wayland_display && wayland_display[0] != '\0') {
		const char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
		if (xdg_runtime_dir && xdg_runtime_dir[0] != '\0') {
			char socket_path[1024];
			snprintf(socket_path, sizeof(socket_path), "%s/%s", xdg_runtime_dir, wayland_display);
			if (access(socket_path, R_OK) == 0) {
				return BACKEND_WAYLAND;
			}
		}
	}

	const char *x11_display = getenv("DISPLAY");
	if (x11_display && x11_display[0] != '\0') {
		return BACKEND_X11;
	}

	return BACKEND_UNKNOWN;
}

int clamp_int(int value, int min, int max) {
	return (value < min) ? min : ((value > max) ? max : value);
}

double clamp_float(double value, double min, double max) {
	return (value < min) ? min : ((value > max) ? max : value);
}

int qatoi(const char *__nptr, const int _def) {
	if (__nptr == NULL) return _def;

	int saved_errno = errno;
	errno = 0;
	char *endptr;
	long val = strtol(__nptr, &endptr, 10);

	if (errno || endptr == __nptr || val < INT_MIN || val > INT_MAX) {
		errno = saved_errno;
		return _def;
	}

	errno = saved_errno;
	return (int)val;
}

double qatof(const char *__nptr, double _def) {
	if (__nptr == NULL) return _def;

	int saved_errno = errno;
	errno = 0;
	char *endptr;
	double val = strtod(__nptr, &endptr);

	if (errno || endptr == __nptr || isinf(val) || isnan(val)) {
		errno = saved_errno;
		return _def;
	}

	errno = saved_errno;
	return val;
}


void print_help(int params_size, PARAM *params, FILE *stream) {
	fprintf(stream, "Usage: %s -t [temperature in Kelvin] -b [bright] -g [gamma]\n\n", APP_NAME);
	for (int c = 0; c < params_size; c++) {
		if (strcmp(params[c].name, "") == 0)
			fprintf(stream, "\n");
		else if (c >= params_size - 2)
			fprintf(stream, "  %-7s %19s %s\n", params[c].name, params[c].value, params[c].desc);
		else
			fprintf(stream, "  %-3s %-23s %s\n", params[c].name, params[c].value, params[c].desc);
	}
}

void parse_args(int argc, char *argv[], int params_size, PARAM *params) {
	for (int c = 0; c < params_size; c++) {
		if (strcmp(params[c].name, "") == 0) continue;
		for (int i = 1; i < argc; i++) {
			if (strcmp(argv[i], params[c].name) == 0) {
				params[c].exists = 1;
				if (i + 1 < argc) {
					params[c].value = argv[i + 1];
					i++;
				}
			}
		}
	}
}
