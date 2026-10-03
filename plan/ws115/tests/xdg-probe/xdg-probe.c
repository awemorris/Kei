/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The xdg-shell probe (ws115-p009): a client whose xdg-shell code comes from
 * upstream wayland-scanner, as GTK's does, linked with zedBSD's
 * libwayland-client.  It shows a window on the compositor and uses the client
 * ABI that p009 added, then prints one PASS line or the step that failed.
 *
 *   xdg-probe <seconds>
 *
 * The window is a 320x200 shared-memory buffer in one colour, titled
 * "xdg-probe", kept up for the given number of seconds.  On the way the probe
 * installs a log handler, gives the surface a listener with the version 6
 * members, offsets the buffer only when the surface's version allows it, and
 * reads the data device manager's version.
 */

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <wayland-client.h>

#include "xdg-shell-client-protocol.h"

/*
 * The window's size in pixels and the colour it is filled with (XRGB).
 */
#define PROBE_WIDTH 320
#define PROBE_HEIGHT 200
#define PROBE_COLOUR 0xff2a7fd4U

/*
 * The globals the probe binds and the state of its one window.
 */
struct probe {
	struct wl_display *display;
	struct wl_compositor *compositor;
	struct wl_shm *shm;
	struct xdg_wm_base *wm_base;
	struct wl_output *output;
	struct wl_data_device_manager *data_device_manager;
	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *toplevel;
	int configured;
	int frame_done;
	int closed;
};

static void probe_log(const char *format, va_list arguments) __attribute__((__format__(__printf__, 1, 0)));
static const char *probe_run(struct probe *probe, int seconds);
static const char *probe_bind(struct probe *probe);
static const char *probe_window(struct probe *probe);
static const char *probe_draw(struct probe *probe);
static void probe_release(struct probe *probe);
static int probe_buffer(struct probe *probe, struct wl_buffer **buffer);
static void probe_registry_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
static void probe_registry_remove(void *data, struct wl_registry *registry, uint32_t name);
static void probe_wm_base_ping(void *data, struct xdg_wm_base *wm_base, uint32_t serial);
static void probe_xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
static void probe_toplevel_configure(void *data, struct xdg_toplevel *toplevel, int32_t width, int32_t height, struct wl_array *states);
static void probe_toplevel_close(void *data, struct xdg_toplevel *toplevel);
static void probe_toplevel_bounds(void *data, struct xdg_toplevel *toplevel, int32_t width, int32_t height);
static void probe_toplevel_capabilities(void *data, struct xdg_toplevel *toplevel, struct wl_array *capabilities);
static void probe_surface_enter(void *data, struct wl_surface *surface, struct wl_output *output);
static void probe_surface_leave(void *data, struct wl_surface *surface, struct wl_output *output);
static void probe_surface_scale(void *data, struct wl_surface *surface, int32_t factor);
static void probe_surface_transform(void *data, struct wl_surface *surface, uint32_t transform);
static void probe_frame_done(void *data, struct wl_callback *callback, uint32_t time);
static uint32_t probe_min(uint32_t a, uint32_t b);

/*
 * The listeners: upstream's generated layouts for xdg-shell, and
 * libwayland-client's for the core interfaces, wl_surface with its
 * version 6 members.
 */
static const struct wl_registry_listener probe_registry_listener = {
	probe_registry_global,
	probe_registry_remove,
};
static const struct xdg_wm_base_listener probe_wm_base_listener = {
	probe_wm_base_ping,
};
static const struct xdg_surface_listener probe_xdg_surface_listener = {
	probe_xdg_surface_configure,
};
static const struct xdg_toplevel_listener probe_toplevel_listener = {
	probe_toplevel_configure,
	probe_toplevel_close,
	probe_toplevel_bounds,
	probe_toplevel_capabilities,
};
static const struct wl_surface_listener probe_surface_listener = {
	probe_surface_enter,
	probe_surface_leave,
	probe_surface_scale,
	probe_surface_transform,
};
static const struct wl_callback_listener probe_frame_listener = {
	probe_frame_done,
};

/*
 * Shows the window and reports the result.
 */
int
main(
	int argc,
	char **argv)
{
	struct probe probe;
	const char *failed_step;
	int seconds;

	/* Refuses a call without the time the window stays up. */
	if (argc != 2) {
		fprintf(stderr, "usage: xdg-probe <seconds>\n");
		return 2;
	}

	/* Connects and runs the steps, which stop at the first that fails. */
	seconds = atoi(argv[1]);
	memset(&probe, 0, sizeof(probe));
	wl_log_set_handler_client(probe_log);
	probe.display = wl_display_connect(NULL);
	if (probe.display == NULL) {
		printf("xdg-probe: FAIL connect\n");
		return 1;
	}

	/* Runs the window and releases everything it made. */
	failed_step = probe_run(&probe, seconds);
	probe_release(&probe);
	wl_display_disconnect(probe.display);
	if (failed_step != NULL) {
		printf("xdg-probe: FAIL %s\n", failed_step);
		return 1;
	}

	/* Reports that the window was shown and every call went through. */
	printf("xdg-probe: PASS\n");

	/* Succeeded: the window was shown for the whole time. */
	return 0;
}

/* Prints a message the library logs, so the run's record shows it. */
static void
probe_log(
	const char *format,
	va_list arguments)
{
	/* Marks the line as the library's. */
	printf("xdg-probe: libwayland: ");
	vprintf(format, arguments);
}

/* Binds the globals, shows the window and keeps it up, naming a failed step. */
static const char *
probe_run(
	struct probe *probe,
	int seconds)
{
	const char *failed_step;
	time_t now;
	time_t end;
	int dispatched;

	/* Binds what the window needs. */
	failed_step = probe_bind(probe);
	if (failed_step != NULL)
		return failed_step;

	/* Makes the toplevel and waits for its first configure. */
	failed_step = probe_window(probe);
	if (failed_step != NULL)
		return failed_step;

	/* Draws the buffer and waits until the compositor has shown it. */
	failed_step = probe_draw(probe);
	if (failed_step != NULL)
		return failed_step;

	/* Keeps the window up, answering the compositor, for the time asked. */
	now = time(NULL);
	end = now + seconds;
	while (now < end && !probe->closed) {
		wl_display_flush(probe->display);
		dispatched = wl_display_dispatch_pending(probe->display);
		if (dispatched < 0)
			return "dispatch";

		/* Waits a little between rounds, then reads what arrived. */
		usleep(100000);
		wl_display_roundtrip(probe->display);
		now = time(NULL);
	}

	/* Succeeded: the window stayed up. */
	return NULL;
}

/* Binds the globals through the registry. */
static const char *
probe_bind(
	struct probe *probe)
{
	struct wl_registry *registry;
	int round;

	/* Lists the globals; the listener binds the ones the probe uses. */
	registry = wl_display_get_registry(probe->display);
	wl_registry_add_listener(registry, &probe_registry_listener, probe);
	round = wl_display_roundtrip(probe->display);
	if (round < 0)
		return "registry";

	/* Refuses a compositor without the three globals a window needs. */
	if (probe->compositor == NULL || probe->shm == NULL || probe->wm_base == NULL)
		return "globals";

	/* Records the data device manager's version (p009's addition). */
	if (probe->data_device_manager != NULL) {
		printf("xdg-probe: data device manager version %u\n",
		       wl_data_device_manager_get_version(probe->data_device_manager));
	}

	/* Succeeded: the registry is no longer needed. */
	wl_registry_destroy(registry);
	return NULL;
}

/* Makes the toplevel window and waits for its first configure. */
static const char *
probe_window(
	struct probe *probe)
{
	uint32_t version;
	int round;

	/* Makes the surface with the listener that has the version 6 members. */
	probe->surface = wl_compositor_create_surface(probe->compositor);
	wl_surface_add_listener(probe->surface, &probe_surface_listener, probe);
	version = wl_surface_get_version(probe->surface);
	printf("xdg-probe: wl_surface version %u, xdg_wm_base version %u\n", version,
	       xdg_wm_base_get_version(probe->wm_base));

	/* Makes it a toplevel through the upstream-generated xdg-shell code. */
	probe->xdg_surface = xdg_wm_base_get_xdg_surface(probe->wm_base, probe->surface);
	xdg_surface_add_listener(probe->xdg_surface, &probe_xdg_surface_listener, probe);
	probe->toplevel = xdg_surface_get_toplevel(probe->xdg_surface);
	xdg_toplevel_add_listener(probe->toplevel, &probe_toplevel_listener, probe);
	xdg_toplevel_set_title(probe->toplevel, "xdg-probe");
	xdg_toplevel_set_app_id(probe->toplevel, "xdg-probe");
	wl_surface_commit(probe->surface);

	/* Waits for the configure the compositor answers the first commit with. */
	while (!probe->configured) {
		round = wl_display_dispatch(probe->display);
		if (round < 0)
			return "configure";
	}

	/* Succeeded: the toplevel is configured. */
	return NULL;
}

/* Attaches the buffer and waits for the frame callback of the commit. */
static const char *
probe_draw(
	struct probe *probe)
{
	struct wl_buffer *buffer;
	struct wl_callback *frame;
	uint32_t version;
	int made;
	int round;

	/* Makes the shared-memory buffer. */
	made = probe_buffer(probe, &buffer);
	if (made != 0)
		return "buffer";

	/* Attaches it, damaging in buffer coordinates where the version allows. */
	version = wl_surface_get_version(probe->surface);
	wl_surface_attach(probe->surface, buffer, 0, 0);
	if (version >= WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION) {
		wl_surface_damage_buffer(probe->surface, 0, 0, PROBE_WIDTH, PROBE_HEIGHT);
	} else {
		wl_surface_damage(probe->surface, 0, 0, PROBE_WIDTH, PROBE_HEIGHT);
	}

	/* Offsets the buffer only on a surface new enough for it, as GTK does. */
	if (version >= WL_SURFACE_OFFSET_SINCE_VERSION) {
		wl_surface_offset(probe->surface, 0, 0);
		printf("xdg-probe: wl_surface.offset sent\n");
	} else {
		printf("xdg-probe: wl_surface.offset skipped (version %u < %u)\n", version,
		       (unsigned)WL_SURFACE_OFFSET_SINCE_VERSION);
	}

	/* Asks for the frame callback and commits. */
	frame = wl_surface_frame(probe->surface);
	wl_callback_add_listener(frame, &probe_frame_listener, probe);
	wl_surface_commit(probe->surface);

	/* Waits for the compositor to show the frame. */
	while (!probe->frame_done) {
		round = wl_display_dispatch(probe->display);
		if (round < 0)
			return "frame";
	}

	/* Succeeded: the buffer is on screen; the surface keeps it. */
	printf("xdg-probe: window shown %dx%d\n", PROBE_WIDTH, PROBE_HEIGHT);
	return NULL;
}

/* Releases the window and the globals. */
static void
probe_release(
	struct probe *probe)
{
	/* The window, innermost first. */
	if (probe->toplevel != NULL)
		xdg_toplevel_destroy(probe->toplevel);
	if (probe->xdg_surface != NULL)
		xdg_surface_destroy(probe->xdg_surface);
	if (probe->surface != NULL)
		wl_surface_destroy(probe->surface);

	/* The output through the client-side destroy p009 added. */
	if (probe->output != NULL)
		wl_output_destroy(probe->output);

	/* The other globals. */
	if (probe->data_device_manager != NULL)
		wl_data_device_manager_destroy(probe->data_device_manager);
	if (probe->wm_base != NULL)
		xdg_wm_base_destroy(probe->wm_base);
	if (probe->shm != NULL)
		wl_shm_destroy(probe->shm);
	if (probe->compositor != NULL)
		wl_compositor_destroy(probe->compositor);

	/* Sends the destroys before the connection ends. */
	wl_display_roundtrip(probe->display);
}

/* Makes the one-colour shared-memory buffer of the window. */
static int
probe_buffer(
	struct probe *probe,
	struct wl_buffer **buffer)
{
	struct wl_shm_pool *pool;
	char name[64];
	uint32_t *pixels;
	size_t size;
	size_t i;
	int fd;
	int error;

	/* Anonymous shared memory: a name used only until it is unlinked. */
	size = (size_t)PROBE_WIDTH * PROBE_HEIGHT * 4U;
	snprintf(name, sizeof(name), "/xdg-probe-%ld", (long)getpid());
	fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
	if (fd < 0)
		return -1;
	(void)shm_unlink(name);

	/* Sizes it. */
	error = ftruncate(fd, (off_t)size);
	if (error != 0) {
		close(fd);
		return -1;
	}

	/* Maps it. */
	pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (pixels == MAP_FAILED) {
		close(fd);
		return -1;
	}

	/* Fills every pixel with the colour. */
	for (i = 0; i < size / 4U; i++)
		pixels[i] = PROBE_COLOUR;

	/* Hands the memory to the compositor as one XRGB8888 buffer. */
	pool = wl_shm_create_pool(probe->shm, fd, (int32_t)size);
	*buffer = wl_shm_pool_create_buffer(pool, 0, PROBE_WIDTH, PROBE_HEIGHT, PROBE_WIDTH * 4,
					    WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(pool);
	munmap(pixels, size);
	close(fd);

	/* Succeeded: the buffer holds the picture. */
	return 0;
}

/* Binds the globals the probe uses, at versions both ends know. */
static void
probe_registry_global(
	void *data,
	struct wl_registry *registry,
	uint32_t name,
	const char *interface,
	uint32_t version)
{
	struct probe *probe;
	int compositor;
	int shm;
	int wm_base;
	int output;
	int data_device_manager;

	/* Compares the global's interface with the ones the probe uses. */
	probe = data;
	compositor = strcmp(interface, "wl_compositor");
	shm = strcmp(interface, "wl_shm");
	wm_base = strcmp(interface, "xdg_wm_base");
	output = strcmp(interface, "wl_output");
	data_device_manager = strcmp(interface, "wl_data_device_manager");

	/* Binds the one it is, if any. */
	if (compositor == 0) {
		probe->compositor = wl_registry_bind(registry, name, &wl_compositor_interface,
						     probe_min(version, (uint32_t)wl_compositor_interface.version));
	} else if (shm == 0) {
		probe->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (wm_base == 0) {
		probe->wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface,
						  probe_min(version, (uint32_t)xdg_wm_base_interface.version));
		xdg_wm_base_add_listener(probe->wm_base, &probe_wm_base_listener, probe);
	} else if (output == 0 && probe->output == NULL) {
		probe->output = wl_registry_bind(registry, name, &wl_output_interface,
						 probe_min(version, (uint32_t)wl_output_interface.version));
	} else if (data_device_manager == 0) {
		probe->data_device_manager = wl_registry_bind(registry, name, &wl_data_device_manager_interface,
							      probe_min(version, 3));
	}
}

/* Ignores a global that goes away; the probe's do not. */
static void
probe_registry_remove(
	void *data,
	struct wl_registry *registry,
	uint32_t name)
{
	(void)data;
	(void)registry;
	(void)name;
}

/* Answers the compositor's liveness check. */
static void
probe_wm_base_ping(
	void *data,
	struct xdg_wm_base *wm_base,
	uint32_t serial)
{
	(void)data;

	/* The pong the compositor waits for. */
	xdg_wm_base_pong(wm_base, serial);
}

/* Acknowledges a configure and notes that the window may be drawn. */
static void
probe_xdg_surface_configure(
	void *data,
	struct xdg_surface *xdg_surface,
	uint32_t serial)
{
	struct probe *probe;

	/* The acknowledgement the next commit applies. */
	probe = data;
	xdg_surface_ack_configure(xdg_surface, serial);
	probe->configured = 1;
}

/* Accepts the size the compositor suggests; the buffer keeps its own. */
static void
probe_toplevel_configure(
	void *data,
	struct xdg_toplevel *toplevel,
	int32_t width,
	int32_t height,
	struct wl_array *states)
{
	(void)data;
	(void)toplevel;
	(void)states;

	/* Records the suggestion. */
	printf("xdg-probe: toplevel configure %dx%d\n", width, height);
}

/* Ends the run early when the compositor asks the window to close. */
static void
probe_toplevel_close(
	void *data,
	struct xdg_toplevel *toplevel)
{
	struct probe *probe;

	(void)toplevel;

	/* The run loop stops at its next round. */
	probe = data;
	probe->closed = 1;
}

/* Records the bounds a version 4 compositor gives. */
static void
probe_toplevel_bounds(
	void *data,
	struct xdg_toplevel *toplevel,
	int32_t width,
	int32_t height)
{
	(void)data;
	(void)toplevel;

	/* Records the bounds. */
	printf("xdg-probe: toplevel bounds %dx%d\n", width, height);
}

/* Ignores the window manager capabilities of a version 5 compositor. */
static void
probe_toplevel_capabilities(
	void *data,
	struct xdg_toplevel *toplevel,
	struct wl_array *capabilities)
{
	(void)data;
	(void)toplevel;
	(void)capabilities;
}

/* Records the output the surface enters. */
static void
probe_surface_enter(
	void *data,
	struct wl_surface *surface,
	struct wl_output *output)
{
	(void)data;
	(void)surface;
	(void)output;

	/* Records the event. */
	printf("xdg-probe: surface entered an output\n");
}

/* Ignores the surface leaving an output. */
static void
probe_surface_leave(
	void *data,
	struct wl_surface *surface,
	struct wl_output *output)
{
	(void)data;
	(void)surface;
	(void)output;
}

/* Records the preferred buffer scale of a version 6 surface. */
static void
probe_surface_scale(
	void *data,
	struct wl_surface *surface,
	int32_t factor)
{
	(void)data;
	(void)surface;

	/* Records the event. */
	printf("xdg-probe: preferred buffer scale %d\n", factor);
}

/* Records the preferred buffer transform of a version 6 surface. */
static void
probe_surface_transform(
	void *data,
	struct wl_surface *surface,
	uint32_t transform)
{
	(void)data;
	(void)surface;

	/* Records the event. */
	printf("xdg-probe: preferred buffer transform %u\n", transform);
}

/* Notes that the compositor has shown the committed frame. */
static void
probe_frame_done(
	void *data,
	struct wl_callback *callback,
	uint32_t time)
{
	struct probe *probe;

	(void)time;

	/* The callback is single-use. */
	probe = data;
	wl_callback_destroy(callback);
	probe->frame_done = 1;
}

/* Reports the smaller of two versions. */
static uint32_t
probe_min(
	uint32_t a,
	uint32_t b)
{
	/* The first is smaller or the same. */
	if (a <= b)
		return a;

	/* Succeeded: the second is smaller. */
	return b;
}
