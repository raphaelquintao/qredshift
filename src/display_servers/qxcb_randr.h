/*
 * Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>
 * This file is part of qredshift.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <xcb/randr.h>
#include <xcb/xcb.h>
#include "../utils/x11_utils.h"

typedef struct {
	xcb_connection_t* conn;
	xcb_screen_t* screen;
	int preferred_screen;
} QXCB_RANDR;


int qxcb_randr_init(QXCB_RANDR* state);

void qxcb_randr_close(QXCB_RANDR* state);

int qxcb_show_info(int only_connected, int interpolation);

int qxcb_set_temperature(int kelvin, double bright, double gamma, int interpolation, DisplayTarget* targets,
	int num_targets);
