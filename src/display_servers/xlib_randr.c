/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 * https://github.com/raphaelquintao/qredshift
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xlib_randr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../core/qramps.h"
#include "../core/qtransition.h"
#include "../globals.h"

#define REF_NAME "Xlib RandR"

int xlib_randr_init(XLIB_RANDR *state) {
	Display *dpy = XOpenDisplay(NULL);
	if (!dpy) {
		fprintf(stderr, "Unable to open display\n");
		return -1;
	}

	Window root = DefaultRootWindow(dpy);
	// XRRScreenResources *res = XRRGetScreenResources(dpy, root);
	XRRScreenResources *res = XRRGetScreenResourcesCurrent(dpy, root);
	if (!res) {
		fprintf(stderr, "Unable to get screen resources\n");
		XCloseDisplay(dpy);
		return -1;
	}

	state->dpy = dpy;
	state->res = res;

	return 0;
}

int xlib_randr_close(XLIB_RANDR *state) {
	XRRFreeScreenResources(state->res);
	XCloseDisplay(state->dpy);
	return 0;
}

int xlib_randr_show_info(int only_connected, int interpolation) {
	printf("Mode: %s\n", REF_NAME);

	XLIB_RANDR state;

	if (xlib_randr_init(&state) < 0)return -1;

	for (int c = 0; c < state.res->noutput; c++) {
		XRROutputInfo *output_info = XRRGetOutputInfo(state.dpy, state.res, state.res->outputs[c]);

		if ((only_connected == 1 && output_info->connection == 0) || only_connected == 0) {
			char *connected = output_info->connection == 0 ? "C" : "D";
			int w = 0, h = 0;

			if (output_info->connection == 0 && output_info->crtc) {
				XRRCrtcInfo *crtc_info = XRRGetCrtcInfo(state.dpy, state.res, output_info->crtc);
				w = crtc_info->width;
				h = crtc_info->height;
				XRRFreeCrtcInfo(crtc_info);
			}

			printf("%d:%s:%s:%dx%d | ", (int)state.res->outputs[c], connected, output_info->name, w, h);

			if (output_info->crtc) {
				XRRCrtcGamma *current_gamma = XRRGetCrtcGamma(state.dpy, output_info->crtc);
				QGAMMA ramp = {current_gamma->red, current_gamma->green, current_gamma->blue};
				GammaParams params = reverse_gamma_ramp(&ramp, current_gamma->size, interpolation);

				printf("T: %dK | B: %.2f | G: %.2f\n", params.kelvin, params.bright, params.gamma);

				XRRFreeGamma(current_gamma);
			} else { printf("No CRTC assigned\n"); }
		}

		XRRFreeOutputInfo(output_info);
	}

	xlib_randr_close(&state);

	return 0;
}


typedef struct {
	RRCrtc crtc;
	int size;
	QGAMMA *from;
	QGAMMA *to;
	QGAMMA *cur;
} XLIB_ANIM;

static void xlib_anim_free(XLIB_ANIM *anims, int n) {
	for (int i = 0; i < n; i++) {
		free_gamma_ramp(anims[i].from);
		free_gamma_ramp(anims[i].to);
		free_gamma_ramp(anims[i].cur);
	}
	free(anims);
}

int xlib_randr_set_temperature(int kelvin, double bright, double gamma, int interpolation,
                               DisplayTarget *targets, int num_targets, int transition_ms) {
	XLIB_RANDR state;

	if (xlib_randr_init(&state) < 0) return -1;

	int restrict_to_targets = (num_targets > 0);

	XLIB_ANIM *anims = NULL;
	int n_anims = 0;
	if (transition_ms > 0) {
		anims = calloc((size_t)state.res->noutput, sizeof(XLIB_ANIM));
		if (!anims) {
			xlib_randr_close(&state);
			return -1;
		}
	}

	for (int c = 0; c < state.res->noutput; c++) {
		XRROutputInfo *output_info = XRRGetOutputInfo(state.dpy, state.res, state.res->outputs[c]);
		if (output_info->connection == 1 || !output_info->crtc) {
			XRRFreeOutputInfo(output_info);
			continue;
		}

		DisplayTarget *target = find_target(targets, num_targets, (int)state.res->outputs[c], output_info->name);
		if (restrict_to_targets && !target) {
			XRRFreeOutputInfo(output_info);
			continue;
		}

		int eff_k = (target && target->kelvin > 0) ? target->kelvin : kelvin;
		double eff_b = (target && target->bright > 0) ? target->bright : bright;
		double eff_g = (target && target->gamma > 0) ? target->gamma : gamma;

		XRRCrtcGamma *current = XRRGetCrtcGamma(state.dpy, output_info->crtc);
		if (!current) {
			XRRFreeOutputInfo(output_info);
			continue;
		}

		int ramp_size = current->size;
		QGAMMA *to = calculate_gamma_ramp(eff_k, eff_b, eff_g, ramp_size, interpolation);
		if (!to) {
			XRRFreeGamma(current);
			XRRFreeOutputInfo(output_info);
			continue;
		}

		int changed = memcmp(current->red, to->r,
		                     ramp_size * sizeof(unsigned short)
		              ) != 0 ||
		              memcmp(current->green, to->g,
		                     ramp_size * sizeof(unsigned short)
		              ) != 0 ||
		              memcmp(current->blue, to->b,
		                     ramp_size * sizeof(unsigned short)
		              ) != 0;

		if (!changed) {
			XRRFreeGamma(current);
			free_gamma_ramp(to);
			XRRFreeOutputInfo(output_info);
			continue;
		}

		if (transition_ms <= 0) {
			/* Instant apply: no animation, exit right away. */
			XRRCrtcGamma wrap = {ramp_size, to->r, to->g, to->b};
			XRRSetCrtcGamma(state.dpy, output_info->crtc, &wrap);
			XRRFreeGamma(current);
			free_gamma_ramp(to);
			XRRFreeOutputInfo(output_info);
			continue;
		}

		/* Defer this output so the change can be animated smoothly. */
		XLIB_ANIM *a = &anims[n_anims];
		a->crtc = output_info->crtc;
		a->size = ramp_size;
		a->to = to;
		a->from = alloc_gamma_ramp(ramp_size);
		a->cur = alloc_gamma_ramp(ramp_size);

		if (!a->from || !a->cur) {
			free_gamma_ramp(a->from);
			free_gamma_ramp(a->cur);
			free_gamma_ramp(a->to);
			XRRFreeGamma(current);
			XRRFreeOutputInfo(output_info);
			continue;
		}

		memcpy(a->from->r, current->red, ramp_size * sizeof(unsigned short));
		memcpy(a->from->g, current->green, ramp_size * sizeof(unsigned short));
		memcpy(a->from->b, current->blue, ramp_size * sizeof(unsigned short));

		XRRFreeGamma(current);
		n_anims++;
		XRRFreeOutputInfo(output_info);
	}

	if (n_anims > 0) {
		uint64_t start = now_monotonic_ms();
		int done = 0;

		while (!done) {
			double t = transition_progress(start, (uint32_t)transition_ms);
			done = (t >= 1.0);

			for (int i = 0; i < n_anims; i++) {
				XLIB_ANIM *a = &anims[i];
				lerp_gamma_ramp(a->from, a->to, t, a->cur, a->size);

				XRRCrtcGamma wrap = {a->size, a->cur->r, a->cur->g, a->cur->b};
				XRRSetCrtcGamma(state.dpy, a->crtc, &wrap);
			}
			XSync(state.dpy, False);

			if (!done) sleep_ms(TRANSITION_FRAME_MS);
		}
	}

	xlib_anim_free(anims, n_anims);

	XSync(state.dpy, False);
	xlib_randr_close(&state);

	return 0;
}
