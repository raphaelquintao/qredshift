/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 * https://github.com/raphaelquintao/qredshift
 * SPDX-License-Identifier: Apache-2.0
 */

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
	P_INTERP,
	P_XLIB,
};

int x11_backend(int argc, char *argv[]) {

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
		{"-xlib", "Use Xlib instead of XCB", "", 0},
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

	int kelvin = qatoi(params[P_TEMP].value, DEF_KELVIN);
	double bright = qatof(params[P_BRIGHT].value, DEF_BRIGHT);
	double gamma = qatof(params[P_GAMMA].value, DEF_GAMMA);

	kelvin = clamp_int(kelvin, MIN_KELVIN, MAX_KELVIN);
	bright = clamp_float(bright, MIN_BRIGHT, MAX_BRIGHT);
	gamma = clamp_float(gamma, MIN_GAMMA, MAX_GAMMA);

	DisplayTarget targets[MAX_TARGETS];
	int num_targets = 0;
	for (int i = 1; i < argc && num_targets < MAX_TARGETS; i++) {
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
