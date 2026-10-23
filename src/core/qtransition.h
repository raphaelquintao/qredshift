/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 * https://github.com/raphaelquintao/qredshift
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdint.h>

#include "qramps.h"

uint64_t now_monotonic_ms(void);

void sleep_ms(uint32_t ms);

double transition_progress(uint64_t start_ms, uint32_t duration_ms);

/*
 * Linearly interpolate every entry of two gamma ramps:
 *   out[i] = from[i] + (to[i] - from[i]) * t
 * where t is clamped to [0.0, 1.0]. All three ramps must share the same size.
 */
void lerp_gamma_ramp(const QGAMMA *from, const QGAMMA *to, double t, QGAMMA *out, int size);
