#include "xlib_randr.h"
#include <stdio.h>
#include <string.h>
#include "../core/qramps.h"

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


int xlib_randr_set_temperature(int kelvin, double bright, double gamma, int interpolation,
                               DisplayTarget *targets, int num_targets) {
	XLIB_RANDR state;

	if (xlib_randr_init(&state) < 0) return -1;

	int restrict_to_targets = (num_targets > 0);

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
		QGAMMA *new_ramp = calculate_gamma_ramp(eff_k, eff_b, eff_g, ramp_size,
		                                        interpolation
		);

		int changed = memcmp(current->red, new_ramp->r,
		                     ramp_size * sizeof(unsigned short)
		              ) != 0 ||
		              memcmp(current->green, new_ramp->g,
		                     ramp_size * sizeof(unsigned short)
		              ) != 0 ||
		              memcmp(current->blue, new_ramp->b,
		                     ramp_size * sizeof(unsigned short)
		              ) != 0;

		XRRFreeGamma(current);

		if (changed) {
			XRRCrtcGamma wrap = {ramp_size, new_ramp->r, new_ramp->g, new_ramp->b};
			XRRSetCrtcGamma(state.dpy, output_info->crtc, &wrap);
		}

		free_gamma_ramp(new_ramp);
		XRRFreeOutputInfo(output_info);
	}

	XSync(state.dpy, False);
	xlib_randr_close(&state);

	return 0;
}
