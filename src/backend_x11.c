// Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>
// SPDX-License-Identifier: Apache-2.0

#include "backend_x11.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "display_servers/qxcb_randr.h"
#include "display_servers/xlib_randr.h"
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
	P_XLIB,
	P_INTERP,
};


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

void parse_display_target(const char *arg, DisplayTarget *target) {
	target->kelvin = -1;
	target->bright = -1.0;
	target->gamma = -1.0;
	target->id[0] = '\0';

	char buf[256];
	strncpy(buf, arg, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	char *token = strtok(buf, ":");
	if (!token) return;
	strncpy(target->id, token, sizeof(target->id) - 1);
	target->id[sizeof(target->id) - 1] = '\0';

	while ((token = strtok(NULL, ":")) != NULL) {
		if (strncmp(token, "t=", 2) == 0)
			target->kelvin = clamp_int(atoi(token + 2), 1000, 25000);
		else if (strncmp(token, "b=", 2) == 0)
			target->bright = clamp_float(atof(token + 2), 0.1, 1.0);
		else if (strncmp(token, "g=", 2) == 0)
			target->gamma = clamp_float(atof(token + 2), 0.1, 5.0);
	}
}

int x11_backend(int argc, char *argv[]) {

	PARAM params[] = {
		{"-h", "Display this help", "", 0},
		{"-v", "Show program version", "", 0},
		{"-i", "Show display info", "", 0},
		{"", "", "", 0},
		{"-t", "Temperature in kelvin 1000 to 25000", "6500", 0},
		{"-b", "Brightness from 0.1 to 1.0", "1.0", 0},
		{"-g", "Gamma from 0.1 to 5.0", "1.0", 0},
		{"-d", "Target display (repeatable)", "<id[:t=K][:b=B][:g=G]>", 0},
		{"", "", "", 0},
		{"-xlib", "Use Xlib instead of XCB", "", 0},
		{"-interp", "Use interpolation method", "", 0},
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
	const int xlib = params[P_XLIB].exists;

	if (params[P_INFO].exists) {
		char *de = getenv("XDG_CURRENT_DESKTOP");
		printf("Display Server: %s\n", "x11");
		printf("Desktop Environment: %s\n", de ? de : "unknown");

		if (xlib) xlib_randr_show_info(1, interp);
		else qxcb_show_info(1, interp);

		return EXIT_SUCCESS;
	}

	if (!(params[P_TEMP].exists || params[P_BRIGHT].exists || params[P_GAMMA].exists || params[P_DISPLAY].exists)) {
		printf("Try '%s -h' for more information.\n", APP_NAME);
		return EXIT_FAILURE;
	}

	int kelvin = atoi(params[P_TEMP].value);
	double bright = atof(params[P_BRIGHT].value);
	double gamma = atof(params[P_GAMMA].value);

	kelvin = clamp_int(kelvin, 1000, 25000);
	bright = clamp_float(bright, 0.1, 1.0);
	gamma = clamp_float(gamma, 0.1, 5.0);

	DisplayTarget targets[16];
	int num_targets = 0;
	for (int i = 1; i < argc && num_targets < 16; i++) {
		if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
			parse_display_target(argv[++i], &targets[num_targets++]);
		}
	}

	// printf("kelvin: %d\n", kelvin);
	// printf("bright: %f\n", bright);
	// printf(" gamma: %f\n", gamma);
	//
	// for (int i = 0; i < num_targets; i++) {
	// 	printf("Target %d: id=%s, kelvin=%d, bright=%.2f, gamma=%.2f\n", i, targets[i].id, targets[i].kelvin, targets[i].bright, targets[i].gamma);
	// }

	if (xlib) xlib_randr_set_temperature(kelvin, bright, gamma, interp, targets, num_targets);
	else qxcb_set_temperature(kelvin, bright, gamma, interp, targets, num_targets);


	return EXIT_SUCCESS;
}
