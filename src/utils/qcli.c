// Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>
// SPDX-License-Identifier: Apache-2.0

#include "qcli.h"
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