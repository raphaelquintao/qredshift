/*
 * Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>
 * This file is part of qredshift.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

typedef struct {
	int kelvin;
	double gamma;
	double bright;
} GammaParams;

typedef struct {
	double r;
	double g;
	double b;
} RGB;

typedef struct {
	unsigned short *r;
	unsigned short *g;
	unsigned short *b;
} QGAMMA;


QGAMMA *calculate_gamma_ramp(int kelvin, double bright, double gamma, int ramp_size, int interpolation);

GammaParams reverse_gamma_ramp(const QGAMMA *ramp, int ramp_size, int interpolation);

void free_gamma_ramp(QGAMMA *ramp);
