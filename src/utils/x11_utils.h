/*
 * Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>
 * This file is part of qredshift.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

typedef struct {
	char id[64];
	int kelvin;
	double bright;
	double gamma;
} DisplayTarget;

DisplayTarget *find_target(DisplayTarget *targets, int num_targets, int index, const char *name);

void parse_display_target(const char *arg, DisplayTarget *target);
