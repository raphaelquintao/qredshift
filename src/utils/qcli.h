/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 * https://github.com/raphaelquintao/qredshift
 * SPDX-License-Identifier: Apache-2.0
 */

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

/**
 * Safe string to integer conversion with default value
 * if input is NULL, empty or overflow/underflow
 * @note Preserves errno state
 * @note Matching behavior: atoi()
 */
int qatoi(const char *__nptr, int _def);

/**
 * Safe string to double conversion with default value
 * if input is NULL, empty, Infinity/NaN, or overflow/underflow
 * @note Preserves errno state
 * @note Matching behavior: atof()
 */
double qatof(const char *__nptr, double _def);

void print_help(int params_size, PARAM *params, FILE *stream);

void parse_args(int argc, char *argv[], int params_size, PARAM *params);
