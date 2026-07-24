/*
 * Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>
 * This file is part of qredshift.
 * SPDX-License-Identifier: Apache-2.0
 */

#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L

#include "qwlr.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client.h>

#include "../core/qramps.h"
#include "../utils/x11_utils.h"
#include "protocols/wlr-gamma-control-unstable-v1-protocol.h"

#define MAX_OUTPUTS 16

typedef struct {
	struct wl_output *wl_output;
	struct zwlr_gamma_control_v1 *gamma_control;
	uint32_t global_name;
	uint32_t gamma_size;
	bool gamma_size_known;
	bool failed;
	char name[64];
	int32_t width;
	int32_t height;
	int current_kelvin;
	double current_bright;
	double current_gamma;
	bool has_current;
	int interpolation;
} OutputState;

typedef struct {
	struct wl_display *display;
	struct wl_registry *registry;
	struct zwlr_gamma_control_manager_v1 *manager;
	OutputState outputs[MAX_OUTPUTS];
	int output_count;
	volatile bool running;
} WaylandState;

static WaylandState g_state;

// --- zwlr_gamma_control_v1 listener -----------------------------------------

static void qwlr_handle_gamma_size(void *data, struct zwlr_gamma_control_v1 *control, uint32_t size) {
	(void)control;
	OutputState *os = data;
	os->gamma_size = size;
	os->gamma_size_known = true;
}

static void qwlr_handle_failed(void *data, struct zwlr_gamma_control_v1 *control) {
	(void)control;
	OutputState *os = data;
	os->failed = true;
	fprintf(stderr, "gamma control failed for an output\n");
}

static const struct zwlr_gamma_control_v1_listener gamma_control_listener = {
	.gamma_size = qwlr_handle_gamma_size,
	.failed = qwlr_handle_failed,
};

// --- wl_output listener ----------------------------------------------------

static void output_handle_geometry(void *data, struct wl_output *output,
                                   int32_t x, int32_t y,
                                   int32_t physical_width,
                                   int32_t physical_height,
                                   int32_t subpixel,
                                   const char *make, const char *model,
                                   int32_t transform) {
	(void)data;
	(void)output;
	(void)x;
	(void)y;
	(void)physical_width;
	(void)physical_height;
	(void)subpixel;
	(void)make;
	(void)model;
	(void)transform;
}

static void output_handle_mode(void *data, struct wl_output *output,
                               uint32_t flags, int32_t width,
                               int32_t height, int32_t refresh) {
	(void)output;
	(void)refresh;
	OutputState *os = data;

	if (flags & WL_OUTPUT_MODE_CURRENT) {
		os->width = width;
		os->height = height;
	}
}

static void output_handle_done(void *data, struct wl_output *output) {
	(void)data;
	(void)output;
}

static void output_handle_scale(void *data, struct wl_output *output,
                                int32_t factor) {
	(void)data;
	(void)output;
	(void)factor;
}

static void output_handle_name(void *data, struct wl_output *output,
                               const char *name) {
	(void)output;
	OutputState *os = data;
	strncpy(os->name, name, sizeof(os->name) - 1);
	os->name[sizeof(os->name) - 1] = '\0';
}

static void output_handle_description(void *data, struct wl_output *output,
                                      const char *description) {
	(void)data;
	(void)output;
	(void)description;
}

static const struct wl_output_listener output_listener = {
	.geometry = output_handle_geometry,
	.mode = output_handle_mode,
	.done = output_handle_done,
	.scale = output_handle_scale,
	.name = output_handle_name,
	.description = output_handle_description,
};

// --- wl_registry listener

static void registry_handle_global(void *data, struct wl_registry *registry,
                                   uint32_t name, const char *interface,
                                   uint32_t version) {
	(void)data;
	if (strcmp(interface, zwlr_gamma_control_manager_v1_interface.name) == 0) {
		g_state.manager = wl_registry_bind(
			registry, name, &zwlr_gamma_control_manager_v1_interface,
			version < 1 ? version : 1
		);
	} else if (strcmp(interface, wl_output_interface.name) == 0) {
		if (g_state.output_count < MAX_OUTPUTS) {
			uint32_t bind_version = version < 4 ? version : 4;
			if (bind_version < 1) bind_version = 1;
			struct wl_output *output = wl_registry_bind(
				registry, name, &wl_output_interface, bind_version
			);
			OutputState *os = &g_state.outputs[g_state.output_count++];
			memset(os, 0, sizeof(*os));
			os->wl_output = output;
			os->global_name = name;
			snprintf(os->name, sizeof(os->name), "output-%u", name);
			wl_output_add_listener(output, &output_listener, os);
		}
	}
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
	(void)data;
	(void)registry;
	(void)name;
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove,
};

// --- Gamma ramp application

static int create_anonymous_file(size_t size) {
	char template[] = "/tmp/qredshift-shm-XXXXXX";
	int fd = mkstemp(template);
	if (fd < 0) {
		perror("mkstemp");
		return -1;
	}
	unlink(template);
	if (ftruncate(fd, (off_t)size) < 0) {
		perror("ftruncate");
		close(fd);
		return -1;
	}
	return fd;
}

void qwlr_set_temperature(int kelvin, double bright, double gamma, DisplayTarget *targets, int num_targets, int interpolation) {
	int restrict_to_targets = (num_targets > 0);

	for (int i = 0; i < g_state.output_count; i++) {
		OutputState *os = &g_state.outputs[i];

		DisplayTarget *target = find_target(targets, num_targets, (int)os->global_name, os->name);
		if (restrict_to_targets && !target) continue;

		int eff_k = kelvin;
		double eff_b = bright;
		double eff_g = gamma;

		if (target) {
			if (target->kelvin > 0) eff_k = target->kelvin;
			if (target->bright > 0) eff_b = target->bright;
			if (target->gamma > 0) eff_g = target->gamma;
		}

		if (os->has_current
		    && os->interpolation == interpolation
		    && os->current_kelvin == eff_k
		    && os->current_bright == eff_b
		    && os->current_gamma == eff_g) {
			continue;
		}

		if (!os->gamma_control || !os->gamma_size_known || os->failed) continue;
		if (os->gamma_size == 0) continue;

		QGAMMA *ramp = calculate_gamma_ramp(eff_k, eff_b, eff_g, (int)os->gamma_size, interpolation);
		if (!ramp) {
			fprintf(stderr, "Failed to compute gamma ramp\n");
			continue;
		}

		size_t table_size = 3 * (size_t)os->gamma_size * sizeof(uint16_t);
		int fd = create_anonymous_file(table_size);
		if (fd < 0) {
			free_gamma_ramp(ramp);
			continue;
		}

		ssize_t written = write(fd, ramp->r, table_size);
		if (written < 0 || (size_t)written != table_size) {
			perror("write gamma table");
			close(fd);
			free_gamma_ramp(ramp);
			continue;
		}
		lseek(fd, 0, SEEK_SET);

		zwlr_gamma_control_v1_set_gamma(os->gamma_control, fd);
		close(fd);
		free_gamma_ramp(ramp);

		os->current_kelvin = eff_k;
		os->current_bright = eff_b;
		os->current_gamma = eff_g;
		os->interpolation = interpolation;
		os->has_current = true;
	}

	wl_display_flush(g_state.display);
}

// --- Public API

bool qwlr_init(void) {
	memset(&g_state, 0, sizeof(g_state));
	g_state.running = true;

	g_state.display = wl_display_connect(NULL);
	if (!g_state.display) {
		fprintf(stderr, "Failed to connect to Wayland display\n");
		return false;
	}

	g_state.registry = wl_display_get_registry(g_state.display);
	if (!g_state.registry) {
		fprintf(stderr, "Failed to get Wayland registry\n");
		wl_display_disconnect(g_state.display);
		return false;
	}
	wl_registry_add_listener(g_state.registry, &registry_listener, NULL);

	if (wl_display_roundtrip(g_state.display) < 0) {
		fprintf(stderr, "Failed to roundtrip Wayland display\n");
		wl_registry_destroy(g_state.registry);
		wl_display_disconnect(g_state.display);
		return false;
	}

	if (!g_state.manager) {
		fprintf(stderr, "Compositor does not support wlr-gamma-control-unstable-v1\n");
		wl_registry_destroy(g_state.registry);
		wl_display_disconnect(g_state.display);
		return false;
	}

	if (g_state.output_count == 0) {
		fprintf(stderr, "No outputs found\n");
		wl_registry_destroy(g_state.registry);
		wl_display_disconnect(g_state.display);
		return false;
	}

	for (int i = 0; i < g_state.output_count; i++) {
		OutputState *os = &g_state.outputs[i];
		os->gamma_control = zwlr_gamma_control_manager_v1_get_gamma_control(g_state.manager, os->wl_output);
		if (os->gamma_control) {
			zwlr_gamma_control_v1_add_listener(os->gamma_control, &gamma_control_listener, os);
		}
	}

	if (wl_display_roundtrip(g_state.display) < 0) {
		fprintf(stderr, "Failed to roundtrip after binding gamma controls\n");
		return false;
	}

	return true;
}

void qwlr_cleanup(void) {
	for (int i = 0; i < g_state.output_count; i++) {
		OutputState *os = &g_state.outputs[i];
		if (os->gamma_control) zwlr_gamma_control_v1_destroy(os->gamma_control);
		if (os->wl_output) wl_output_destroy(os->wl_output);
	}

	if (g_state.manager) zwlr_gamma_control_manager_v1_destroy(g_state.manager);
	if (g_state.registry) wl_registry_destroy(g_state.registry);
	if (g_state.display) wl_display_disconnect(g_state.display);
}

void qwlr_write_info(const char *reply_path) {
	int fd = open(reply_path, O_WRONLY | O_NONBLOCK);
	if (fd < 0) return;

	FILE *f = fdopen(fd, "w");
	if (!f) {
		close(fd);
		return;
	}

	fprintf(f, "Mode: Wayland (wlr-gamma-control-unstable-v1)\n");

	if (g_state.output_count == 0) {
		fprintf(f, "No outputs found\n");
	}

	for (int i = 0; i < g_state.output_count; i++) {
		OutputState *os = &g_state.outputs[i];

		fprintf(f, "%u:C:%s:%dx%d | ", os->global_name, os->name, os->width, os->height);

		if (os->failed || !os->gamma_control) {
			fprintf(f, "No gamma control available\n");
		} else if (!os->gamma_size_known) {
			fprintf(f, "Gamma size not yet known\n");
		} else if (!os->has_current) {
			fprintf(f, "No settings applied yet\n");
		} else {
			fprintf(f, "T: %dK | B: %.2f | G: %.2f\n", os->current_kelvin, os->current_bright, os->current_gamma);
		}
	}

	fclose(f);
}

int qwlr_get_fd(void) {
	return wl_display_get_fd(g_state.display);
}

int qwlr_prepare_read(void) {
	return wl_display_prepare_read(g_state.display);
}

void qwlr_dispatch_pending(void) {
	wl_display_dispatch_pending(g_state.display);
}

void qwlr_flush(void) {
	wl_display_flush(g_state.display);
}

void qwlr_read_events(void) {
	wl_display_read_events(g_state.display);
}

void qwlr_cancel_read(void) {
	wl_display_cancel_read(g_state.display);
}

void qwlr_stop(void) {
	g_state.running = false;
}

bool qwlr_is_running(void) {
	return g_state.running;
}
