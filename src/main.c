#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "backend_wayland.h"
#include "backend_x11.h"
#include "utils/qcli.h"


// const char *APP_NAME = NAME;
// const char *APP_VERSION = VERSION;

int main(int argc, char *argv[]) {
	DisplayBackend backend = detect_display_backend();

	switch (backend) {
		case BACKEND_WAYLAND:
			printf("[qredshift] Wayland environment verified.\n");
			return wayland_backend(argc, argv);

		case BACKEND_X11:
			// printf("[qredshift] X11 environment verified.\n");
			return x11_backend(argc, argv);
		case BACKEND_UNKNOWN:
		default:
			fprintf(stderr, "Error: No active X11 or Wayland display session detected.\n");
			return EXIT_FAILURE;
	}
}
