/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 * https://github.com/raphaelquintao/qredshift
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
#include "../core/qtransition.h"
#include "../globals.h"
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

	/* Transition */
	QGAMMA *anim_from;
	QGAMMA *anim_to;
	QGAMMA *anim_cur;
	int anim_size;
	uint64_t anim_start_ms;
	uint32_t anim_duration_ms;
	bool animating;
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

// Set a fully computed ramp to the compositor via an anonymous shm file
static bool apply_ramp(OutputState *os, const QGAMMA *ramp, int size) {
	size_t table_size = 3 * (size_t)size * sizeof(uint16_t);
	int fd = create_anonymous_file(table_size);
	if (fd < 0) return false;

	ssize_t written = write(fd, ramp->r, table_size);
	if (written < 0 || (size_t)written != table_size) {
		perror("write gamma table");
		close(fd);
		return false;
	}
	lseek(fd, 0, SEEK_SET);

	zwlr_gamma_control_v1_set_gamma(os->gamma_control, fd);
	close(fd);
	return true;
}

// Drops any animation state for an output without touching the displayed ramp.
static void release_animation(OutputState *os) {
	free_gamma_ramp(os->anim_from);
	free_gamma_ramp(os->anim_to);
	free_gamma_ramp(os->anim_cur);
	os->anim_from = NULL;
	os->anim_to = NULL;
	os->anim_cur = NULL;
	os->anim_size = 0;
	os->anim_duration_ms = 0;
	os->animating = false;
}

void qwlr_set_temperature(int kelvin, double bright, double gamma, DisplayTarget *targets, int num_targets, int interpolation, int transition_ms) {
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
		    && !os->animating
		    && os->interpolation == interpolation
		    && os->current_kelvin == eff_k
		    && os->current_bright == eff_b
		    && os->current_gamma == eff_g) {
			continue;
		}

		if (!os->gamma_control || !os->gamma_size_known || os->failed) continue;
		if (os->gamma_size == 0) continue;

		int size = (int)os->gamma_size;
		QGAMMA *to = calculate_gamma_ramp(eff_k, eff_b, eff_g, size, interpolation);
		if (!to) {
			fprintf(stderr, "Failed to compute gamma ramp\n");
			continue;
		}

		if (transition_ms <= 0) {
			// Instant path: drop any running animation and apply directly.
			// No transition ramps are computed or allocated.
			release_animation(os);
			apply_ramp(os, to, size);
			free_gamma_ramp(to);

			os->current_kelvin = eff_k;
			os->current_bright = eff_b;
			os->current_gamma = eff_g;
			os->interpolation = interpolation;
			os->has_current = true;
			continue;
		}

		// Transition path: capture the starting ramp before tearing down any
		// running animation. If a transition is already running it continues
		// smoothly from the frame currently on screen.
		QGAMMA *from = NULL;
		if (os->animating && os->anim_cur && os->anim_size == size) {
			from = os->anim_cur;
			os->anim_cur = NULL;
		}
		free_gamma_ramp(os->anim_from);
		free_gamma_ramp(os->anim_to);
		os->anim_from = NULL;
		os->anim_to = NULL;
		os->anim_size = 0;
		os->anim_duration_ms = 0;
		os->animating = false;

		if (!from) {
			int fk = os->has_current ? os->current_kelvin : DEF_KELVIN;
			double fb = os->has_current ? os->current_bright : DEF_BRIGHT;
			double fg = os->has_current ? os->current_gamma : DEF_GAMMA;
			int fi = os->has_current ? os->interpolation : interpolation;
			from = calculate_gamma_ramp(fk, fb, fg, size, fi);
		}
		if (!from) {
			free_gamma_ramp(to);
			continue;
		}

		QGAMMA *cur = alloc_gamma_ramp(size);
		if (!cur) {
			free_gamma_ramp(from);
			free_gamma_ramp(to);
			continue;
		}

		os->anim_from = from;
		os->anim_to = to;
		os->anim_cur = cur;
		os->anim_size = size;
		os->anim_start_ms = now_monotonic_ms();
		os->anim_duration_ms = (uint32_t)transition_ms;
		os->animating = true;

		os->current_kelvin = eff_k;
		os->current_bright = eff_b;
		os->current_gamma = eff_g;
		os->interpolation = interpolation;
		os->has_current = true;

		lerp_gamma_ramp(os->anim_from, os->anim_to, 0.0, os->anim_cur, size);
		apply_ramp(os, os->anim_cur, size);
	}

	wl_display_flush(g_state.display);
}

// Advance any in-progress ramp transitions by one animation frame.
// Returns true while at least one output is still animating.
bool qwlr_animate_step(void) {
	bool any = false;

	for (int i = 0; i < g_state.output_count; i++) {
		OutputState *os = &g_state.outputs[i];
		if (!os->animating) continue;

		double t = transition_progress(os->anim_start_ms, os->anim_duration_ms);
		lerp_gamma_ramp(os->anim_from, os->anim_to, t, os->anim_cur, os->anim_size);
		apply_ramp(os, os->anim_cur, os->anim_size);

		if (t >= 1.0) {
			release_animation(os);
		} else {
			any = true;
		}
	}

	if (any) wl_display_flush(g_state.display);
	return any;
}

// Milliseconds until the next frame should run, or -1 when no transition is active.
int qwlr_next_timeout_ms(void) {
	uint64_t now = now_monotonic_ms();
	int best = -1;

	for (int i = 0; i < g_state.output_count; i++) {
		OutputState *os = &g_state.outputs[i];
		if (!os->animating) continue;

		uint64_t end = os->anim_start_ms + os->anim_duration_ms;
		int remaining = (now >= end) ? 0 : (int)(end - now);
		if (remaining > TRANSITION_FRAME_MS) remaining = TRANSITION_FRAME_MS;
		if (best < 0 || remaining < best) best = remaining;
	}

	return best;
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
		free_gamma_ramp(os->anim_from);
		free_gamma_ramp(os->anim_to);
		free_gamma_ramp(os->anim_cur);
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
