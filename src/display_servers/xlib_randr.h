/*
 * Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>
 * This file is part of qredshift.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include "../utils/x11_utils.h"


typedef struct {
	Display *dpy;
	XRRScreenResources *res;
} XLIB_RANDR;


int xlib_randr_init(XLIB_RANDR *state);

int xlib_randr_close(XLIB_RANDR *state);

int xlib_randr_show_info(int connected, int interpolation);

int xlib_randr_set_temperature(int kelvin, double bright, double gamma,
                               int interpolation, DisplayTarget *targets,
                               int num_targets);
