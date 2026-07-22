/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "x11_utils.h"
#include <stdlib.h>
#include <string.h>


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
