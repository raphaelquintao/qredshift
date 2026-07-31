/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 * https://github.com/raphaelquintao/qredshift
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


extern const char *APP_NAME;
extern const char *APP_VERSION;

extern int Q_VERBOSE;


// Maximum number of "-d" target overrides accepted on the command line.
#define MAX_TARGETS 16


// Don't change that, unless you know what you're doing.
// It can cause number overflow and lead to undefined behavior.
#define MIN_KELVIN 1000
#define MAX_KELVIN 25000  // Max to use -interp.
#define DEF_KELVIN 6500

#define MIN_BRIGHT 0.1
#define MAX_BRIGHT 1.0
#define DEF_BRIGHT 1.0

#define MIN_GAMMA 0.1
#define MAX_GAMMA 5.0
#define DEF_GAMMA 1.0
