/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 * https://github.com/raphaelquintao/qredshift
 * SPDX-License-Identifier: Apache-2.0
 */

#define _POSIX_C_SOURCE 200809L

#include "qtransition.h"

#include <errno.h>
#include <time.h>

uint64_t now_monotonic_ms(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000U + (uint64_t)ts.tv_nsec / 1000000U;
}

void sleep_ms(uint32_t ms) {
	struct timespec req = {
		.tv_sec = (time_t)(ms / 1000U),
		.tv_nsec = (long)(ms % 1000U) * 1000000L
	};

	while (nanosleep(&req, &req) == -1 && errno == EINTR) {
		/* resume the remaining time if interrupted by a signal */
	}
}

double transition_progress(uint64_t start_ms, uint32_t duration_ms) {
	if (duration_ms == 0) return 1.0;

	uint64_t elapsed = now_monotonic_ms() - start_ms;
	double t = (double)elapsed / (double)duration_ms;

	if (t < 0.0) return 0.0;
	if (t > 1.0) return 1.0;
	return t;
}

void lerp_gamma_ramp(const QGAMMA *from, const QGAMMA *to, double t, QGAMMA *out, int size) {
	if (t < 0.0) t = 0.0;
	if (t > 1.0) t = 1.0;

	for (int i = 0; i < size; i++) {
		double dr = (double)to->r[i] - (double)from->r[i];
		double dg = (double)to->g[i] - (double)from->g[i];
		double db = (double)to->b[i] - (double)from->b[i];

		out->r[i] = (unsigned short)((double)from->r[i] + dr * t);
		out->g[i] = (unsigned short)((double)from->g[i] + dg * t);
		out->b[i] = (unsigned short)((double)from->b[i] + db * t);
	}
}
