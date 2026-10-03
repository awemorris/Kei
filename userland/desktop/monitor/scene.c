/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The System Monitor's screen (design.md sections 2 and 3): the layout of
 * the plates, and a frame's shapes and glyphs built from the latest values,
 * the history and the state.
 *
 * ws134-p002 draws the plates flat, in their layers' tones: the summary
 * row, the CPU's tiles, the state's nested squares, the GPU cards, the
 * network's and the disks' graphs, the memory's strata and the events.
 * The 3D core, the relief and the motion come with ws134-p003.
 *
 * The layout is made in a logical 1280x800 window and stretched to the
 * real one; the text is drawn at the smaller of the two scales, so that it
 * never overflows its plate.
 */

#include "app.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* The logical window the layout is made in. */
#define SCENE_WIDTH		1280.0f
#define SCENE_HEIGHT		800.0f

/* The longest graph, in samples, and the points of a sparkline. */
#define SCENE_POINTS		SM_HISTORY_MAX

/* A colour as four floats. */
struct scene_color {
	float r;
	float g;
	float b;
	float a;
};

/* The colour tokens (design.md section 2.4). */
#define TOKEN_BG_DEEP		0x10151eU
#define TOKEN_BG_MID		0x181f2bU
#define TOKEN_SURFACE_0		0x1b232fU
#define TOKEN_SURFACE_1		0x1f2836U
#define TOKEN_SURFACE_2		0x26303fU
#define TOKEN_EDGE_0		0x303d4eU
#define TOKEN_EDGE_1		0x405268U
#define TOKEN_EDGE_2		0x5c7692U
#define TOKEN_TEXT		0xd6e0ecU
#define TOKEN_TEXT_DIM		0x8a9ab0U
#define TOKEN_TEXT_FAINT	0x5e6d82U
#define TOKEN_CYAN		0x5cc4e6U
#define TOKEN_ICE		0xaad6f0U
#define TOKEN_MINT		0x6ed6b0U
#define TOKEN_AMBER		0xe6a84cU
#define TOKEN_CORAL		0xd8664eU

/* The time ranges of the graphs, in samples, and their names. */
static const unsigned scene_ranges[SM_RANGES] = { 60U, 300U, 900U, 3600U };

/* The layers' surfaces and edges, from the back (0) to the front (2). */
static const uint32_t layer_surface[3] = { TOKEN_SURFACE_0, TOKEN_SURFACE_1, TOKEN_SURFACE_2 };
static const uint32_t layer_edge[3] = { TOKEN_EDGE_0, TOKEN_EDGE_1, TOKEN_EDGE_2 };

static struct scene_color color_of(uint32_t rgb, float alpha);
static struct scene_color color_mix(struct scene_color from, struct scene_color to, float amount);
static int scene_reserve(struct sm_scene *scene, size_t vertices);
static void scene_vertex(struct sm_scene *scene, float x, float y, float lx, float ly, const float *shape, struct scene_color color, struct scene_color second);
static void scene_use(struct sm_scene *scene, unsigned pipe, size_t first);
static void scene_quad(struct sm_scene *scene, unsigned pipe, const float *corners, const float *locals, const float *shape, struct scene_color color, struct scene_color second);
static void scene_rect(struct sm_scene *scene, float x, float y, float width, float height, struct scene_color color);
static void scene_gradient(struct sm_scene *scene, float x, float y, float width, float height, struct scene_color top, struct scene_color bottom);
static void scene_plate(struct sm_scene *scene, const struct sm_box *box, unsigned layer, float radius);
static void scene_round(struct sm_scene *scene, float x, float y, float width, float height, float radius, struct scene_color fill, struct scene_color edge);
static void scene_disc(struct sm_scene *scene, float cx, float cy, float radius, struct scene_color color);
static float scene_text(struct sm_app *app, enum sm_style style, float x, float baseline, const char *text, struct scene_color color, int align);
static void scene_segment(struct sm_scene *scene, float x0, float y0, float x1, float y1, float thickness, struct scene_color color);
static void scene_graph(struct sm_app *app, const struct sm_box *box, enum sm_series series, float top_value, struct scene_color color, int area, float shift);
static float scene_peak(const struct sm_app *app, enum sm_series first, enum sm_series second, unsigned count);
static void scene_meter(struct sm_app *app, float x, float y, float width, const char *label, const char *value, float share, struct scene_color color);
static struct scene_color level_color(enum sm_level level);
static void build_header(struct sm_app *app, uint64_t now_ms);
static void build_summary(struct sm_app *app, float shift);
static void build_cores(struct sm_app *app);
static void build_state(struct sm_app *app);
static void build_graphics(struct sm_app *app);
static void build_network(struct sm_app *app, float shift);
static void build_memory(struct sm_app *app);
static void build_disk(struct sm_app *app, float shift);
static void build_events(struct sm_app *app);
static void shown_text(struct sm_app *app, enum sm_plate plate, const char *name, const char *value);

/*
 * Lays the plates out in a window of a size (design.md section 2.3).
 */
void
sm_layout_compute(
	struct sm_layout *layout,
	float width,
	float height)
{
	static const struct sm_box logical[SM_PLATES] = {
		{ 24.0f, 48.0f, 233.6f, 112.0f },
		{ 273.6f, 48.0f, 233.6f, 112.0f },
		{ 523.2f, 48.0f, 233.6f, 112.0f },
		{ 772.8f, 48.0f, 233.6f, 112.0f },
		{ 1022.4f, 48.0f, 233.6f, 112.0f },
		{ 24.0f, 176.0f, 400.0f, 344.0f },
		{ 440.0f, 176.0f, 400.0f, 344.0f },
		{ 856.0f, 176.0f, 400.0f, 344.0f },
		{ 24.0f, 536.0f, 496.0f, 204.0f },
		{ 536.0f, 536.0f, 256.0f, 204.0f },
		{ 808.0f, 536.0f, 448.0f, 204.0f },
		{ 1100.0f, 572.0f, 140.0f, 152.0f },
		{ 24.0f, 752.0f, 1232.0f, 36.0f }
	};
	float sx;
	float sy;
	unsigned plate;

	/* The scales: the boxes stretch, the text keeps to the smaller. */
	sx = width / SCENE_WIDTH;
	sy = height / SCENE_HEIGHT;
	layout->width = width;
	layout->height = height;
	layout->scale = sx;
	if (sy < layout->scale)
		layout->scale = sy;

	/* The header's band and each plate. */
	layout->header.x = 24.0f * sx;
	layout->header.y = 8.0f * sy;
	layout->header.width = 1232.0f * sx;
	layout->header.height = 32.0f * sy;
	for (plate = 0; plate < SM_PLATES; plate++) {
		layout->plates[plate].x = logical[plate].x * sx;
		layout->plates[plate].y = logical[plate].y * sy;
		layout->plates[plate].width = logical[plate].width * sx;
		layout->plates[plate].height = logical[plate].height * sy;
	}
}

/*
 * Empties a scene for the next frame (its memory is kept).
 */
void
sm_scene_clear(
	struct sm_scene *scene)
{
	/* No vertices and no draws. */
	scene->vertex_count = 0;
	scene->draw_count = 0;
}

/*
 * Frees a scene's memory.
 */
void
sm_scene_release(
	struct sm_scene *scene)
{
	/* The vertices and the draws. */
	free(scene->vertices);
	free(scene->draws);
	memset(scene, 0, sizeof(*scene));
}

/*
 * Builds the frame's shapes and glyphs at a time (milliseconds since the
 * monitor started; the graphs slide between samples by it).  Returns 0, or
 * ENOMEM when the scene could not grow.
 */
int
sm_scene_build(
	struct sm_app *app,
	uint64_t now_ms)
{
	const struct sm_layout *layout;
	float shift;
	float step;
	float x;
	int error;

	/* Room for a frame's worth, then nothing in it. */
	error = scene_reserve(&app->scene, 65536U);
	if (error != 0)
		return error;
	sm_scene_clear(&app->scene);
	layout = &app->layout;

	/* How far the graphs have slid towards the next sample (none with the clock stopped). */
	shift = 0.0f;
	if (!app->fixed_clock && app->have_frame && app->source.period_ms != 0U && now_ms > app->frame_at_ms) {
		shift = (float)(now_ms - app->frame_at_ms) / (float)app->source.period_ms;
		if (shift > 1.0f)
			shift = 1.0f;
	}

	/* The background: the deep gradient and the faint grid of layer 0. */
	scene_gradient(&app->scene, 0.0f, 0.0f, layout->width, layout->height, color_of(TOKEN_BG_DEEP, 1.0f), color_of(TOKEN_BG_MID, 1.0f));

	/* The grid: vertical lines every 48 logical pixels, below the header. */
	step = 48.0f * layout->scale;
	for (x = step; x < layout->width; x += step)
		scene_rect(&app->scene, floorf(x), layout->header.y + layout->header.height, 1.0f, layout->height, color_of(0x161d29U, 1.0f));

	/* The parts, back to front. */
	build_header(app, now_ms);
	build_summary(app, shift);
	build_cores(app);
	build_state(app);
	build_graphics(app);
	build_network(app, shift);
	build_memory(app);
	build_disk(app, shift);
	build_events(app);

	/* Succeeded: the scene is ready to draw. */
	return 0;
}

/* A colour token (0xRRGGBB) with an alpha, as floats. */
static struct scene_color
color_of(
	uint32_t rgb,
	float alpha)
{
	struct scene_color color;

	/* The three channels and the alpha. */
	color.r = (float)((rgb >> 16) & 0xffU) / 255.0f;
	color.g = (float)((rgb >> 8) & 0xffU) / 255.0f;
	color.b = (float)(rgb & 0xffU) / 255.0f;
	color.a = alpha;
	return color;
}

/* A colour between two (0: the first, 1: the second). */
static struct scene_color
color_mix(
	struct scene_color from,
	struct scene_color to,
	float amount)
{
	struct scene_color color;

	/* Each channel. */
	color.r = from.r + (to.r - from.r) * amount;
	color.g = from.g + (to.g - from.g) * amount;
	color.b = from.b + (to.b - from.b) * amount;
	color.a = from.a + (to.a - from.a) * amount;
	return color;
}

/* Makes room for more vertices and draws; returns 0 or ENOMEM. */
static int
scene_reserve(
	struct sm_scene *scene,
	size_t vertices)
{
	float *more_vertices;
	struct sm_draw *more_draws;
	size_t capacity;

	/* The vertices, doubled until they fit. */
	if (scene->vertex_count + vertices > scene->vertex_capacity) {
		capacity = scene->vertex_capacity;
		if (capacity == 0U)
			capacity = 65536U;
		while (capacity < scene->vertex_count + vertices)
			capacity *= 2U;
		more_vertices = realloc(scene->vertices, capacity * SM_VERTEX_FLOATS * sizeof(float));
		if (more_vertices == NULL)
			return ENOMEM;
		scene->vertices = more_vertices;
		scene->vertex_capacity = capacity;
	}

	/* The draws: room for a few more always. */
	if (scene->draw_count + 4U > scene->draw_capacity) {
		capacity = scene->draw_capacity * 2U;
		if (capacity == 0U)
			capacity = 256U;
		more_draws = realloc(scene->draws, capacity * sizeof(scene->draws[0]));
		if (more_draws == NULL)
			return ENOMEM;
		scene->draws = more_draws;
		scene->draw_capacity = capacity;
	}

	/* Succeeded: the room is there. */
	return 0;
}

/* Adds one vertex (the caller made room). */
static void
scene_vertex(
	struct sm_scene *scene,
	float x,
	float y,
	float lx,
	float ly,
	const float *shape,
	struct scene_color color,
	struct scene_color second)
{
	float *vertex;

	/* The four vec4s. */
	vertex = scene->vertices + scene->vertex_count * SM_VERTEX_FLOATS;
	vertex[0] = x;
	vertex[1] = y;
	vertex[2] = lx;
	vertex[3] = ly;
	vertex[4] = shape[0];
	vertex[5] = shape[1];
	vertex[6] = shape[2];
	vertex[7] = shape[3];
	vertex[8] = color.r;
	vertex[9] = color.g;
	vertex[10] = color.b;
	vertex[11] = color.a;
	vertex[12] = second.r;
	vertex[13] = second.g;
	vertex[14] = second.b;
	vertex[15] = second.a;
	scene->vertex_count++;
}

/* Counts the vertices from first in a draw of a pipeline: the last draw grows, or a new one starts. */
static void
scene_use(
	struct sm_scene *scene,
	unsigned pipe,
	size_t first)
{
	struct sm_draw *draw;
	size_t count;

	/* The vertices just added. */
	count = scene->vertex_count - first;

	/* The last draw of the same pipeline takes them. */
	if (scene->draw_count != 0U) {
		draw = &scene->draws[scene->draw_count - 1U];
		if (draw->pipe == pipe && (size_t)draw->first + draw->count == first) {
			draw->count += (uint32_t)count;
			return;
		}
	}

	/* A new draw. */
	draw = &scene->draws[scene->draw_count++];
	draw->pipe = pipe;
	draw->first = (uint32_t)first;
	draw->count = (uint32_t)count;
}

/* Adds a quad of four corners (x, y pairs, in order around it) with their local points, as two triangles. */
static void
scene_quad(
	struct sm_scene *scene,
	unsigned pipe,
	const float *corners,
	const float *locals,
	const float *shape,
	struct scene_color color,
	struct scene_color second)
{
	static const unsigned order[6] = { 0U, 1U, 2U, 0U, 2U, 3U };
	size_t first;
	unsigned index;
	unsigned corner;
	int error;

	/* Room, or nothing drawn. */
	error = scene_reserve(scene, 6U);
	if (error != 0)
		return;

	/* The two triangles. */
	first = scene->vertex_count;
	for (index = 0; index < 6U; index++) {
		corner = order[index];
		scene_vertex(scene, corners[corner * 2U], corners[corner * 2U + 1U], locals[corner * 2U], locals[corner * 2U + 1U],
			     shape, color, second);
	}

	/* In the draw of the pipeline. */
	scene_use(scene, pipe, first);
}

/* A solid rectangle. */
static void
scene_rect(
	struct sm_scene *scene,
	float x,
	float y,
	float width,
	float height,
	struct scene_color color)
{
	float corners[8];
	float locals[8];
	float shape[4];

	/* The corners; the local points do not matter to a solid fill. */
	corners[0] = x;
	corners[1] = y;
	corners[2] = x + width;
	corners[3] = y;
	corners[4] = x + width;
	corners[5] = y + height;
	corners[6] = x;
	corners[7] = y + height;
	memset(locals, 0, sizeof(locals));
	shape[0] = width * 0.5f;
	shape[1] = height * 0.5f;
	shape[2] = 0.0f;
	shape[3] = SM_SHAPE_SOLID;
	scene_quad(scene, SM_PIPE_SHAPE, corners, locals, shape, color, color);
}

/* A rectangle shaded from one colour at the top to another at the bottom. */
static void
scene_gradient(
	struct sm_scene *scene,
	float x,
	float y,
	float width,
	float height,
	struct scene_color top,
	struct scene_color bottom)
{
	float corners[8];
	float locals[8];
	float shape[4];

	/* The corners, and their points from the centre. */
	corners[0] = x;
	corners[1] = y;
	corners[2] = x + width;
	corners[3] = y;
	corners[4] = x + width;
	corners[5] = y + height;
	corners[6] = x;
	corners[7] = y + height;
	locals[0] = -width * 0.5f;
	locals[1] = -height * 0.5f;
	locals[2] = width * 0.5f;
	locals[3] = -height * 0.5f;
	locals[4] = width * 0.5f;
	locals[5] = height * 0.5f;
	locals[6] = -width * 0.5f;
	locals[7] = height * 0.5f;
	shape[0] = width * 0.5f;
	shape[1] = height * 0.5f;
	shape[2] = 0.0f;
	shape[3] = SM_SHAPE_GRADIENT;
	scene_quad(scene, SM_PIPE_SHAPE, corners, locals, shape, top, bottom);
}

/* A rounded rectangle with an edge: the shader's plate (a pixel larger for the edge's smoothing). */
static void
scene_round(
	struct sm_scene *scene,
	float x,
	float y,
	float width,
	float height,
	float radius,
	struct scene_color fill,
	struct scene_color edge)
{
	float corners[8];
	float locals[8];
	float shape[4];
	float half_width;
	float half_height;

	/* The quad a pixel beyond the shape, and its points from the centre. */
	half_width = width * 0.5f;
	half_height = height * 0.5f;
	corners[0] = x - 1.0f;
	corners[1] = y - 1.0f;
	corners[2] = x + width + 1.0f;
	corners[3] = y - 1.0f;
	corners[4] = x + width + 1.0f;
	corners[5] = y + height + 1.0f;
	corners[6] = x - 1.0f;
	corners[7] = y + height + 1.0f;
	locals[0] = -half_width - 1.0f;
	locals[1] = -half_height - 1.0f;
	locals[2] = half_width + 1.0f;
	locals[3] = -half_height - 1.0f;
	locals[4] = half_width + 1.0f;
	locals[5] = half_height + 1.0f;
	locals[6] = -half_width - 1.0f;
	locals[7] = half_height + 1.0f;
	shape[0] = half_width;
	shape[1] = half_height;
	shape[2] = radius;
	shape[3] = SM_SHAPE_PLATE;
	scene_quad(scene, SM_PIPE_SHAPE, corners, locals, shape, fill, edge);
}

/* A plate of a layer: its surface and edge, its corner radius, and its shadow-free depth by tone. */
static void
scene_plate(
	struct sm_scene *scene,
	const struct sm_box *box,
	unsigned layer,
	float radius)
{
	/* The plate in its layer's colours. */
	scene_round(scene, box->x, box->y, box->width, box->height, radius, color_of(layer_surface[layer], 1.0f), color_of(layer_edge[layer], 1.0f));
}

/* A disc. */
static void
scene_disc(
	struct sm_scene *scene,
	float cx,
	float cy,
	float radius,
	struct scene_color color)
{
	float corners[8];
	float locals[8];
	float shape[4];
	float reach;

	/* A square a pixel beyond the disc. */
	reach = radius + 1.0f;
	corners[0] = cx - reach;
	corners[1] = cy - reach;
	corners[2] = cx + reach;
	corners[3] = cy - reach;
	corners[4] = cx + reach;
	corners[5] = cy + reach;
	corners[6] = cx - reach;
	corners[7] = cy + reach;
	locals[0] = -reach;
	locals[1] = -reach;
	locals[2] = reach;
	locals[3] = -reach;
	locals[4] = reach;
	locals[5] = reach;
	locals[6] = -reach;
	locals[7] = reach;
	shape[0] = radius;
	shape[1] = radius;
	shape[2] = 0.0f;
	shape[3] = SM_SHAPE_DISC;
	scene_quad(scene, SM_PIPE_SHAPE, corners, locals, shape, color, color);
}

/*
 * Draws a string in a style from the atlas at a baseline: from x (align
 * 0), centred on x (1) or ending at x (2).  Reports its width.
 */
static float
scene_text(
	struct sm_app *app,
	enum sm_style style,
	float x,
	float baseline,
	const char *text,
	struct scene_color color,
	int align)
{
	const struct sm_atlas *atlas;
	const struct sm_glyph *glyph;
	float corners[8];
	float locals[8];
	float shape[4];
	float width;
	float pen;
	float top;
	float u0;
	float v0;
	float u1;
	float v1;
	unsigned character;

	/* Nothing without the atlas. */
	atlas = &app->atlas;
	if (atlas->pixels == NULL)
		return 0.0f;

	/* Where the pen starts. */
	width = sm_atlas_width(atlas, style, text);
	pen = x;
	if (align == 1)
		pen = x - width * 0.5f;
	else if (align == 2)
		pen = x - width;
	pen = floorf(pen + 0.5f);
	top = floorf(baseline + 0.5f) - (float)atlas->ascent[style] - 2.0f;
	memset(shape, 0, sizeof(shape));

	/* One quad a character: its cell, sampled from the atlas. */
	for (; *text != '\0'; text++) {
		character = (unsigned char)*text;
		if (character < SM_GLYPH_FIRST || character > SM_GLYPH_LAST)
			character = ' ';
		glyph = &atlas->glyphs[style][character - SM_GLYPH_FIRST];
		if (character != ' ') {
			/* The cell's corners and its place in the atlas. */
			corners[0] = pen - 2.0f;
			corners[1] = top;
			corners[2] = pen - 2.0f + (float)glyph->width;
			corners[3] = top;
			corners[4] = corners[2];
			corners[5] = top + (float)glyph->height;
			corners[6] = corners[0];
			corners[7] = corners[5];
			u0 = (float)glyph->x / (float)atlas->width;
			v0 = (float)glyph->y / (float)atlas->height;
			u1 = (float)(glyph->x + glyph->width) / (float)atlas->width;
			v1 = (float)(glyph->y + glyph->height) / (float)atlas->height;
			locals[0] = u0;
			locals[1] = v0;
			locals[2] = u1;
			locals[3] = v0;
			locals[4] = u1;
			locals[5] = v1;
			locals[6] = u0;
			locals[7] = v1;
			scene_quad(&app->scene, SM_PIPE_GLYPH, corners, locals, shape, color, color);
		}

		/* The pen moves past it. */
		pen += (float)glyph->advance;
	}

	/* Succeeded: the width drawn. */
	return width;
}

/* A line from one point to another, of a thickness, smoothed at its sides. */
static void
scene_segment(
	struct sm_scene *scene,
	float x0,
	float y0,
	float x1,
	float y1,
	float thickness,
	struct scene_color color)
{
	float corners[8];
	float locals[8];
	float shape[4];
	float dx;
	float dy;
	float length;
	float nx;
	float ny;
	float reach;

	/* The direction and its normal; a point is nothing. */
	dx = x1 - x0;
	dy = y1 - y0;
	length = sqrtf(dx * dx + dy * dy);
	if (length < 0.001f)
		return;
	nx = -dy / length;
	ny = dx / length;

	/* The quad half the thickness and a pixel to each side. */
	reach = thickness * 0.5f + 1.0f;
	corners[0] = x0 + nx * reach;
	corners[1] = y0 + ny * reach;
	corners[2] = x1 + nx * reach;
	corners[3] = y1 + ny * reach;
	corners[4] = x1 - nx * reach;
	corners[5] = y1 - ny * reach;
	corners[6] = x0 - nx * reach;
	corners[7] = y0 - ny * reach;
	locals[0] = 0.0f;
	locals[1] = reach;
	locals[2] = 0.0f;
	locals[3] = reach;
	locals[4] = 0.0f;
	locals[5] = -reach;
	locals[6] = 0.0f;
	locals[7] = -reach;
	shape[0] = length * 0.5f;
	shape[1] = thickness * 0.5f + 0.5f;
	shape[2] = 0.0f;
	shape[3] = SM_SHAPE_LINE;
	scene_quad(scene, SM_PIPE_SHAPE, corners, locals, shape, color, color);
}

/*
 * Draws a series of the history in a box over the range shown: a line,
 * and (area) the area under it fading down.  top_value is the value at the
 * box's top; shift slides it left by up to a sample towards the next one.
 */
static void
scene_graph(
	struct sm_app *app,
	const struct sm_box *box,
	enum sm_series series,
	float top_value,
	struct scene_color color,
	int area,
	float shift)
{
	static float values[SCENE_POINTS];
	float corners[8];
	float locals[8];
	float shape[4];
	float step;
	float x0;
	float y0;
	float x1;
	float y1;
	float bottom;
	float middle;
	unsigned range;
	unsigned count;
	unsigned index;
	struct scene_color clear;

	/* The range's samples, as many as were kept. */
	range = scene_ranges[app->range];
	count = sm_history_read(&app->history, series, range, values);
	if (count < 2U || top_value <= 0.0f)
		return;

	/* The step between samples across the box, and the shift. */
	step = box->width / (float)(range - 1U);
	bottom = box->y + box->height;
	middle = box->y + box->height * 0.5f;
	clear = color;
	clear.a = 0.0f;

	/* Each segment, from the oldest; the newest is at the box's right edge less the shift. */
	for (index = 0; index + 1U < count; index++) {
		x0 = box->x + box->width - (float)(count - 1U - index) * step - shift * step;
		x1 = x0 + step;
		if (x1 < box->x)
			continue;
		y0 = bottom - box->height * fminf(values[index] / top_value, 1.0f);
		y1 = bottom - box->height * fminf(values[index + 1U] / top_value, 1.0f);

		/* The area under it, fading towards the bottom. */
		if (area) {
			corners[0] = x0;
			corners[1] = y0;
			corners[2] = x1;
			corners[3] = y1;
			corners[4] = x1;
			corners[5] = bottom;
			corners[6] = x0;
			corners[7] = bottom;
			locals[0] = 0.0f;
			locals[1] = y0 - middle;
			locals[2] = 0.0f;
			locals[3] = y1 - middle;
			locals[4] = 0.0f;
			locals[5] = bottom - middle;
			locals[6] = 0.0f;
			locals[7] = bottom - middle;
			shape[0] = box->width * 0.5f;
			shape[1] = box->height * 0.5f;
			shape[2] = 0.0f;
			shape[3] = SM_SHAPE_AREA;
			scene_quad(&app->scene, SM_PIPE_SHAPE, corners, locals, shape, color_mix(clear, color, 0.28f), clear);
		}

		/* The line. */
		scene_segment(&app->scene, x0, y0, x1, y1, 1.6f * app->layout.scale, color);
	}
}

/* The peak of one or two series over the range shown, rounded up to 1, 2 or 5 of a power of ten (at least 1). */
static float
scene_peak(
	const struct sm_app *app,
	enum sm_series first,
	enum sm_series second,
	unsigned count)
{
	float peak;
	float other;
	float power;

	/* The larger of the two series' peaks, with room above it. */
	peak = sm_history_peak(&app->history, first, count);
	if (second != first) {
		other = sm_history_peak(&app->history, second, count);
		if (other > peak)
			peak = other;
	}

	/* Room above it. */
	peak *= 1.25f;
	if (peak < 1.0f)
		return 1.0f;

	/* The next 1, 2 or 5 step. */
	power = powf(10.0f, floorf(log10f(peak)));
	if (peak <= power)
		return power;
	if (peak <= 2.0f * power)
		return 2.0f * power;
	if (peak <= 5.0f * power)
		return 5.0f * power;
	return 10.0f * power;
}

/* A labelled meter: the label, the value at the right, and a bar filled to a share. */
static void
scene_meter(
	struct sm_app *app,
	float x,
	float y,
	float width,
	const char *label,
	const char *value,
	float share,
	struct scene_color color)
{
	float s;
	float bar_y;

	/* The label and the value. */
	s = app->layout.scale;
	(void)scene_text(app, SM_STYLE_SMALL, x, y + 12.0f * s, label, color_of(TOKEN_TEXT_DIM, 1.0f), 0);
	(void)scene_text(app, SM_STYLE_SMALL, x + width, y + 12.0f * s, value, color_of(TOKEN_TEXT, 1.0f), 2);

	/* The track and its fill. */
	bar_y = y + 18.0f * s;
	if (share < 0.0f)
		share = 0.0f;
	if (share > 1.0f)
		share = 1.0f;
	scene_round(&app->scene, x, bar_y, width, 5.0f * s, 2.5f * s, color_of(0x2c3a4cU, 1.0f), color_of(0x2c3a4cU, 0.0f));
	if (share > 0.0f)
		scene_round(&app->scene, x, bar_y, fmaxf(width * share, 5.0f * s), 5.0f * s, 2.5f * s, color, color_of(0, 0.0f));
}

/* The colour of a level (design.md section 3.2). */
static struct scene_color
level_color(
	enum sm_level level)
{
	/* Cyan, ice, amber and the muted coral. */
	switch (level) {
	case SM_LEVEL_ELEVATED:
		return color_of(TOKEN_ICE, 1.0f);
	case SM_LEVEL_WARNING:
		return color_of(TOKEN_AMBER, 1.0f);
	case SM_LEVEL_CRITICAL:
		return color_of(TOKEN_CORAL, 1.0f);
	default:
		return color_of(TOKEN_MINT, 1.0f);
	}
}

/* The header's band: the machine, its CPUs and uptime, and the state's chip. */
static void
build_header(
	struct sm_app *app,
	uint64_t now_ms)
{
	const struct sm_box *box;
	struct timespec now;
	char uptime[32];
	char line[160];
	float s;
	float baseline;
	float chip_width;
	float chip_x;
	uint64_t seconds;
	int error;

	UNUSED_PARAMETER(now_ms);

	/* The system's uptime (the stopped clock's in a test). */
	s = app->layout.scale;
	box = &app->layout.header;
	seconds = app->fixed_ms / 1000U;
	if (!app->fixed_clock) {
		error = clock_gettime(CLOCK_MONOTONIC, &now);
		seconds = 0;
		if (error == 0)
			seconds = (uint64_t)now.tv_sec;
	}

	/* As text. */
	(void)sm_format_uptime(uptime, sizeof(uptime), seconds);

	/* The machine's line. */
	baseline = box->y + box->height * 0.5f + 5.0f * s;
	(void)snprintf(line, sizeof(line), "%s  |  %u CPUs  |  up %s", app->source.info.host, app->source.info.cpu_count, uptime);
	(void)scene_text(app, SM_STYLE_NOTE, box->x + 4.0f * s, baseline, line, color_of(TOKEN_TEXT_DIM, 1.0f), 0);

	/* The state's chip at the right: a dot and the level's name. */
	chip_width = sm_atlas_width(&app->atlas, SM_STYLE_NOTE, sm_level_name(app->level)) + 40.0f * s;
	chip_x = box->x + box->width - chip_width;
	scene_round(&app->scene, chip_x, box->y + 2.0f * s, chip_width, box->height - 4.0f * s, (box->height - 4.0f * s) * 0.5f,
		    color_mix(color_of(TOKEN_SURFACE_1, 1.0f), level_color(app->level), 0.14f), color_mix(color_of(TOKEN_EDGE_1, 1.0f), level_color(app->level), 0.5f));
	scene_disc(&app->scene, chip_x + 16.0f * s, box->y + box->height * 0.5f, 4.5f * s, level_color(app->level));
	(void)scene_text(app, SM_STYLE_NOTE, chip_x + 28.0f * s, baseline, sm_level_name(app->level), color_of(TOKEN_TEXT, 1.0f), 0);
	shown_text(app, SM_PLATE_STATE, "state", sm_level_name(app->level));
}

/* The summary row: five plates of a title, a value, a note and a graph. */
static void
build_summary(
	struct sm_app *app,
	float shift)
{
	static const char *const titles[5] = { "CPU", "GPU", "Memory", "Network", "Disk" };
	static const char *const names[5] = { "cpu", "gpu", "memory", "network", "disk" };
	const struct sm_frame *frame;
	const struct sm_info *info;
	const struct sm_box *box;
	struct sm_box graph;
	char value[48];
	char note[64];
	char first[24];
	char second[24];
	float s;
	float peak;
	unsigned plate;

	/* Each plate. */
	s = app->layout.scale;
	frame = &app->frame;
	info = &app->source.info;
	for (plate = 0; plate < 5U; plate++) {
		box = &app->layout.plates[SM_PLATE_CPU + plate];
		scene_plate(&app->scene, box, 2, 14.0f * s);
		(void)scene_text(app, SM_STYLE_TITLE, box->x + 16.0f * s, box->y + 26.0f * s, titles[plate], color_of(TOKEN_TEXT, 1.0f), 0);

		/* The value and the note. */
		value[0] = '\0';
		note[0] = '\0';
		switch (plate) {
		case 0:
			(void)sm_format_percent(value, sizeof(value), frame->cpu);
			(void)snprintf(note, sizeof(note), "%u cores", info->cpu_count);
			break;
		case 1:
			if (info->gpu_count == 0U) {
				strcpy(value, "-");
				strcpy(note, "none");
			} else {
				(void)sm_format_percent(value, sizeof(value), frame->gpu_busy[0]);
				(void)snprintf(note, sizeof(note), "%.20s", info->gpu_name[0]);
			}

			break;
		case 2:
			(void)sm_format_pair(value, sizeof(value), frame->memory_used, info->memory_total);
			(void)sm_format_bytes(first, sizeof(first), frame->swap_used);
			(void)snprintf(note, sizeof(note), "Swap %s", first);
			break;
		case 3:
			(void)sm_format_rate(value, sizeof(value), frame->rx_rate + frame->tx_rate, 1);
			(void)sm_format_rate(first, sizeof(first), frame->rx_rate, 1);
			(void)sm_format_rate(second, sizeof(second), frame->tx_rate, 1);
			(void)snprintf(note, sizeof(note), "RX %s", first);
			break;
		default:
			(void)sm_format_rate(value, sizeof(value), frame->read_rate + frame->write_rate, 0);
			(void)sm_format_rate(first, sizeof(first), frame->read_rate, 0);
			(void)snprintf(note, sizeof(note), "R %s", first);
			break;
		}

		/* A plate without a frame yet shows a dash. */
		if (!app->have_frame)
			strcpy(value, "-");
		(void)scene_text(app, SM_STYLE_NOTE, box->x + box->width - 16.0f * s, box->y + 26.0f * s, note, color_of(TOKEN_TEXT_DIM, 1.0f), 2);
		(void)scene_text(app, SM_STYLE_VALUE, box->x + 16.0f * s, box->y + 66.0f * s, value, color_of(TOKEN_TEXT, 1.0f), 0);
		shown_text(app, (enum sm_plate)(SM_PLATE_CPU + plate), names[plate], value);

		/* The graph across the plate's foot. */
		graph.x = box->x + 16.0f * s;
		graph.y = box->y + 76.0f * s;
		graph.width = box->width - 32.0f * s;
		graph.height = 26.0f * s;
		switch (plate) {
		case 0:
			scene_graph(app, &graph, SM_SERIES_CPU, 1.0f, color_of(TOKEN_CYAN, 1.0f), 1, shift);
			break;
		case 1:
			scene_graph(app, &graph, SM_SERIES_GPU, 1.0f, color_of(TOKEN_ICE, 1.0f), 1, shift);
			break;
		case 2:
			scene_graph(app, &graph, SM_SERIES_MEMORY, 1.0f, color_of(TOKEN_CYAN, 1.0f), 1, shift);
			break;
		case 3:
			peak = scene_peak(app, SM_SERIES_RX, SM_SERIES_TX, scene_ranges[app->range]);
			scene_graph(app, &graph, SM_SERIES_RX, peak, color_of(TOKEN_MINT, 1.0f), 1, shift);
			break;
		default:
			peak = scene_peak(app, SM_SERIES_READ, SM_SERIES_WRITE, scene_ranges[app->range]);
			scene_graph(app, &graph, SM_SERIES_READ, peak, color_of(TOKEN_CYAN, 1.0f), 0, shift);
			scene_graph(app, &graph, SM_SERIES_WRITE, peak, color_of(TOKEN_AMBER, 1.0f), 0, shift);
			break;
		}
	}
}

/* The CPU cores: a tile a CPU, lit by its load, and the overall, highest and lowest. */
static void
build_cores(
	struct sm_app *app)
{
	const struct sm_box *box;
	const struct sm_frame *frame;
	struct scene_color cold;
	struct scene_color hot;
	struct scene_color edge;
	char line[96];
	char overall[16];
	char highest[16];
	char lowest[16];
	float s;
	float cell;
	float gap;
	float grid_width;
	float grid_height;
	float left;
	float top;
	float load;
	unsigned count;
	unsigned columns;
	unsigned rows;
	unsigned index;
	unsigned high_index;
	double high;
	double low;

	/* The plate and its title. */
	s = app->layout.scale;
	box = &app->layout.plates[SM_PLATE_CORES];
	frame = &app->frame;
	scene_plate(&app->scene, box, 1, 14.0f * s);
	(void)scene_text(app, SM_STYLE_TITLE, box->x + 18.0f * s, box->y + 28.0f * s, "CPU cores", color_of(TOKEN_TEXT, 1.0f), 0);
	(void)scene_text(app, SM_STYLE_NOTE, box->x + box->width - 18.0f * s, box->y + 28.0f * s, "per-core load", color_of(TOKEN_TEXT_DIM, 1.0f), 2);

	/* The grid: columns for a wide field, cells as large as fit. */
	count = app->source.info.cpu_count;
	if (count == 0U)
		return;
	columns = (unsigned)ceilf(sqrtf((float)count * 1.6f));
	if (columns > count)
		columns = count;
	rows = (count + columns - 1U) / columns;
	gap = 8.0f * s;
	cell = (box->width - 48.0f * s - gap * (float)(columns - 1U)) / (float)columns;
	if (cell > (box->height - 120.0f * s - gap * (float)(rows - 1U)) / (float)rows)
		cell = (box->height - 120.0f * s - gap * (float)(rows - 1U)) / (float)rows;
	if (cell > 64.0f * s)
		cell = 64.0f * s;
	grid_width = cell * (float)columns + gap * (float)(columns - 1U);
	grid_height = cell * (float)rows + gap * (float)(rows - 1U);
	left = box->x + (box->width - grid_width) * 0.5f;
	top = box->y + 48.0f * s + (box->height - 120.0f * s - grid_height) * 0.5f;

	/* Each tile, its tone from the cold surface to the cyan by its load, its edge lit above 85%. */
	cold = color_of(0x2a3749U, 1.0f);
	hot = color_mix(cold, color_of(TOKEN_CYAN, 1.0f), 0.85f);
	high = -1.0;
	low = 2.0;
	high_index = 0;
	for (index = 0; index < count; index++) {
		load = (float)frame->cpu_core[index];
		edge = color_of(TOKEN_EDGE_1, 1.0f);
		if (load > 0.85f)
			edge = color_of(TOKEN_ICE, 0.9f);
		scene_round(&app->scene, left + (float)(index % columns) * (cell + gap), top + (float)(index / columns) * (cell + gap),
			    cell, cell, 6.0f * s, color_mix(cold, hot, load), edge);

		/* The highest and the lowest so far. */
		if (frame->cpu_core[index] > high) {
			high = frame->cpu_core[index];
			high_index = index;
		}

		/* And the lowest. */
		if (frame->cpu_core[index] < low)
			low = frame->cpu_core[index];
	}

	/* The overall, the highest (with its core) and the lowest. */
	(void)sm_format_percent(overall, sizeof(overall), frame->cpu);
	(void)sm_format_percent(highest, sizeof(highest), high);
	(void)sm_format_percent(lowest, sizeof(lowest), low);
	(void)snprintf(line, sizeof(line), "Overall %s    Highest %s (core %u)    Lowest %s", overall, highest, high_index, lowest);
	(void)scene_text(app, SM_STYLE_NOTE, box->x + 18.0f * s, box->y + box->height - 20.0f * s, line, color_of(TOKEN_TEXT_DIM, 1.0f), 0);
}

/* The system's state: nested squares in the level's colour (the 3D core comes with p003), the level and its cause. */
static void
build_state(
	struct sm_app *app)
{
	const struct sm_box *box;
	struct scene_color tone;
	const char *note;
	float s;
	float cx;
	float cy;
	float size;
	unsigned shell;

	/* The plate and its title. */
	s = app->layout.scale;
	box = &app->layout.plates[SM_PLATE_STATE];
	scene_plate(&app->scene, box, 2, 14.0f * s);
	(void)scene_text(app, SM_STYLE_TITLE, box->x + 18.0f * s, box->y + 28.0f * s, "System state", color_of(TOKEN_TEXT, 1.0f), 0);
	note = "all nominal";
	if (app->level != SM_LEVEL_NORMAL)
		note = "attention";
	(void)scene_text(app, SM_STYLE_NOTE, box->x + box->width - 18.0f * s, box->y + 28.0f * s, note, color_of(TOKEN_TEXT_DIM, 1.0f), 2);

	/* Three nested squares, the inner ones brighter. */
	tone = level_color(app->level);
	cx = box->x + box->width * 0.5f;
	cy = box->y + box->height * 0.48f;
	for (shell = 0; shell < 3U; shell++) {
		size = (box->height * 0.5f) * (1.0f - 0.3f * (float)shell);
		scene_round(&app->scene, cx - size * 0.5f, cy - size * 0.5f, size, size, (10.0f - 3.0f * (float)shell) * s,
			    color_mix(color_of(TOKEN_SURFACE_2, 1.0f), tone, 0.12f + 0.16f * (float)shell),
			    color_mix(color_of(TOKEN_EDGE_2, 1.0f), tone, 0.5f));
	}

	/* The level's word and what holds it. */
	(void)scene_text(app, SM_STYLE_STATE, cx, box->y + box->height - 46.0f * s, sm_level_name(app->level), color_of(TOKEN_TEXT, 1.0f), 1);
	(void)scene_text(app, SM_STYLE_NOTE, cx, box->y + box->height - 22.0f * s, app->rules.summary, color_of(TOKEN_TEXT_DIM, 1.0f), 1);
}

/* The GPUs: a module card each (two fit), its use, memory, temperature and power. */
static void
build_graphics(
	struct sm_app *app)
{
	const struct sm_box *box;
	const struct sm_frame *frame;
	const struct sm_info *info;
	struct sm_box card;
	struct scene_color warm;
	const char *plural;
	char note[32];
	char value[32];
	char text[32];
	float s;
	float share;
	float card_y;
	float card_height;
	float x;
	float width;
	unsigned gpu;
	unsigned shown;

	/* The plate and its title. */
	s = app->layout.scale;
	box = &app->layout.plates[SM_PLATE_GRAPHICS];
	frame = &app->frame;
	info = &app->source.info;
	scene_plate(&app->scene, box, 1, 14.0f * s);
	(void)scene_text(app, SM_STYLE_TITLE, box->x + 18.0f * s, box->y + 28.0f * s, "Graphics", color_of(TOKEN_TEXT, 1.0f), 0);
	plural = "s";
	if (info->gpu_count == 1U)
		plural = "";
	(void)snprintf(note, sizeof(note), "%u device%s", info->gpu_count, plural);
	(void)scene_text(app, SM_STYLE_NOTE, box->x + box->width - 18.0f * s, box->y + 28.0f * s, note, color_of(TOKEN_TEXT_DIM, 1.0f), 2);

	/* No GPU: a line saying so. */
	if (info->gpu_count == 0U) {
		(void)scene_text(app, SM_STYLE_NOTE, box->x + box->width * 0.5f, box->y + box->height * 0.5f, "No GPU",
				 color_of(TOKEN_TEXT_FAINT, 1.0f), 1);
		return;
	}

	/* A card each, the first two. */
	shown = info->gpu_count;
	if (shown > 2U)
		shown = 2U;
	card_height = (box->height - 64.0f * s - 12.0f * s) / 2.0f;
	for (gpu = 0; gpu < shown; gpu++) {
		/* The card. */
		card_y = box->y + 46.0f * s + (float)gpu * (card_height + 12.0f * s);
		card.x = box->x + 16.0f * s;
		card.y = card_y;
		card.width = box->width - 32.0f * s;
		card.height = card_height;
		scene_plate(&app->scene, &card, 2, 10.0f * s);
		(void)snprintf(text, sizeof(text), "GPU %u  %.24s", gpu, info->gpu_name[gpu]);
		(void)scene_text(app, SM_STYLE_NOTE, card.x + 14.0f * s, card.y + 22.0f * s, text, color_of(TOKEN_TEXT, 1.0f), 0);

		/* Its use, large. */
		(void)sm_format_percent(value, sizeof(value), frame->gpu_busy[gpu]);
		(void)scene_text(app, SM_STYLE_VALUE_SMALL, card.x + 14.0f * s, card.y + 58.0f * s, value, color_of(TOKEN_TEXT, 1.0f), 0);
		(void)scene_text(app, SM_STYLE_SMALL, card.x + 14.0f * s, card.y + 76.0f * s, "Utilization", color_of(TOKEN_TEXT_DIM, 1.0f), 0);

		/* The meters: memory, temperature, power. */
		x = card.x + 120.0f * s;
		width = card.width - 134.0f * s;
		(void)sm_format_pair(text, sizeof(text), frame->gpu_memory_used[gpu], info->gpu_memory_total[gpu]);
		share = 0.0f;
		if (info->gpu_memory_total[gpu] != 0U)
			share = (float)frame->gpu_memory_used[gpu] / (float)info->gpu_memory_total[gpu];
		scene_meter(app, x, card.y + 30.0f * s, width, "Memory", text, share, color_of(TOKEN_CYAN, 1.0f));
		(void)snprintf(text, sizeof(text), "%.0f C", frame->gpu_celsius[gpu]);
		warm = color_of(TOKEN_MINT, 1.0f);
		if (frame->gpu_celsius[gpu] > 80.0)
			warm = color_of(TOKEN_AMBER, 1.0f);
		scene_meter(app, x, card.y + 58.0f * s, width, "Temperature", text, (float)(frame->gpu_celsius[gpu] - 30.0) / 70.0f, warm);
		(void)snprintf(text, sizeof(text), "%.0f W", frame->gpu_watts[gpu]);
		scene_meter(app, x, card.y + 86.0f * s, width, "Power", text, (float)frame->gpu_watts[gpu] / 60.0f, color_of(TOKEN_MINT, 1.0f));
	}
}

/* The network: the received and the sent flows over the range shown. */
static void
build_network(
	struct sm_app *app,
	float shift)
{
	const struct sm_box *box;
	struct sm_box graph;
	char note[64];
	char rx[24];
	char tx[24];
	char scale_text[24];
	float s;
	float peak;

	/* The plate and its title. */
	s = app->layout.scale;
	box = &app->layout.plates[SM_PLATE_FLOW];
	scene_plate(&app->scene, box, 1, 14.0f * s);
	(void)scene_text(app, SM_STYLE_TITLE, box->x + 18.0f * s, box->y + 28.0f * s, "Network", color_of(TOKEN_TEXT, 1.0f), 0);
	(void)sm_format_rate(rx, sizeof(rx), app->frame.rx_rate, 1);
	(void)sm_format_rate(tx, sizeof(tx), app->frame.tx_rate, 1);
	(void)snprintf(note, sizeof(note), "RX %s   TX %s", rx, tx);
	(void)scene_text(app, SM_STYLE_NOTE, box->x + box->width - 18.0f * s, box->y + 28.0f * s, note, color_of(TOKEN_TEXT_DIM, 1.0f), 2);

	/* The graph and its scale. */
	graph.x = box->x + 18.0f * s;
	graph.y = box->y + 52.0f * s;
	graph.width = box->width - 36.0f * s;
	graph.height = box->height - 72.0f * s;
	peak = scene_peak(app, SM_SERIES_RX, SM_SERIES_TX, scene_ranges[app->range]);
	scene_rect(&app->scene, graph.x, graph.y, graph.width, 1.0f, color_of(TOKEN_EDGE_0, 1.0f));
	scene_rect(&app->scene, graph.x, graph.y + graph.height, graph.width, 1.0f, color_of(TOKEN_EDGE_0, 1.0f));
	(void)sm_format_rate(scale_text, sizeof(scale_text), peak, 1);
	(void)scene_text(app, SM_STYLE_SMALL, graph.x, graph.y + 14.0f * s, scale_text, color_of(TOKEN_TEXT_FAINT, 1.0f), 0);
	scene_graph(app, &graph, SM_SERIES_RX, peak, color_of(TOKEN_CYAN, 1.0f), 1, shift);
	scene_graph(app, &graph, SM_SERIES_TX, peak, color_of(TOKEN_MINT, 1.0f), 1, shift);
}

/* The memory: its strata, used, cache, available and swap. */
static void
build_memory(
	struct sm_app *app)
{
	static const char *const names[4] = { "Used", "Cache", "Available", "Swap" };
	const struct sm_box *box;
	const struct sm_frame *frame;
	const struct sm_info *info;
	struct scene_color colors[4];
	char value[24];
	uint64_t amounts[4];
	uint64_t totals[4];
	float s;
	float y;
	float height;
	float width;
	float share;
	unsigned layer;

	/* The plate and its title. */
	s = app->layout.scale;
	box = &app->layout.plates[SM_PLATE_STRATA];
	frame = &app->frame;
	info = &app->source.info;
	scene_plate(&app->scene, box, 1, 14.0f * s);
	(void)scene_text(app, SM_STYLE_TITLE, box->x + 18.0f * s, box->y + 28.0f * s, "Memory", color_of(TOKEN_TEXT, 1.0f), 0);
	(void)sm_format_bytes(value, sizeof(value), info->memory_total);
	(void)scene_text(app, SM_STYLE_NOTE, box->x + box->width - 18.0f * s, box->y + 28.0f * s, value, color_of(TOKEN_TEXT_DIM, 1.0f), 2);

	/* The four strata, each a bar of its share of its total. */
	amounts[0] = frame->memory_used;
	amounts[1] = frame->memory_cache;
	amounts[2] = frame->memory_available;
	amounts[3] = frame->swap_used;
	totals[0] = info->memory_total;
	totals[1] = info->memory_total;
	totals[2] = info->memory_total;
	totals[3] = info->swap_total;
	colors[0] = color_of(TOKEN_CYAN, 1.0f);
	colors[1] = color_of(TOKEN_ICE, 1.0f);
	colors[2] = color_of(0x3c5068U, 1.0f);
	colors[3] = color_of(TOKEN_AMBER, 1.0f);
	height = (box->height - 64.0f * s) / 4.0f - 6.0f * s;
	width = box->width - 36.0f * s;
	for (layer = 0; layer < 4U; layer++) {
		y = box->y + 46.0f * s + (float)layer * (height + 6.0f * s);
		share = 0.0f;
		if (totals[layer] != 0U)
			share = (float)amounts[layer] / (float)totals[layer];
		scene_round(&app->scene, box->x + 18.0f * s, y, width, height, 6.0f * s, color_of(0x243041U, 1.0f), color_of(TOKEN_EDGE_0, 1.0f));
		if (share > 0.0f) {
			scene_round(&app->scene, box->x + 18.0f * s, y, fmaxf(width * fminf(share, 1.0f), 8.0f * s), height, 6.0f * s,
				    color_mix(color_of(0x243041U, 1.0f), colors[layer], 0.75f), color_of(0, 0.0f));
		}

		/* Its name and amount. */
		(void)scene_text(app, SM_STYLE_SMALL, box->x + 28.0f * s, y + height * 0.5f + 4.0f * s, names[layer], color_of(TOKEN_TEXT, 1.0f), 0);
		(void)sm_format_bytes(value, sizeof(value), amounts[layer]);
		(void)scene_text(app, SM_STYLE_SMALL, box->x + box->width - 28.0f * s, y + height * 0.5f + 4.0f * s, value,
				 color_of(TOKEN_TEXT, 1.0f), 2);
	}
}

/* The disks: the read and the written flows, and the latency's plate. */
static void
build_disk(
	struct sm_app *app,
	float shift)
{
	static const char *const lane_names[2] = { "Read", "Write" };
	static const enum sm_series lane_series[2] = { SM_SERIES_READ, SM_SERIES_WRITE };
	static const uint32_t lane_colors[2] = { TOKEN_CYAN, TOKEN_AMBER };
	const struct sm_box *box;
	const struct sm_box *latency;
	struct sm_box graph;
	struct scene_color dot;
	char note[64];
	char first[24];
	char second[24];
	char value[24];
	const char *word;
	float s;
	float peak;
	unsigned lane;

	/* The plate and its title. */
	s = app->layout.scale;
	box = &app->layout.plates[SM_PLATE_LANES];
	scene_plate(&app->scene, box, 1, 14.0f * s);
	(void)scene_text(app, SM_STYLE_TITLE, box->x + 18.0f * s, box->y + 28.0f * s, "Disk", color_of(TOKEN_TEXT, 1.0f), 0);
	(void)sm_format_rate(first, sizeof(first), app->frame.read_rate, 0);
	(void)sm_format_rate(second, sizeof(second), app->frame.write_rate, 0);
	(void)snprintf(note, sizeof(note), "Read %s   Write %s", first, second);
	(void)scene_text(app, SM_STYLE_NOTE, box->x + box->width - 172.0f * s, box->y + 28.0f * s, note, color_of(TOKEN_TEXT_DIM, 1.0f), 2);

	/* Two lanes: read above, write below, on one scale. */
	peak = scene_peak(app, SM_SERIES_READ, SM_SERIES_WRITE, scene_ranges[app->range]);
	for (lane = 0; lane < 2U; lane++) {
		graph.x = box->x + 70.0f * s;
		graph.y = box->y + 50.0f * s + (float)lane * 72.0f * s;
		graph.width = box->width - 252.0f * s;
		graph.height = 56.0f * s;
		scene_rect(&app->scene, graph.x, graph.y + graph.height, graph.width, 1.0f, color_of(TOKEN_EDGE_0, 1.0f));
		(void)scene_text(app, SM_STYLE_SMALL, box->x + 18.0f * s, graph.y + graph.height * 0.5f + 4.0f * s, lane_names[lane],
				 color_of(TOKEN_TEXT_DIM, 1.0f), 0);
		scene_graph(app, &graph, lane_series[lane], peak, color_of(lane_colors[lane], 1.0f), 1, shift);
	}

	/* The latency's plate, in front: its value and a word for it. */
	latency = &app->layout.plates[SM_PLATE_LATENCY];
	scene_plate(&app->scene, latency, 2, 10.0f * s);
	(void)scene_text(app, SM_STYLE_NOTE, latency->x + 14.0f * s, latency->y + 24.0f * s, "Latency", color_of(TOKEN_TEXT, 1.0f), 0);
	(void)snprintf(value, sizeof(value), "%.1f ms", app->frame.disk_latency_ms);
	(void)scene_text(app, SM_STYLE_VALUE_SMALL, latency->x + 14.0f * s, latency->y + 64.0f * s, value, color_of(TOKEN_TEXT, 1.0f), 0);
	shown_text(app, SM_PLATE_LATENCY, "latency", value);
	word = "Good";
	if (app->frame.disk_latency_ms > 50.0)
		word = "Stalled";
	else if (app->frame.disk_latency_ms > 5.0)
		word = "Slow";
	dot = color_of(TOKEN_MINT, 1.0f);
	if (app->frame.disk_latency_ms > 5.0)
		dot = color_of(TOKEN_AMBER, 1.0f);
	scene_disc(&app->scene, latency->x + 20.0f * s, latency->y + latency->height - 24.0f * s, 4.0f * s, dot);
	(void)scene_text(app, SM_STYLE_SMALL, latency->x + 32.0f * s, latency->y + latency->height - 19.0f * s, word, color_of(TOKEN_TEXT_DIM, 1.0f), 0);
}

/* The events' strip: the newest three, newest first. */
static void
build_events(
	struct sm_app *app)
{
	const struct sm_box *box;
	const struct sm_event *event;
	char line[120];
	float s;
	float x;
	float baseline;
	unsigned shown;
	unsigned index;
	uint64_t seconds;

	/* The strip and its title. */
	s = app->layout.scale;
	box = &app->layout.plates[SM_PLATE_EVENTS];
	scene_plate(&app->scene, box, 0, 10.0f * s);
	baseline = box->y + box->height * 0.5f + 5.0f * s;
	x = box->x + 18.0f * s;
	x += scene_text(app, SM_STYLE_NOTE, x, baseline, "Events", color_of(TOKEN_TEXT, 1.0f), 0) + 28.0f * s;

	/* None yet. */
	if (app->event_count == 0U) {
		(void)scene_text(app, SM_STYLE_NOTE, x, baseline, "No events", color_of(TOKEN_TEXT_FAINT, 1.0f), 0);
		return;
	}

	/* The newest three: a dot in the level's colour, the time and the text. */
	shown = app->event_count;
	if (shown > 3U)
		shown = 3U;
	for (index = 0; index < shown; index++) {
		event = &app->events[(app->event_count - 1U - index) % SM_EVENTS_MAX];
		seconds = event->time_ms / 1000U;
		scene_disc(&app->scene, x + 4.0f * s, box->y + box->height * 0.5f, 4.0f * s, level_color(event->level));
		(void)snprintf(line, sizeof(line), "%02llu:%02llu  %s", (unsigned long long)((seconds / 60U) % 60U),
			       (unsigned long long)(seconds % 60U), event->text);
		x += 14.0f * s;
		x += scene_text(app, SM_STYLE_NOTE, x, baseline, line, color_of(TOKEN_TEXT_DIM, 1.0f), 0) + 32.0f * s;
	}
}

/* Logs a plate's value when it changes ("ZMON TEXT plate=cpu value=37%"), for the tests to compare with the input. */
static void
shown_text(
	struct sm_app *app,
	enum sm_plate plate,
	const char *name,
	const char *value)
{
	int same;

	/* The same text as last time says nothing. */
	same = strncmp(app->shown[plate], value, sizeof(app->shown[plate]) - 1U);
	if (same == 0)
		return;

	/* The new text, kept and logged. */
	(void)snprintf(app->shown[plate], sizeof(app->shown[plate]), "%s", value);
	printf("ZMON TEXT plate=%s value=\"%s\"\n", name, value);
}
