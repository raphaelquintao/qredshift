/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
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
