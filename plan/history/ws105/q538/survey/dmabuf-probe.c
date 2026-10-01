/* dmabuf-probe.c -- minimal Wayland server: wl_compositor v4 + zwp_linux_dmabuf_v1 v3 (WS105 design check). */
#define _GNU_SOURCE
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <linux/dma-buf.h>
#include <wayland-server.h>
#include "linux-dmabuf-v1-server-protocol.h"

#define FMT_ARGB8888 0x34325241
#define FMT_XRGB8888 0x34325258

static int frames_left = 3, frame_no;
static struct wl_display *dpy;

struct buf {
	int fd;
	uint32_t offset, stride, width, height, format;
	uint64_t mod;
};
struct params {
	struct buf b;
	int have;
};
struct surf {
	struct wl_resource *pending;
};

static void destroy_res(struct wl_client *c, struct wl_resource *r) { (void)c; wl_resource_destroy(r); }

/* wl_buffer */
static const struct wl_buffer_interface buffer_impl = {.destroy = destroy_res};
static void buffer_free(struct wl_resource *r)
{
	struct buf *b = wl_resource_get_user_data(r);
	close(b->fd);
	free(b);
}

/* zwp_linux_buffer_params_v1 */
static void params_add(struct wl_client *c, struct wl_resource *r, int32_t fd, uint32_t plane, uint32_t offset, uint32_t stride, uint32_t hi, uint32_t lo)
{
	(void)c;
	struct params *p = wl_resource_get_user_data(r);
	if (plane != 0 || p->have) {
		close(fd);
		wl_resource_post_error(r, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_IDX, "plane");
		return;
	}
	p->b.fd = fd, p->b.offset = offset, p->b.stride = stride, p->b.mod = (uint64_t)hi << 32 | lo, p->have = 1;
}
static struct wl_resource *make_buffer(struct wl_client *c, struct params *p, uint32_t id, int32_t w, int32_t h, uint32_t fmt)
{
	struct buf *b = malloc(sizeof *b);
	*b = p->b;
	b->width = w, b->height = h, b->format = fmt;
	p->have = 0;
	off_t size = lseek(b->fd, 0, SEEK_END);
	printf("IMPORT %dx%d fmt=0x%x mod=0x%llx offset=%u stride=%u dmabuf_size=%lld\n", w, h, fmt, (unsigned long long)b->mod, b->offset, b->stride, (long long)size);
	struct wl_resource *br = wl_resource_create(c, &wl_buffer_interface, 1, id);
	wl_resource_set_implementation(br, &buffer_impl, b, buffer_free);
	return br;
}
static void params_create(struct wl_client *c, struct wl_resource *r, int32_t w, int32_t h, uint32_t fmt, uint32_t flags)
{
	(void)flags;
	struct wl_resource *br = make_buffer(c, wl_resource_get_user_data(r), 0, w, h, fmt);
	zwp_linux_buffer_params_v1_send_created(r, br);
}
static void params_create_immed(struct wl_client *c, struct wl_resource *r, uint32_t id, int32_t w, int32_t h, uint32_t fmt, uint32_t flags)
{
	(void)flags;
	make_buffer(c, wl_resource_get_user_data(r), id, w, h, fmt);
}
static const struct zwp_linux_buffer_params_v1_interface params_impl = {
    .destroy = destroy_res, .add = params_add, .create = params_create, .create_immed = params_create_immed};
static void params_free(struct wl_resource *r) { free(wl_resource_get_user_data(r)); }

/* zwp_linux_dmabuf_v1 */
static void dmabuf_create_params(struct wl_client *c, struct wl_resource *r, uint32_t id)
{
	struct wl_resource *pr = wl_resource_create(c, &zwp_linux_buffer_params_v1_interface, wl_resource_get_version(r), id);
	wl_resource_set_implementation(pr, &params_impl, calloc(1, sizeof(struct params)), params_free);
}
static const struct zwp_linux_dmabuf_v1_interface dmabuf_impl = {.destroy = destroy_res, .create_params = dmabuf_create_params};
static void dmabuf_bind(struct wl_client *c, void *data, uint32_t version, uint32_t id)
{
	(void)data;
	struct wl_resource *r = wl_resource_create(c, &zwp_linux_dmabuf_v1_interface, version, id);
	wl_resource_set_implementation(r, &dmabuf_impl, NULL, NULL);
	uint32_t fmts[2] = {FMT_ARGB8888, FMT_XRGB8888};
	for (int i = 0; i < 2; i++) {
		zwp_linux_dmabuf_v1_send_format(r, fmts[i]);
		if (version >= ZWP_LINUX_DMABUF_V1_MODIFIER_SINCE_VERSION)
			zwp_linux_dmabuf_v1_send_modifier(r, fmts[i], 0, 0); /* DRM_FORMAT_MOD_LINEAR */
	}
}

/* wl_surface */
static void surf_attach(struct wl_client *c, struct wl_resource *r, struct wl_resource *buffer, int32_t x, int32_t y)
{
	(void)c, (void)x, (void)y;
	((struct surf *)wl_resource_get_user_data(r))->pending = buffer;
}
static void surf_damage(struct wl_client *c, struct wl_resource *r, int32_t x, int32_t y, int32_t w, int32_t h) { (void)c, (void)r, (void)x, (void)y, (void)w, (void)h; }
static void surf_frame(struct wl_client *c, struct wl_resource *r, uint32_t id)
{
	(void)r;
	struct wl_resource *cb = wl_resource_create(c, &wl_callback_interface, 1, id);
	wl_callback_send_done(cb, 0); /* immediately */
	wl_resource_destroy(cb);
}
static void surf_noop_region(struct wl_client *c, struct wl_resource *r, struct wl_resource *reg) { (void)c, (void)r, (void)reg; }
static double now_ms(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}
static void surf_commit(struct wl_client *c, struct wl_resource *r)
{
	(void)c;
	struct surf *s = wl_resource_get_user_data(r);
	if (!s->pending)
		return;
	struct buf *b = wl_resource_get_user_data(s->pending);
	struct dma_buf_export_sync_file es = {.flags = DMA_BUF_SYNC_READ, .fd = -1};
	double t0 = now_ms();
	int er = ioctl(b->fd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &es);
	if (er == 0) {
		struct pollfd p = {.fd = es.fd, .events = POLLIN};
		poll(&p, 1, 5000);
		close(es.fd);
	}
	double waited = now_ms() - t0;
	off_t size = lseek(b->fd, 0, SEEK_END);
	uint32_t pix = 0;
	uint8_t *m = mmap(NULL, size, PROT_READ, MAP_SHARED, b->fd, 0);
	if (m != MAP_FAILED) {
		pix = *(uint32_t *)(m + b->offset + (b->height / 2) * b->stride + (b->width / 2) * 4);
		munmap(m, size);
	}
	printf("PROBE frame=%d pixel=0x%08x waited_ms=%.1f export_sync=%d\n", frame_no++, pix, waited, er == 0 ? 0 : errno);
	fflush(stdout);
	wl_buffer_send_release(s->pending);
	s->pending = NULL;
	if (--frames_left == 0)
		wl_display_terminate(dpy);
}
static void surf_set_scale(struct wl_client *c, struct wl_resource *r, int32_t v) { (void)c, (void)r, (void)v; }
static const struct wl_surface_interface surf_impl = {
    .destroy = destroy_res, .attach = surf_attach, .damage = surf_damage, .frame = surf_frame, .set_opaque_region = surf_noop_region,
    .set_input_region = surf_noop_region, .commit = surf_commit, .set_buffer_transform = surf_set_scale, .set_buffer_scale = surf_set_scale,
    .damage_buffer = surf_damage};
static void surf_free(struct wl_resource *r) { free(wl_resource_get_user_data(r)); }

/* wl_region (unused but required by wl_compositor) */
static void region_rect(struct wl_client *c, struct wl_resource *r, int32_t x, int32_t y, int32_t w, int32_t h) { (void)c, (void)r, (void)x, (void)y, (void)w, (void)h; }
static const struct wl_region_interface region_impl = {.destroy = destroy_res, .add = region_rect, .subtract = region_rect};

static void comp_create_surface(struct wl_client *c, struct wl_resource *r, uint32_t id)
{
	struct wl_resource *s = wl_resource_create(c, &wl_surface_interface, wl_resource_get_version(r), id);
	wl_resource_set_implementation(s, &surf_impl, calloc(1, sizeof(struct surf)), surf_free);
	printf("SURFACE created\n");
}
static void comp_create_region(struct wl_client *c, struct wl_resource *r, uint32_t id)
{
	struct wl_resource *g = wl_resource_create(c, &wl_region_interface, wl_resource_get_version(r), id);
	wl_resource_set_implementation(g, &region_impl, NULL, NULL);
}
static const struct wl_compositor_interface comp_impl = {.create_surface = comp_create_surface, .create_region = comp_create_region};
static void comp_bind(struct wl_client *c, void *data, uint32_t version, uint32_t id)
{
	(void)data;
	struct wl_resource *r = wl_resource_create(c, &wl_compositor_interface, version, id);
	wl_resource_set_implementation(r, &comp_impl, NULL, NULL);
}

int main(int argc, char **argv)
{
	const char *sock = "wl-probe";
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--frames") && i + 1 < argc)
			frames_left = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--socket") && i + 1 < argc)
			sock = argv[++i];
	}
	setvbuf(stdout, NULL, _IOLBF, 0);
	dpy = wl_display_create();
	if (wl_display_add_socket(dpy, sock) != 0) {
		perror("wl_display_add_socket");
		return 1;
	}
	wl_global_create(dpy, &wl_compositor_interface, 4, NULL, comp_bind);
	wl_global_create(dpy, &zwp_linux_dmabuf_v1_interface, 3, NULL, dmabuf_bind);
	printf("READY socket=%s\n", sock);
	wl_display_run(dpy);
	wl_display_destroy(dpy);
	return 0;
}
