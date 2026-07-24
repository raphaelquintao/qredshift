/*
 * Copyright (c) 2026 Raphael Quintao <raphaelquintao@gmail.com>
 * This file is part of qredshift.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "qxcb_randr.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../core/qramps.h"
#include "../utils/x11_utils.h"

#define REF_NAME "XCB RandR"

int qxcb_randr_init(QXCB_RANDR *state) {
	state->conn = xcb_connect(NULL, &state->preferred_screen);

	if (xcb_connection_has_error(state->conn)) {
		fprintf(stderr, "Failed to connect to X server.\n");
		return -1;
	}

	const xcb_setup_t *setup = xcb_get_setup(state->conn);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	for (int i = 0; iter.rem > 0; i++) {
		state->screen = iter.data;
		xcb_screen_next(&iter);
		if (i == state->preferred_screen) break;
	}

	if (state->screen == NULL) {
		fprintf(stderr, "Screen %i could not be found.\n", state->preferred_screen);
		return -1;
	}

	return 0;
}

void qxcb_randr_close(QXCB_RANDR *state) { xcb_disconnect(state->conn); }

char *int_array_to_string(uint8_t *arr, int len) {
	char *str = malloc(len + 1);
	memcpy(str, arr, len);
	str[len] = '\0';
	return str;
}


int qxcb_show_info(int only_connected, int interpolation) {
	printf("Mode: %s\n", REF_NAME);

	QXCB_RANDR state;

	if (qxcb_randr_init(&state) < 0) {
		fprintf(stderr, "error while init\n");
		return -1;
	}

	xcb_generic_error_t *error = NULL;
	int ret = 0;

	xcb_randr_get_screen_resources_current_cookie_t res_cookie = xcb_randr_get_screen_resources_current(state.conn, state.screen->root);
	xcb_randr_get_screen_resources_current_reply_t *res_reply = xcb_randr_get_screen_resources_current_reply(state.conn, res_cookie, &error);
	if (error) {
		fprintf(stderr, "`%s' returned error %d\n", "RANDR Get Screen Resources Current", error->error_code);
		free(error);
		qxcb_randr_close(&state);
		return -1;
	}

	int num_outputs = (int)res_reply->num_outputs;
	xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);

	xcb_randr_get_output_info_cookie_t *output_info_cookies = malloc(num_outputs * sizeof(*output_info_cookies));
	xcb_randr_get_output_info_reply_t **output_info_replies = calloc(num_outputs, sizeof(*output_info_replies));
	xcb_randr_get_crtc_info_cookie_t *crtc_info_cookies = calloc(num_outputs, sizeof(*crtc_info_cookies));
	xcb_randr_get_crtc_gamma_cookie_t *gamma_cookies = calloc(num_outputs, sizeof(*gamma_cookies));
	uint8_t *has_crtc_info = calloc(num_outputs, 1);
	uint8_t *has_gamma = calloc(num_outputs, 1);

	// Pass 1: pipeline all output_info requests
	for (int i = 0; i < num_outputs; i++) {
		output_info_cookies[i] = xcb_randr_get_output_info(state.conn, outputs[i], res_reply->config_timestamp);
	}

	// Pass 2a: collect all output_info replies
	for (int i = 0; i < num_outputs; i++) {
		error = NULL;
		output_info_replies[i] = xcb_randr_get_output_info_reply(state.conn, output_info_cookies[i], &error);
		if (error) {
			fprintf(stderr, "`%s' returned error %d\n", "Randr Get Output Info", error->error_code);
			free(error);
			error = NULL;
			ret = -1;
			continue;
		}
	}

	// Pass 2b: pipeline all crtc_info and gamma requests
	for (int i = 0; i < num_outputs; i++) {
		xcb_randr_get_output_info_reply_t *oir = output_info_replies[i];
		if (!oir) continue;

		int visible = (only_connected == 1 && oir->connection == 0) || (only_connected == 0);
		if (!visible) continue;

		xcb_randr_crtc_t crtc = oir->crtc;
		if (oir->connection == 0 && crtc) {
			crtc_info_cookies[i] = xcb_randr_get_crtc_info(state.conn, crtc, res_reply->config_timestamp);
			has_crtc_info[i] = 1;
		}
		if (crtc) {
			gamma_cookies[i] = xcb_randr_get_crtc_gamma(state.conn, crtc);
			has_gamma[i] = 1;
		}
	}

	// Pass 3: collect all replies and print
	for (int i = 0; i < num_outputs; i++) {
		xcb_randr_get_output_info_reply_t *oir = output_info_replies[i];
		if (!oir) continue;

		int visible = (only_connected == 1 && oir->connection == 0) || (only_connected == 0);
		if (!visible) continue;

		int w = 0, h = 0;
		if (has_crtc_info[i]) {
			error = NULL;
			xcb_randr_get_crtc_info_reply_t *crtc_info = xcb_randr_get_crtc_info_reply(state.conn, crtc_info_cookies[i], &error);
			if (!error && crtc_info) {
				w = crtc_info->width;
				h = crtc_info->height;
				free(crtc_info);
			} else {
				fprintf(stderr, "`%s' returned error %d\n", "Randr Get CRTC Info", error ? error->error_code : -1);
				if (error) {
					free(error);
					error = NULL;
				}
			}
		}

		int _nlen = oir->name_len;
		char *name = int_array_to_string(xcb_randr_get_output_info_name(oir), _nlen);

		printf("%u:%c:%s:%dx%d | ", (unsigned int)outputs[i], oir->connection == 0 ? 'C' : 'D', name, w, h);
		free(name);

		if (has_gamma[i]) {
			error = NULL;
			xcb_randr_get_crtc_gamma_reply_t *gamma_reply = xcb_randr_get_crtc_gamma_reply(state.conn, gamma_cookies[i], &error);
			if (gamma_reply && !error) {
				uint16_t *r = xcb_randr_get_crtc_gamma_red(gamma_reply);
				uint16_t *g = xcb_randr_get_crtc_gamma_green(gamma_reply);
				uint16_t *b = xcb_randr_get_crtc_gamma_blue(gamma_reply);
				QGAMMA ramp = {r, g, b};
				GammaParams params = reverse_gamma_ramp(&ramp, gamma_reply->size, interpolation);
				printf("T: %dK | B: %.2f | G: %.2f\n", params.kelvin, params.bright, params.gamma);
				free(gamma_reply);
			} else {
				printf("No gamma available\n");
				if (error) {
					free(error);
					error = NULL;
				}
			}
		} else { printf("No CRTC assigned\n"); }
	}

	// cleanup:
	for (int i = 0; i < num_outputs; i++) free(output_info_replies[i]);
	free(output_info_cookies);
	free(output_info_replies);
	free(crtc_info_cookies);
	free(gamma_cookies);
	free(has_crtc_info);
	free(has_gamma);
	free(res_reply);
	qxcb_randr_close(&state);

	return ret;
}

/**
 * Set the temperature, brightness, and gamma for the specified display targets.
 *
 * Fully pipelined XCB: all output_info requests fire together, then all gamma
 * reads, then all writes are fired with a single sync round-trip at the end,
 * no per-display blocking, and unchanged ramps are skipped via a stateless
 * read and compare against the live X server (see memcmp below).
 *
 * https://xcb.freedesktop.org/tutorial/
 * https://xcb.freedesktop.org/PublicApi/
 */
int qxcb_set_temperature(int kelvin, double bright, double gamma, int interpolation, DisplayTarget *targets, int num_targets) {
	QXCB_RANDR state;

	if (qxcb_randr_init(&state) < 0) {
		fprintf(stderr, "error while init\n");
		return 1;
	}

	int restrict_to_targets = (num_targets > 0);

	xcb_generic_error_t *error = NULL;
	int ret = 0;

	xcb_randr_get_screen_resources_current_cookie_t res_cookie = xcb_randr_get_screen_resources_current(state.conn, state.screen->root);
	xcb_randr_get_screen_resources_current_reply_t *res_reply = xcb_randr_get_screen_resources_current_reply(state.conn, res_cookie, &error);

	if (error) {
		fprintf(stderr, "`%s' returned error %d\n", "RANDR Get Screen Resources Current", error->error_code);
		free(error);
		qxcb_randr_close(&state);
		return -1;
	}

	int num_outputs = res_reply->num_outputs;
	xcb_randr_output_t *outputs = xcb_randr_get_screen_resources_current_outputs(res_reply);

	xcb_randr_get_output_info_cookie_t *out_cookies = malloc(num_outputs * sizeof(*out_cookies));
	xcb_randr_get_output_info_reply_t **out_replies = calloc(num_outputs, sizeof(*out_replies));
	xcb_randr_get_crtc_gamma_cookie_t *gamma_cookies = calloc(num_outputs, sizeof(*gamma_cookies));
	char **out_names = calloc(num_outputs, sizeof(char *));
	uint8_t *has_gamma = calloc(num_outputs, 1);

	// Pass 1: pipeline all output_info requests
	for (int i = 0; i < num_outputs; i++) {
		out_cookies[i] = xcb_randr_get_output_info(state.conn, outputs[i], res_reply->config_timestamp);
	}

	// Pass 2a: collect all output info replies
	for (int i = 0; i < num_outputs; i++) {
		error = NULL; // Reset tracking pointer before the call
		out_replies[i] = xcb_randr_get_output_info_reply(state.conn, out_cookies[i], &error);
		if (error || !out_replies[i]) {
			if (error) {
				free(error);
				error = NULL;
			}
			ret = -1;
			continue;
		}

		xcb_randr_get_output_info_reply_t *oir = out_replies[i];
		if (oir->connection != 0 || !oir->crtc) continue;

		out_names[i] = int_array_to_string(xcb_randr_get_output_info_name(oir), oir->name_len);
	}

	// Pass 2b: pipeline all gamma read requests
	for (int i = 0; i < num_outputs; i++) {
		xcb_randr_get_output_info_reply_t *oir = out_replies[i];
		if (!oir || oir->connection != 0 || !oir->crtc) continue;
		gamma_cookies[i] = xcb_randr_get_crtc_gamma(state.conn, oir->crtc);
		has_gamma[i] = 1;
	}

	// Pass 3: compare and set only changed CRTCs
	int total_changed = 0;
	for (int i = 0; i < num_outputs; i++) {
		if (!has_gamma[i]) continue;

		DisplayTarget *target = find_target(targets, num_targets, (int)outputs[i], out_names[i]);
		if (restrict_to_targets && !target) continue;

		int eff_k = kelvin;
		double eff_b = bright;
		double eff_g = gamma;

		if (target) {
			if (target->kelvin > 0) eff_k = target->kelvin;
			if (target->bright > 0) eff_b = target->bright;
			if (target->gamma > 0) eff_g = target->gamma;
		}


		error = NULL;
		xcb_randr_get_crtc_gamma_reply_t *gamma_reply = xcb_randr_get_crtc_gamma_reply(state.conn, gamma_cookies[i], &error);
		if (error || !gamma_reply) {
			if (error) {
				free(error);
				error = NULL;
			}
			ret = -1;
			continue;
		}

		int ramp_size = (int)gamma_reply->size;
		QGAMMA *new_ramp = calculate_gamma_ramp(eff_k, eff_b, eff_g, ramp_size, interpolation);

		uint16_t *cur_r = xcb_randr_get_crtc_gamma_red(gamma_reply);
		uint16_t *cur_g = xcb_randr_get_crtc_gamma_green(gamma_reply);
		uint16_t *cur_b = xcb_randr_get_crtc_gamma_blue(gamma_reply);

		int changed = memcmp(cur_r, new_ramp->r, ramp_size * sizeof(uint16_t)) != 0 ||
		              memcmp(cur_g, new_ramp->g, ramp_size * sizeof(uint16_t)) != 0 ||
		              memcmp(cur_b, new_ramp->b, ramp_size * sizeof(uint16_t)) != 0;

		free(gamma_reply);

		if (changed) {
			xcb_randr_set_crtc_gamma(state.conn, out_replies[i]->crtc, (uint16_t)ramp_size, new_ramp->r, new_ramp->g, new_ramp->b);
			total_changed++;
		}

		free_gamma_ramp(new_ramp);
	}

	// Only run the synchronous round-trip trick if mutations were pushed
	if (total_changed > 0) {
		xcb_get_input_focus_reply(state.conn, xcb_get_input_focus(state.conn), NULL);
	} else {
		xcb_flush(state.conn);
	}

	// cleanup:
	for (int i = 0; i < num_outputs; i++) {
		free(out_replies[i]);
		free(out_names[i]);
	}
	free(out_replies);
	free(out_cookies);
	free(gamma_cookies);
	free(out_names);
	free(has_gamma);
	free(res_reply);
	qxcb_randr_close(&state);

	return ret;
}
