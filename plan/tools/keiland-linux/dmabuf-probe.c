/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* A bounded test compositor that observes writer fences and actual DMA-BUF pixels. */
#define _GNU_SOURCE
#include <errno.h>
#include <linux/dma-buf.h>
#include <linux/sync_file.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server.h>
#include "linux-dmabuf-v1-server-protocol.h"

/* One imported plane, owned by its wl_buffer resource until client destruction. */
struct probe_buffer {
	int fd;
	unsigned have;
	uint32_t offset;
	uint32_t stride;
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint64_t modifier;
};

/* One test surface, retaining its pending attachment and FIFO callbacks until commit. */
struct probe_surface {
	struct wl_resource *buffer;
	struct wl_list frames;
};

/* One pacing callback, owned by its resource until commit or client disconnect. */
struct probe_frame {
	struct wl_list link;
	struct wl_resource *resource;
};

/* The one test server, alive until the requested frame count or timeout. */
static struct wl_display *probe_display;

/* The finite requested workload and observed frame count, changed only by the server event loop. */
static unsigned probe_limit = 90;
static unsigned probe_count;

/* Records failed kernel or client input checks so process exit cannot claim acceptance. */
static unsigned probe_failed;

/* Size output is optional; fence and pixel observations always remain visible. */
static unsigned probe_size_log;

static void probe_destroy(struct wl_client *client, struct wl_resource *resource);
static void probe_buffer_free(struct wl_resource *resource);
static void probe_params_free(struct wl_resource *resource);
static void probe_add(struct wl_client *client, struct wl_resource *resource, int32_t fd, uint32_t plane, uint32_t offset, uint32_t stride, uint32_t high, uint32_t low);
static struct wl_resource *probe_buffer_create(struct wl_client *client, struct wl_resource *resource, uint32_t id, int32_t width, int32_t height, uint32_t format);
static void probe_create(struct wl_client *client, struct wl_resource *resource, int32_t width, int32_t height, uint32_t format, uint32_t flags);
static void probe_create_immed(struct wl_client *client, struct wl_resource *resource, uint32_t id, int32_t width, int32_t height, uint32_t format, uint32_t flags);
static void probe_params(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void probe_bind_dmabuf(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static void probe_attach(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer, int32_t x, int32_t y);
static void probe_damage(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
static void probe_frame_free(struct wl_resource *resource);
static void probe_frame_create(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void probe_region_set(struct wl_client *client, struct wl_resource *resource, struct wl_resource *region);
static void probe_transform(struct wl_client *client, struct wl_resource *resource, int32_t transform);
static void probe_scale(struct wl_client *client, struct wl_resource *resource, int32_t scale);
static void probe_commit(struct wl_client *client, struct wl_resource *resource);
static void probe_surface_free(struct wl_resource *resource);
static void probe_surface_create(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void probe_region_create(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void probe_bind_compositor(struct wl_client *client, void *data, uint32_t version, uint32_t id);
static int probe_timeout(void *data);
static int probe_observe(struct probe_buffer *buffer);
static uint64_t probe_time(void);
static int probe_initialization_failed(void);

/* Runs a finite test server and returns failure for malformed imports or an incomplete frame sequence. */
int
main(
	int argc,
	char **argv)
{
	const char *socket;
	unsigned timeout;
	int index;
	int error;
	struct wl_global *global;
	struct wl_event_source *timer;
	int evaluated;

	/* Parses the same finite workload options used by the Queue acceptance commands. */
	socket = NULL;
	timeout = 60;
	for (index = 1; index < argc; index++) {
		/* Size logging does not change protocol behavior. */
		evaluated = strcmp(argv[index], "--size-log");
		if (evaluated == 0) {
			probe_size_log = 1;
			continue;
		}

		/* Every remaining option requires exactly one argument. */
		if (index + 1 >= argc)
			return 2;

		/* Selects the private absolute socket path. */
		evaluated = strcmp(argv[index], "--socket");
		if (evaluated == 0) {
			socket = argv[++index];
			continue;
		}

		/* Selects a bounded positive frame count. */
		evaluated = strcmp(argv[index], "--frames");
		if (evaluated == 0) {
			probe_limit = (unsigned)strtoul(argv[++index], NULL, 10);
			continue;
		}

		/* Selects the finite server deadline in seconds. */
		evaluated = strcmp(argv[index], "--timeout");
		if (evaluated == 0) {
			timeout = (unsigned)strtoul(argv[++index], NULL, 10);
			continue;
		}

		/* Unknown options cannot silently alter an acceptance run. */
		return 2;
	}

	/* Requires finite practical values before opening the socket. */
	if (socket == NULL)
		return 2;

	/* Zero frames supplies no acceptance evidence. */
	if (probe_limit == 0)
		return 2;

	/* Timeout arithmetic must fit the event loop's signed milliseconds. */
	if (timeout > 600)
		return 2;

	/* A zero-second deadline is not a valid test run. */
	if (timeout == 0)
		return 2;

	/* Flushes each observation so failures retain the preceding evidence. */
	(void)setvbuf(stdout, NULL, _IOLBF, 0);
	probe_display = wl_display_create();
	if (probe_display == NULL)
		return 1;

	/* Uses a private test socket without any host compositor connection. */
	error = wl_display_add_socket(probe_display, socket);
	if (error != 0)
		return probe_initialization_failed();

	/* Advertises core v4 surface requests and version-three DMA-BUF modifier events. */
	global = wl_global_create(probe_display, &wl_compositor_interface, 4, NULL, probe_bind_compositor);
	if (global == NULL)
		return probe_initialization_failed();

	/* The DMA-BUF global is deliberately independent of system Vulkan's WSI. */
	global = wl_global_create(probe_display, &zwp_linux_dmabuf_v1_interface, 3, NULL, probe_bind_dmabuf);
	if (global == NULL)
		return probe_initialization_failed();

	/* The event loop deadline prevents an absent or stalled client from hanging the test. */
	timer = wl_event_loop_add_timer(wl_display_get_event_loop(probe_display), probe_timeout, NULL);
	if (timer == NULL)
		return probe_initialization_failed();

	/* Arms the one finite test deadline before accepting clients. */
	error = wl_event_source_timer_update(timer, (int)timeout * 1000);
	if (error != 0)
		return probe_initialization_failed();

	/* Runs until all expected commits arrive or a checked operation fails. */
	wl_display_run(probe_display);
	(void)wl_event_source_remove(timer);
	wl_display_destroy_clients(probe_display);
	wl_display_destroy(probe_display);
	printf("RESULT frames=%u failed=%u\n", probe_count, probe_failed);
	if (probe_failed != 0)
		return 1;

	/* Incomplete observations cannot clear the Phase. */
	if (probe_count != probe_limit)
		return 1;

	/* Succeeded: every requested frame was observed from its actual exported image. */
	return 0;
}

/* Handles core and extension protocol resource destruction. */
static void
probe_destroy(
	struct wl_client *client,
	struct wl_resource *resource)
{
	/* Resource destructors release their independently owned descriptors and callback data. */
	(void)client;
	wl_resource_destroy(resource);
}

/* Releases the descriptor imported into one compositor buffer. */
static void
probe_buffer_free(
	struct wl_resource *resource)
{
	struct probe_buffer *buffer;

	/* A buffer owns exactly one received descriptor until client destruction. */
	buffer = wl_resource_get_user_data(resource);
	(void)close(buffer->fd);
	free(buffer);
}

/* Releases a parameter descriptor only when ownership was never transferred into a buffer. */
static void
probe_params_free(
	struct wl_resource *resource)
{
	struct probe_buffer *params;

	/* An unused or failed immediate creation leaves descriptor ownership on the parameter resource. */
	params = wl_resource_get_user_data(resource);
	if (params->have != 0)
		(void)close(params->fd);

	/* Releases the short-lived plane description after its destructor. */
	free(params);
}

/* Accepts exactly one plane and retains the received DMA-BUF descriptor. */
static void
probe_add(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t fd,
	uint32_t plane,
	uint32_t offset,
	uint32_t stride,
	uint32_t high,
	uint32_t low)
{
	struct probe_buffer *params;

	/* The acceptance server exercises the one-plane WSI contract only. */
	(void)client;
	params = wl_resource_get_user_data(resource);
	if (plane != 0) {
		(void)close(fd);
		wl_resource_post_error(resource, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_IDX, "one plane required");
		return;
	}

	/* Duplicate plane requests are invalid protocol inputs. */
	if (params->have != 0) {
		(void)close(fd);
		wl_resource_post_error(resource, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_IDX, "one plane required");
		return;
	}

	/* Transfers received descriptor ownership to this parameter resource. */
	params->fd = fd;
	params->offset = offset;
	params->stride = stride;
	params->modifier = ((uint64_t)high << 32) | low;
	params->have = 1;

	/* Succeeded: immediate creation may now consume this plane description. */
	return;
}

/* Validates immediate construction and transfers the plane to a compositor buffer resource. */
static struct wl_resource *
probe_buffer_create(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	int32_t width,
	int32_t height,
	uint32_t format)
{
	/* Buffer resource requests are implemented only for descriptor lifetime and release observations. */
	static const struct wl_buffer_interface probe_buffer_impl = {probe_destroy};
	struct probe_buffer *params;
	struct probe_buffer *buffer;
	struct wl_resource *created;

	/* The CPU pixel observer understands linear ARGB and XRGB images only. */
	params = wl_resource_get_user_data(resource);
	if (params->have == 0)
		return NULL;

	/* Positive dimensions are required for a bounded mapping. */
	if (width <= 0)
		return NULL;

	/* The supplied height likewise must describe a real image. */
	if (height <= 0)
		return NULL;

	/* This probe deliberately rejects modifiers it cannot map directly. */
	if (params->modifier != 0)
		return NULL;

	/* Requires the exact little-endian channel ordering used by the color test. */
	if (format != 0x34325241U) {
		if (format != 0x34325258U)
			return NULL;
	}

	/* Each row must contain every four-byte pixel. */
	if ((uint64_t)(uint32_t)width * 4 > params->stride)
		return NULL;

	/* Allocates the callback record before transferring descriptor ownership. */
	buffer = malloc(sizeof(*buffer));
	if (buffer == NULL)
		return NULL;

	/* The resource identity is allocated by the immediate constructor request. */
	created = wl_resource_create(client, &wl_buffer_interface, 1, id);
	if (created == NULL) {
		free(buffer);
		return NULL;
	}

	/* Transfers exactly one descriptor from the plane parameters to the buffer. */
	*buffer = *params;
	buffer->width = (uint32_t)width;
	buffer->height = (uint32_t)height;
	buffer->format = format;
	params->have = 0;
	wl_resource_set_implementation(created, &probe_buffer_impl, buffer, probe_buffer_free);

	/* Reports the actual layout as durable V3 acceptance evidence. */
	printf("IMPORT width=%u height=%u modifier=0x%llx offset=%u stride=%u\n", buffer->width, buffer->height, (unsigned long long)buffer->modifier, buffer->offset, buffer->stride);

	/* Succeeded: the resource owns the imported image descriptor. */
	return created;
}

/* Supports asynchronous creation for protocol completeness, though the WSI uses create_immed. */
static void
probe_create(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t width,
	int32_t height,
	uint32_t format,
	uint32_t flags)
{
	struct wl_resource *buffer;

	/* Unimplemented flags cannot change the mapped pixel interpretation. */
	if (flags != 0) {
		zwp_linux_buffer_params_v1_send_failed(resource);
		return;
	}

	/* Sends either the created buffer or the protocol's explicit failure event. */
	buffer = probe_buffer_create(client, resource, 0, width, height, format);
	if (buffer == NULL) {
		zwp_linux_buffer_params_v1_send_failed(resource);
		return;
	}

	/* Transfers the new server-selected buffer identity to the client. */
	zwp_linux_buffer_params_v1_send_created(resource, buffer);
}

/* Implements the constructor used by all WSI acceptance runs. */
static void
probe_create_immed(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id,
	int32_t width,
	int32_t height,
	uint32_t format,
	uint32_t flags)
{
	struct wl_resource *buffer;

	/* No flag may silently alter the expected color byte order. */
	buffer = NULL;
	if (flags == 0)
		buffer = probe_buffer_create(client, resource, id, width, height, format);

	/* Immediate construction failure is a protocol error, never an unobserved frame. */
	if (buffer == NULL)
		wl_resource_post_error(resource, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INVALID_WL_BUFFER, "invalid image");
}

/* Constructs one short-lived plane parameter resource. */
static void
probe_params(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id)
{
	/* Only version-three single-plane construction is relevant to this bounded server. */
	static const struct zwp_linux_buffer_params_v1_interface probe_params_impl = {
	    probe_destroy, probe_add, probe_create, probe_create_immed};
	struct wl_resource *created;
	struct probe_buffer *params;

	/* Allocates callback storage before creating the protocol identity. */
	params = calloc(1, sizeof(*params));
	if (params == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	/* Parameter resources inherit the version-three bind. */
	created = wl_resource_create(client, &zwp_linux_buffer_params_v1_interface, wl_resource_get_version(resource), id);
	if (created == NULL) {
		free(params);
		wl_client_post_no_memory(client);
		return;
	}

	/* Owns plane data until immediate creation or parameter destruction. */
	wl_resource_set_implementation(created, &probe_params_impl, params, probe_params_free);
}

/* Advertises modifier-only format pairs so the WSI cannot rely on legacy format events. */
static void
probe_bind_dmabuf(
	struct wl_client *client,
	void *data,
	uint32_t version,
	uint32_t id)
{
	/* Version four feedback requests are never advertised by the version-three global. */
	static const struct zwp_linux_dmabuf_v1_interface probe_dmabuf_impl = {
	    probe_destroy, probe_params, NULL, NULL};
	struct wl_resource *resource;

	/* The server advertises only version three. */
	(void)data;
	resource = wl_resource_create(client, &zwp_linux_dmabuf_v1_interface, (int)version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	/* Sends only the pairs supported by this CPU mapping observer. */
	wl_resource_set_implementation(resource, &probe_dmabuf_impl, NULL, NULL);
	zwp_linux_dmabuf_v1_send_modifier(resource, 0x34325241U, 0, 0);
	zwp_linux_dmabuf_v1_send_modifier(resource, 0x34325258U, 0, 0);
}

/* Stores a pending attachment until the client's next commit. */
static void
probe_attach(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *buffer,
	int32_t x,
	int32_t y)
{
	struct probe_surface *surface;

	/* Attach offsets do not affect the buffer's actual center pixel. */
	(void)client;
	(void)x;
	(void)y;
	surface = wl_resource_get_user_data(resource);
	surface->buffer = buffer;
}

/* Damage and region coordinates have no effect on the full-image probe. */
static void
probe_damage(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t x,
	int32_t y,
	int32_t width,
	int32_t height)
{
	/* The observer always reads a deterministic pixel in the committed image. */
	(void)client;
	(void)resource;
	(void)x;
	(void)y;
	(void)width;
	(void)height;
}

/* Removes callback storage from its surface list on either done or client disconnect. */
static void
probe_frame_free(
	struct wl_resource *resource)
{
	struct probe_frame *frame;

	/* Callback destruction preserves the owning surface's intrusive list. */
	frame = wl_resource_get_user_data(resource);
	wl_list_remove(&frame->link);
	free(frame);
}

/* Queues one frame completion for the next commit. */
static void
probe_frame_create(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id)
{
	struct probe_surface *surface;
	struct probe_frame *frame;

	/* Allocates callback storage before the Wayland resource. */
	surface = wl_resource_get_user_data(resource);
	frame = calloc(1, sizeof(*frame));
	if (frame == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	/* The core callback has no requests and lives until done. */
	frame->resource = wl_resource_create(client, &wl_callback_interface, 1, id);
	if (frame->resource == NULL) {
		free(frame);
		wl_client_post_no_memory(client);
		return;
	}

	/* Resource destruction always unlinks the retained frame. */
	wl_list_insert(surface->frames.prev, &frame->link);
	wl_resource_set_implementation(frame->resource, NULL, frame, probe_frame_free);
}

/* Accepts ordinary region requests without changing full-image inspection. */
static void
probe_region_set(
	struct wl_client *client,
	struct wl_resource *resource,
	struct wl_resource *region)
{
	/* This test surface has no visible clipping or input interaction. */
	(void)client;
	(void)resource;
	(void)region;
}

/* Accepts only the test client's default transform semantics. */
static void
probe_transform(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t transform)
{
	/* The pixel test uses identity transform and inspects storage before any display transform. */
	(void)client;
	(void)resource;
	(void)transform;
}

/* Accepts the core scale request without resampling the underlying image storage. */
static void
probe_scale(
	struct wl_client *client,
	struct wl_resource *resource,
	int32_t scale)
{
	/* Storage dimensions, rather than logical surface dimensions, are the size acceptance evidence. */
	(void)client;
	(void)resource;
	(void)scale;
}

/* Observes the committed buffer, then releases it and completes pending pacing callbacks. */
static void
probe_commit(
	struct wl_client *client,
	struct wl_resource *resource)
{
	struct probe_surface *surface;
	struct probe_frame *frame;
	int error;

	/* A null attachment contains no image frame to observe. */
	surface = wl_resource_get_user_data(resource);
	if (surface->buffer == NULL)
		return;

	/* A failed fence wait or invalid mapping prevents any successful test outcome. */
	error = probe_observe(wl_resource_get_user_data(surface->buffer));
	if (error != 0) {
		probe_failed = 1;
		wl_display_terminate(probe_display);
		return;
	}

	/* The compositor releases only after its CPU read and read-side synchronization have finished. */
	wl_buffer_send_release(surface->buffer);
	surface->buffer = NULL;
	for (;;) {
		/* An empty callback list finishes the resource retirement pass. */
		if (surface->frames.next == &surface->frames)
			break;

		/* Each callback is done exactly once for the committed frame. */
		frame = wl_container_of(surface->frames.next, frame, link);
		wl_callback_send_done(frame->resource, (uint32_t)(probe_time() / 1000000));
		wl_resource_destroy(frame->resource);
	}

	/* Sends the final release before terminating so the client can cleanly finish. */
	wl_client_flush(client);
	if (probe_count >= probe_limit)
		wl_display_terminate(probe_display);
}

/* Retires queued frame resources before removing their owning surface. */
static void
probe_surface_free(
	struct wl_resource *resource)
{
	struct probe_surface *surface;
	struct probe_frame *frame;

	/* A disconnected client may retain pacing callbacks that never reached a commit. */
	surface = wl_resource_get_user_data(resource);
	for (;;) {
		/* An empty callback list finishes the resource retirement pass. */
		if (surface->frames.next == &surface->frames)
			break;

		/* Resource destruction unlinks callback data from this still-live surface. */
		frame = wl_container_of(surface->frames.next, frame, link);
		wl_resource_destroy(frame->resource);
	}

	/* No callback now refers to this surface's list head. */
	free(surface);
}

/* Constructs one test surface with version-four core requests. */
static void
probe_surface_create(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id)
{
	/* The v4 core surface requests used by the WSI; later requests remain unavailable. */
	static const struct wl_surface_interface probe_surface_impl = {
	    probe_destroy, probe_attach, probe_damage, probe_frame_create,
	    probe_region_set, probe_region_set, probe_commit, probe_transform,
	    probe_scale, probe_damage, NULL};
	struct probe_surface *surface;
	struct wl_resource *created;

	/* Allocates stable callback storage and initializes its pacing list. */
	surface = calloc(1, sizeof(*surface));
	if (surface == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	/* The core surface cannot outlive its resource-owned private state. */
	wl_list_init(&surface->frames);
	created = wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(resource), id);
	if (created == NULL) {
		free(surface);
		wl_client_post_no_memory(client);
		return;
	}

	/* Every surface request sees this one private attachment and pacing list. */
	wl_resource_set_implementation(created, &probe_surface_impl, surface, probe_surface_free);
}

/* Constructs a no-clipping region for ordinary core client compatibility. */
static void
probe_region_create(
	struct wl_client *client,
	struct wl_resource *resource,
	uint32_t id)
{
	/* Regions are sufficient to accept ordinary core client setup, though the probe never clips pixels. */
	static const struct wl_region_interface probe_region_impl = {
	    probe_destroy, probe_damage, probe_damage};
	struct wl_resource *created;

	/* Regions own no image data in this test compositor. */
	(void)resource;
	created = wl_resource_create(client, &wl_region_interface, 1, id);
	if (created == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	/* Installs ordinary add/subtract/destroy requests without private storage. */
	wl_resource_set_implementation(created, &probe_region_impl, NULL, NULL);
}

/* Binds the independent test compositor's core global. */
static void
probe_bind_compositor(
	struct wl_client *client,
	void *data,
	uint32_t version,
	uint32_t id)
{
	/* The test compositor constructs private surface and region resources only. */
	static const struct wl_compositor_interface probe_compositor_impl = {
	    probe_surface_create, probe_region_create};
	struct wl_resource *resource;

	/* Each bound global only owns its protocol identity. */
	(void)data;
	resource = wl_resource_create(client, &wl_compositor_interface, (int)version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	/* Child requests allocate the buffer-independent surface records. */
	wl_resource_set_implementation(resource, &probe_compositor_impl, NULL, NULL);
}

/* Ends a stalled or incomplete acceptance run without claiming success. */
static int
probe_timeout(
	void *data)
{
	/* The deadline belongs to this single finite test workload. */
	(void)data;
	probe_failed = 1;
	wl_display_terminate(probe_display);

	/* The event-loop callback itself was handled. */
	return 0;
}

/* Measures writer fences, waits for completion, and reads the actual committed image's center pixel. */
static int
probe_observe(
	struct probe_buffer *buffer)
{
	struct dma_buf_export_sync_file export;
	struct dma_buf_sync sync;
	struct sync_file_info information;
	struct sync_fence_info *details;
	uint32_t fence_index;
	struct pollfd descriptor;
	void *mapping;
	off_t size;
	uint64_t position;
	uint64_t start;
	uint64_t waited;
	uint32_t pixel;
	int error;

	/* Exports writer completion only; fence counts prove implicit-sync payload transfer. */
	memset(&export, 0, sizeof(export));
	export.flags = DMA_BUF_SYNC_READ;
	error = ioctl(buffer->fd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &export);
	if (error != 0)
		return -1;

	/* Queries the number of kernel fences without pretending a completed payload has vanished. */
	memset(&information, 0, sizeof(information));
	error = ioctl(export.fd, SYNC_IOC_FILE_INFO, &information);
	if (error != 0) {
		(void)close(export.fd);
		return -1;
	}

	/* Reads actual timeline identities to distinguish payloads from the kernel's empty-reservation stub. */
	details = calloc((size_t)information.num_fences + 1, sizeof(*details));
	if (details == NULL) {
		(void)close(export.fd);
		return -1;
	}

	/* Queries the measured fence records before the completion wait. */
	information.sync_fence_info = (uint64_t)(uintptr_t)details;
	error = ioctl(export.fd, SYNC_IOC_FILE_INFO, &information);
	if (error != 0) {
		free(details);
		(void)close(export.fd);
		return -1;
	}

	/* Retains raw fence identities so the evidence does not confuse an empty reservation with a writer. */
	for (fence_index = 0; fence_index < information.num_fences; fence_index++) {
		/* Includes the driver, timeline and completion status supplied by the kernel. */
		printf("FENCE frame=%u driver=%s timeline=%s status=%d\n", probe_count, details[fence_index].driver_name, details[fence_index].obj_name, details[fence_index].status);
	}

	/* Releases the temporary detailed query before mapping the buffer. */
	free(details);

	/* Measures a finite writer wait; CPU lavapipe normally completes before this observation. */
	start = probe_time();
	descriptor.fd = export.fd;
	descriptor.events = POLLIN;
	descriptor.revents = 0;
	error = poll(&descriptor, 1, 5000);
	waited = (probe_time() - start) / 1000000;
	(void)close(export.fd);
	if (error <= 0)
		return -1;

	/* A malformed or failed sync descriptor must not allow an unprotected image read. */
	if ((descriptor.revents & POLLIN) == 0)
		return -1;

	/* Validates the center pixel lies inside the actual DMA-BUF allocation. */
	size = lseek(buffer->fd, 0, SEEK_END);
	if (size <= 0)
		return -1;

	/* Uses 64-bit arithmetic for compositor-controlled dimensions and strides. */
	position = buffer->offset + (uint64_t)(buffer->height / 2) * buffer->stride + (uint64_t)(buffer->width / 2) * 4;
	if (position + 4 > (uint64_t)size)
		return -1;

	/* Begins CPU read synchronization before touching externally written memory. */
	memset(&sync, 0, sizeof(sync));
	sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ;
	error = ioctl(buffer->fd, DMA_BUF_IOCTL_SYNC, &sync);
	if (error != 0)
		return -1;

	/* Maps only the buffer allocation and never an unrelated host device. */
	mapping = mmap(NULL, (size_t)size, PROT_READ, MAP_SHARED, buffer->fd, 0);
	if (mapping == MAP_FAILED) {
		sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ;
		(void)ioctl(buffer->fd, DMA_BUF_IOCTL_SYNC, &sync);
		return -1;
	}

	/* Copies the observed center pixel without alignment assumptions. */
	memcpy(&pixel, (unsigned char *)mapping + (size_t)position, sizeof(pixel));
	(void)munmap(mapping, (size_t)size);
	sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ;
	error = ioctl(buffer->fd, DMA_BUF_IOCTL_SYNC, &sync);
	if (error != 0)
		return -1;

	/* Frame numbering starts at zero so the deterministic color pattern is N modulo three. */
	printf("PROBE frame=%u pixel=0x%08x fences=%u waited_ms=%llu", probe_count, pixel, information.num_fences, (unsigned long long)waited);
	if (probe_size_log != 0)
		printf(" width=%u height=%u", buffer->width, buffer->height);

	/* Completes the observation line before accepting the next frame. */
	printf("\n");
	probe_count++;

	/* Succeeded: the actual image pixel was read only after its writer completed. */
	return 0;
}

/* Uses a monotonic timebase for writer wait evidence and callback timestamps. */
static uint64_t
probe_time(
	void)
{
	struct timespec now;
	int error;

	/* A failed clock supplies no meaningful latency evidence. */
	error = clock_gettime(CLOCK_MONOTONIC, &now);
	if (error != 0)
		return 0;

	/* Returns nanoseconds without wall-clock adjustments. */
	return (uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec;
}

/* Retires the test display after any partial global or socket setup failure. */
static int
probe_initialization_failed(
	void)
{
	/* Partial initialization owns no externally persistent artifacts. */
	wl_display_destroy(probe_display);

	/* Reports server initialization failure without claiming frame evidence. */
	return 1;
}
