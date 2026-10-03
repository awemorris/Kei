/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The System Monitor's window and drawing (plan/ws134/design.md sections
 * 2 to 4): the glyph atlas the text is drawn from, the list of shapes a
 * frame is made of, the Vulkan renderer that draws it, the screen's layout,
 * and the application's state.
 */

#ifndef MONITOR_APP_H
#define MONITOR_APP_H

#define VK_USE_PLATFORM_WAYLAND_KHR 1
#include <vulkan/vulkan.h>
#include <wayland-client.h>
#include <keiland.h>
#include <keiui.h>

#include "monitor.h"

/* Marks a parameter a function does not use. */
#define UNUSED_PARAMETER(name)	((void)(name))

/* A vertex: four vec4s (position and the shape's local point, the shape, the colour, the second colour). */
#define SM_VERTEX_FLOATS	16U

/* The shapes the fragment shader draws (the shape's fourth float). */
#define SM_SHAPE_SOLID		0.0f
#define SM_SHAPE_PLATE		1.0f
#define SM_SHAPE_LINE		2.0f
#define SM_SHAPE_GRADIENT	3.0f
#define SM_SHAPE_AREA		4.0f
#define SM_SHAPE_DISC		5.0f

/* The pipelines: shapes, and glyphs from the atlas. */
#define SM_PIPE_SHAPE		0U
#define SM_PIPE_GLYPH		1U
#define SM_PIPES		2U

/* The text styles the atlas holds (design.md section 2.4). */
enum sm_style {
	SM_STYLE_TITLE,
	SM_STYLE_NOTE,
	SM_STYLE_SMALL,
	SM_STYLE_VALUE,
	SM_STYLE_VALUE_SMALL,
	SM_STYLE_STATE,
	SM_STYLES
};

/* The characters the atlas holds: printable ASCII. */
#define SM_GLYPH_FIRST		32U
#define SM_GLYPH_LAST		126U
#define SM_GLYPH_COUNT		(SM_GLYPH_LAST - SM_GLYPH_FIRST + 1U)

/* One glyph's cell in the atlas and how far the pen moves past it. */
struct sm_glyph {
	int x;
	int y;
	int width;
	int height;
	int advance;
};

/*
 * The atlas: every style's glyphs drawn once on the CPU, white with their
 * coverage as alpha, uploaded once (design.md section 4.1).  Each glyph's
 * cell is the pen's advance wide and the line high, with a margin, and is
 * drawn with its top at the baseline less the ascent.
 */
struct sm_atlas {
	uint32_t *pixels;
	int width;
	int height;
	float scale;
	int ascent[SM_STYLES];
	int line[SM_STYLES];
	struct sm_glyph glyphs[SM_STYLES][SM_GLYPH_COUNT];
	unsigned long serial;
};

/* One draw of a frame: a pipeline and a run of vertices. */
struct sm_draw {
	unsigned pipe;
	uint32_t first;
	uint32_t count;
};

/* A frame's shapes: the vertices and the draws over them, in order. */
struct sm_scene {
	float *vertices;
	size_t vertex_count;
	size_t vertex_capacity;
	struct sm_draw *draws;
	size_t draw_count;
	size_t draw_capacity;
};

/* One swapchain image's objects. */
struct sm_target {
	VkImage image;
	VkImageView view;
	VkFramebuffer framebuffer;
	VkSemaphore rendered;
};

/*
 * The renderer: the device, the swapchain and its targets, the pass and
 * the pipelines, the atlas's image, and the vertex buffer.  One frame at
 * a time: each is waited for before the next, so the host writes the
 * vertices without further synchronization (as Notes does).
 */
struct sm_renderer {
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical;
	VkDevice device;
	VkQueue queue;
	uint32_t family;
	VkPhysicalDeviceMemoryProperties memory;
	char device_name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
	VkSwapchainKHR swapchain;
	VkFormat format;
	VkExtent2D extent;
	struct sm_target *targets;
	uint32_t count;
	VkRenderPass pass;
	VkDescriptorSetLayout set_layout;
	VkPipelineLayout layout;
	VkPipeline pipes[SM_PIPES];
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet set;
	VkSampler sampler;
	VkImage atlas;
	VkDeviceMemory atlas_memory;
	VkImageView atlas_view;
	unsigned long atlas_serial;
	int atlas_ready;
	VkBuffer vertices;
	VkDeviceMemory vertex_memory;
	void *vertex_map;
	size_t vertex_capacity;
	VkCommandPool pool;
	VkCommandBuffer command;
	VkFence fence;
	VkSemaphore acquired;
	uint64_t allocated_bytes;
	const char *operation;
};

/* A plate's place on the screen, in pixels. */
struct sm_box {
	float x;
	float y;
	float width;
	float height;
};

/* The plates of the screen (design.md section 2.3). */
enum sm_plate {
	SM_PLATE_CPU,
	SM_PLATE_GPU,
	SM_PLATE_MEMORY,
	SM_PLATE_NETWORK,
	SM_PLATE_DISK,
	SM_PLATE_CORES,
	SM_PLATE_STATE,
	SM_PLATE_GRAPHICS,
	SM_PLATE_FLOW,
	SM_PLATE_STRATA,
	SM_PLATE_LANES,
	SM_PLATE_LATENCY,
	SM_PLATE_EVENTS,
	SM_PLATES
};

/* The layout: the window's size, the scales, the header's band and each plate's box. */
struct sm_layout {
	float width;
	float height;
	float scale;
	struct sm_box header;
	struct sm_box plates[SM_PLATES];
};

/* An event the Events strip lists (design.md section 3.8). */
#define SM_EVENTS_MAX		200U
struct sm_event {
	uint64_t time_ms;
	enum sm_level level;
	char text[80];
};

/*
 * A value's slide (design.md section 3.1): the text shown, the one before
 * it, when it changed, and whether it rose (1) or fell (-1).
 */
struct sm_slide {
	char text[48];
	char old[48];
	uint64_t changed_ms;
	int direction;
};

/*
 * The motion's state (design.md sections 3.1 and 3.2): the pointer's
 * place (from -1 to 1 across the window), the camera's tilt following it
 * on a spring, the core's measures following the values, and the phases
 * of what turns and flows, advanced by the time between frames.
 */
struct sm_motion {
	uint64_t last_ms;
	float pointer_x;
	float pointer_y;
	float tilt_x;
	float tilt_y;
	float tilt_speed_x;
	float tilt_speed_y;
	float cpu;
	float gpu;
	float memory;
	float network;
	float disk;
	float level;
	float ring_phase;
	float orbit_phase;
	float rx_phase;
	float tx_phase;
	float read_phase;
	float write_phase;
};

/* The time ranges the graphs show (the titlebar's controls). */
#define SM_RANGES		4U

/* The application. */
struct sm_app {
	/* The options. */
	const char *replay_path;
	uint64_t seed;
	unsigned period_ms;
	unsigned fps;
	uint32_t width;
	uint32_t height;
	unsigned cpus;
	unsigned gpus;
	int calm;
	int fixed_clock;
	uint64_t fixed_ms;
	uint64_t timeout_ms;
	uint64_t clock_offset_ms;
	const char *token;

	/* The window, its titlebar, and the frame callback that allows the next frame. */
	struct kui_window *window;
	struct wl_display *display;
	struct wl_surface *surface;
	struct keiland_titlebar *titlebar;
	struct wl_callback *frame_callback;
	int frame_allowed;
	int visible;
	uint64_t frame_asked_ms;
	int quit;
	int dirty;

	/* The data: the source, the last frame, the history, the rules and the events. */
	struct sm_source source;
	struct sm_frame frame;
	int have_frame;
	struct sm_history history;
	struct sm_rules rules;
	enum sm_level level;
	struct sm_event events[SM_EVENTS_MAX];
	unsigned event_count;
	uint64_t frame_at_ms;
	unsigned range;

	/* The drawing. */
	struct kui_text sans;
	struct kui_text mono;
	struct sm_atlas atlas;
	struct sm_scene scene;
	struct sm_renderer renderer;
	struct sm_layout layout;
	struct sm_layout view;
	struct sm_motion motion;
	struct sm_slide slides[SM_PLATES];
	char shown[SM_PLATES][48];

	/* The clock and the reports. */
	uint64_t start_us;
	uint64_t frames;
	uint64_t next_frame_us;
	uint64_t frame_wait_us;
	uint64_t report_ms;
	uint64_t memory_report_ms;
};

/* atlas.c */
int sm_atlas_build(struct sm_atlas *atlas, struct kui_text *sans, struct kui_text *mono, float scale);
void sm_atlas_release(struct sm_atlas *atlas);
float sm_atlas_width(const struct sm_atlas *atlas, enum sm_style style, const char *text);

/* scene.c */
void sm_scene_clear(struct sm_scene *scene);
void sm_scene_release(struct sm_scene *scene);
int sm_scene_build(struct sm_app *app, uint64_t now_ms);
void sm_layout_compute(struct sm_layout *layout, float width, float height);

/* space.c */
void sm_motion_update(struct sm_app *app, uint64_t now_ms);
void sm_space_core(struct sm_app *app, const struct sm_box *box, uint64_t now_ms);
void sm_space_relief(struct sm_app *app, const struct sm_box *box);

/* render.c */
VkResult sm_renderer_open(struct sm_renderer *renderer, struct wl_display *display, struct wl_surface *surface, uint32_t width, uint32_t height);
VkResult sm_renderer_resize(struct sm_renderer *renderer, uint32_t width, uint32_t height);
VkResult sm_renderer_recover(struct sm_renderer *renderer, struct wl_display *display, struct wl_surface *surface, uint32_t width, uint32_t height);
VkResult sm_renderer_atlas(struct sm_renderer *renderer, const struct sm_atlas *atlas);
VkResult sm_renderer_draw(struct sm_renderer *renderer, const struct sm_scene *scene, uint64_t *wait_us);
void sm_renderer_close(struct sm_renderer *renderer);

#endif
