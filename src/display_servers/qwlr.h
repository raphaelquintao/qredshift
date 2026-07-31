/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 * https://github.com/raphaelquintao/qredshift
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "../utils/x11_utils.h"



bool qwlr_init(void);
void qwlr_cleanup(void);

void qwlr_set_temperature(int kelvin, double bright, double gamma, DisplayTarget *targets, int num_targets, int interpolation);

void qwlr_write_info(const char *reply_path);

int qwlr_get_fd(void);
int qwlr_prepare_read(void);
void qwlr_dispatch_pending(void);
void qwlr_flush(void);
void qwlr_read_events(void);
void qwlr_cancel_read(void);

void qwlr_stop(void);
bool qwlr_is_running(void);
