#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <linux/input-event-codes.h>
#include <wayland-util.h>

#include "cat.h"
#include "shm.h"
#include "xdg-shell-client-protocol.h"

static const int width = 128;
static const int height = 128;

static bool configured = false;
static bool running = true;

static struct wl_shm *shm = NULL;
static struct wl_compositor *compositor = NULL;
static struct xdg_wm_base *xdg_wm_base = NULL;

static void *shm_data = NULL;
static struct wl_surface *surface = NULL;
static struct xdg_toplevel *xdg_toplevel = NULL;

static void noop() {
	// This space intentionally left blank
    // Stands for "No Operation". Used to ignore things.
}

static void xdg_wm_base_handle_ping(void *data,
		struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
	// The compositor will send us a ping event to check that we're responsive.
	// We need to send back a pong request immediately.
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_handle_ping,
};

static void xdg_surface_handle_configure(void *data,
		struct xdg_surface *xdg_surface, uint32_t serial) {
	// The compositor configures our surface, acknowledge the configure event
	xdg_surface_ack_configure(xdg_surface, serial);

	if (configured) {
		// If this isn't the first configure event we've received, we already
		// have a buffer attached, so no need to do anything. Commit the
		// surface to apply the configure acknowledgement.
		wl_surface_commit(surface);
	}

	configured = true;
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_handle_configure,
};

static void xdg_toplevel_handle_close(void *data,
		struct xdg_toplevel *xdg_toplevel) {
	// Stop running if the user requests to close the toplevel
	running = false;
}

static void xdg_toplevel_handle_configure(void *data,
                                          struct xdg_toplevel *xdg_toplevel,
                                          int32_t width,
                                          int32_t height,
                                          struct wl_array *states) {
    // struct client_state *state = data;
    if (width == 0 || height == 0) {
        printf("%d",width);
        printf("%d",height);
        /* Compositor is deferring to us */
        return;
    }
    // ADDED FOR DEBUG/TROUBLESHOOTING - Ac5000
    printf("handle_config called\n");
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = xdg_toplevel_handle_configure,
	.close = xdg_toplevel_handle_close,
};

static void pointer_handle_button(void *data, struct wl_pointer *pointer,
		uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
	// struct wl_seat *seat = data;

	// If the user presses the left pointer button, start an interactive move
	// of the toplevel
	// if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED) {
	// 	xdg_toplevel_move(xdg_toplevel, seat, serial);
	// }

     // Make fullscreen when right-mouse is released. 
    if (button == BTN_RIGHT && state == WL_POINTER_BUTTON_STATE_RELEASED ) {
        // If null, compositor chooses the output.
        xdg_toplevel_set_fullscreen(xdg_toplevel, NULL);
	}

     // Unset fullscreen when left-mouse is released. 
    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_RELEASED ) {
        // If null, compositor chooses the output.
        xdg_toplevel_unset_fullscreen(xdg_toplevel);
	}
}

static const struct wl_pointer_listener pointer_listener = {
    //Do something when mouse enters the app window/area.
	.enter = noop,
    //Do something when mouse leaves the app window/area.
	.leave = noop,
    //Do something when mouse moves inside the app window/area.
	.motion = noop,
    //Do something when mouse buttons are pressed.
	.button = pointer_handle_button,
    //Do something with mouse axes. (Mouse wheel, up/down, etc.)
	.axis = noop,
};

static void seat_handle_capabilities(void *data, struct wl_seat *seat,
		uint32_t capabilities) {
	// If the wl_seat has the pointer capability, start listening to pointer
	// events
	if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
		struct wl_pointer *pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(pointer, &pointer_listener, seat);
	}
}

static const struct wl_seat_listener seat_listener = {
	.capabilities = seat_handle_capabilities,
};

static void handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	if (strcmp(interface, wl_shm_interface.name) == 0) {
		shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		struct wl_seat *seat =
			wl_registry_bind(registry, name, &wl_seat_interface, 1);
		wl_seat_add_listener(seat, &seat_listener, NULL);
	} else if (strcmp(interface, wl_compositor_interface.name) == 0) {
		compositor = wl_registry_bind(registry, name,
			&wl_compositor_interface, 1);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);
	}
}

static void handle_global_remove(void *data, struct wl_registry *registry,
		uint32_t name) {
	// Who cares
}

static const struct wl_registry_listener registry_listener = {
	.global = handle_global,
	.global_remove = handle_global_remove,
};

static struct wl_buffer *create_buffer(void) {
	int stride = width * 4;
	int size = stride * height;

	// Allocate a shared memory file with the right size
	int fd = create_shm_file(size);
	if (fd < 0) {
		fprintf(stderr, "creating a buffer file for %d B failed: %m\n", size);
		return NULL;
	}

	// Map the shared memory file
	shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (shm_data == MAP_FAILED) {
		fprintf(stderr, "mmap failed: %m\n");
		close(fd);
		return NULL;
	}

	// Create a wl_buffer from our shared memory file descriptor
	struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
	struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height,
		stride, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);

	// Now that we've mapped the file and created the wl_buffer, we no longer
	// need to keep file descriptor opened
	close(fd);

	// Copy pixels into our shared memory file (MagickImage is from cat.h)
	memcpy(shm_data, MagickImage, size);

	return buffer;
}

int main(int argc, char *argv[]) {
/* 
    In summary, the following steps will take you from zero to a window on-screen:

    Bind to wl_compositor and use it to create a wl_surface.
    Bind to xdg_wm_base and use it to create an xdg_surface with your wl_surface.
    Create an xdg_toplevel from the xdg_surface with xdg_surface.get_toplevel.
    Configure a listener for the xdg_surface and await the configure event.
    Bind to the buffer allocation mechanism of your choosing (such as wl_shm) and allocate a shared buffer, then render your content to it.
    Use wl_surface.attach to attach the wl_buffer to the wl_surface.
    Use xdg_surface.ack_configure, passing it the serial from configure, acknowledging that you have prepared a suitable frame.
    Send a wl_surface.commit request.
 */
	// Connect to the Wayland compositor
	struct wl_display *display = wl_display_connect(NULL);
	if (display == NULL) {
		fprintf(stderr, "failed to create display\n");
		return EXIT_FAILURE;
	}

	// Obtain the wl_registry and fetch the list of globals
	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	if (wl_display_roundtrip(display) == -1) {
		return EXIT_FAILURE;
	}

	// Check that all globals we require are available
	if (shm == NULL || compositor == NULL || xdg_wm_base == NULL) {
		fprintf(stderr, "no wl_shm, wl_compositor or xdg_wm_base support\n");
		return EXIT_FAILURE;
	}

	// Create a wl_surface, a xdg_surface and a xdg_toplevel
	surface = wl_compositor_create_surface(compositor);
	struct xdg_surface *xdg_surface =
		xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
	xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);

    xdg_toplevel_set_title(xdg_toplevel, "Hello Wayland Ac5000");
	xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
	xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);

	// Perform the initial commit and wait for the first configure event
	wl_surface_commit(surface);
	while (wl_display_dispatch(display) != -1 && !configured) {
		// This space intentionally left blank
	}

	// Create a wl_buffer, attach it to the surface and commit the surface
	struct wl_buffer *buffer = create_buffer();
	if (buffer == NULL) {
		return EXIT_FAILURE;
	}

	wl_surface_attach(surface, buffer, 0, 0);
	wl_surface_commit(surface);

	// Continue dispatching events until the user closes the toplevel
	while (wl_display_dispatch(display) != -1 && running) {
		// This space intentionally left blank
	}

	xdg_toplevel_destroy(xdg_toplevel);
	xdg_surface_destroy(xdg_surface);
	wl_surface_destroy(surface);
	wl_buffer_destroy(buffer);

	return EXIT_SUCCESS;
}
