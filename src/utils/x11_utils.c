/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 * https://github.com/raphaelquintao/qredshift
 * SPDX-License-Identifier: Apache-2.0
 */

#include "x11_utils.h"
#include <stdlib.h>
#include <string.h>
#include "qcli.h"
#include "../globals.h"

DisplayTarget *find_target(DisplayTarget *targets, int num_targets, int output_id, const char *name) {
	if (num_targets <= 0) return NULL;

	for (int i = 0; i < num_targets; i++) {
		char *endptr;
		// Attempt to parse the configured ID string as a number
		long parsed_val = strtol(targets[i].id, &endptr, 10);

		// If the entire config ID was numeric, compare it directly to the hardware XID
		if (*endptr == '\0' && targets[i].id != endptr) {
			if (parsed_val == (long)output_id) {
				return &targets[i];
			}
		}

		// Otherwise, match against the readable output interface name (e.g., "HDMI-1")
		if (name && strcmp(targets[i].id, name) == 0) {
			return &targets[i];
		}
	}
	return NULL;
}

// Parses a "-d" argument of the form "<id>[:t=K][:b=B][:g=G]" into a
// DisplayTarget. Unspecified fields are left at their "unset" sentinel
// values (-1 / -1.0) so callers can fall back to the global setting.
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
			target->kelvin = clamp_int(qatoi(token + 2, -1), MIN_KELVIN, MAX_KELVIN);
		else if (strncmp(token, "b=", 2) == 0)
			target->bright = clamp_float(qatof(token + 2, -1.0), MIN_BRIGHT, MAX_BRIGHT);
		else if (strncmp(token, "g=", 2) == 0)
			target->gamma = clamp_float(qatof(token + 2, -1.0), MIN_GAMMA, MAX_GAMMA);
	}
}
