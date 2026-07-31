/*
 * Copyright (c) 2026 - Raphael Quintao <raphaelquintao@gmail.com>
 * https://github.com/raphaelquintao/qredshift
 * SPDX-License-Identifier: Apache-2.0
 */

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "backend_x11.h"
#include "globals.h"
#include "utils/qcli.h"

#define PLUGIN_NAME "libqredshift_wayland_" VERSION ".so"

int main(int argc, char *argv[]) {
	DisplayBackend backend = detect_display_backend();

	switch (backend) {
		case BACKEND_WAYLAND: ;
			{
				char exe_path[1024];
				char plugin_path[2048];

				union {
					void *data_ptr;
					int (*func_ptr)(int, char *[]);
				} caster;


				ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
				if (len != -1) {
					exe_path[len] = '\0';

					char *last_slash = strrchr(exe_path, '/');
					if (last_slash != NULL) {
						*last_slash = '\0';
						snprintf(plugin_path, sizeof(plugin_path), "%s/" PLUGIN_NAME, exe_path);
					} else {
						snprintf(plugin_path, sizeof(plugin_path), "./" PLUGIN_NAME);
					}
				} else {
					snprintf(plugin_path, sizeof(plugin_path), "./" PLUGIN_NAME);
				}
				// printf("%s\n", plugin_path);
				void *plugin = dlopen(plugin_path, RTLD_NOW);
				if (!plugin) {
					plugin = dlopen("/usr/lib/qredshift/" PLUGIN_NAME, RTLD_NOW);
				}

				if (!plugin) {
					fprintf(stderr, "Runtime Error: Wayland session detected, but backend library could not be loaded");
					fprintf(stderr, "%s\n", dlerror());
					fprintf(stderr, "Make sure you have it on \"/usr/lib/qredshift/" PLUGIN_NAME "\" or in same path of the binary.\n");
					return EXIT_FAILURE;
				}

				// 2. Fetch the symbol safely
				void *symbol_ptr = dlsym(plugin, "wayland_backend");
				if (!symbol_ptr) {
					fprintf(stderr, "Runtime Error: Failed to find 'wayland_backend' symbol in library: %s\n", dlerror());
					dlclose(plugin);
					return EXIT_FAILURE;
				}

				// Assign through the union (POSIX/C99 safe type punning technique)
				caster.data_ptr = symbol_ptr;
				int (*wayland_backend_ptr)(int, char *[]) = caster.func_ptr;

				// 3. Execute it safely
				int result = wayland_backend_ptr(argc, argv);

				// 4. Clean up
				dlclose(plugin);
				return result;
			}
		// return wayland_backend(argc, argv);
		case BACKEND_X11:
			return x11_backend(argc, argv);

		case BACKEND_UNKNOWN:
		default:
			fprintf(stderr, "Error: No active X11 or Wayland display session detected.\n");
			return EXIT_FAILURE;
	}
}
