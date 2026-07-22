// Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdio.h>

typedef struct {
	char *name;
	char *desc;
	char *value;
	int exists;
} PARAM;

typedef enum {
	BACKEND_X11,
	BACKEND_WAYLAND,
	BACKEND_UNKNOWN
} DisplayBackend;

DisplayBackend detect_display_backend(void);

int clamp_int(int value, int min, int max);

double clamp_float(double value, double min, double max);

void print_help(int params_size, PARAM *params, FILE *stream);
