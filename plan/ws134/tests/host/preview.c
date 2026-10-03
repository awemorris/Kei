/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * ws134-p003: the System Monitor's scene without a window, for looking at
 * a layout on the host: plays a source to a stopped time like
 * --clock=fixed (userland/desktop/monitor/main.c), builds the frame's scene
 * (scene.c, space.c, draw.c, atlas.c) and writes its vertices, its draws
 * and the atlas to a file that preview.py rasterizes the way the shaders
 * draw (plan/ws134/tests/host/preview.sh).
 *
 *	preview OUT WIDTH HEIGHT TIME_MS SANS MONO [replay:FILE | sim:SEED[:CPUS:GPUS]] [POINTER_X POINTER_Y]
 */

#include "app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void take_frames(struct sm_app *app, uint64_t now);

/* Builds one scene and writes it. */
int
main(
	int argc,
	char **argv)
{
	static struct sm_app app;
	FILE *file;
	uint64_t time;
	uint32_t header[6];
	unsigned cpus;
	unsigned gpus;
	unsigned bench;
	unsigned round;
	struct timespec begin;
	struct timespec end;
	const char *bench_text;
	int error;

	/* The arguments. */
	if (argc < 8) {
		fprintf(stderr, "usage: preview OUT WIDTH HEIGHT TIME_MS SANS MONO SOURCE [POINTER_X POINTER_Y]\n");
		return 2;
	}

	app.fixed_clock = 1;
	app.fixed_ms = strtoull(argv[4], NULL, 10);
	app.period_ms = 1000;
	app.range = 1;

	/* The fonts and the source. */
	error = kui_text_open(&app.sans, argv[5], NULL);
	if (error == 0)
		error = kui_text_open(&app.mono, argv[6], NULL);
	if (error != 0) {
		fprintf(stderr, "preview: fonts: error %d\n", error);
		return 1;
	}

	if (strncmp(argv[7], "replay:", 7) == 0) {
		error = sm_source_open_replay(&app.source, argv[7] + 7, 1000);
	} else {
		cpus = 4;
		gpus = 1;
		(void)sscanf(argv[7], "sim:%llu:%u:%u", (unsigned long long *)&app.seed, &cpus, &gpus);
		error = sm_source_open_sim(&app.source, app.seed, 1000, cpus, gpus, 0);
		(void)snprintf(app.source.info.gpu_name[0], SM_NAME_MAX, "Virtio-GPU Venus (llvmpipe)");
	}

	if (error != 0) {
		fprintf(stderr, "preview: source: error %d\n", error);
		return 1;
	}

	/* The pointer, if given (-1 to 1). */
	if (argc > 9) {
		app.motion.pointer_x = (float)strtod(argv[8], NULL);
		app.motion.pointer_y = (float)strtod(argv[9], NULL);
	}

	/* The data up to the time, the layout and the atlas. */
	sm_history_init(&app.history);
	sm_rules_init(&app.rules);
	for (time = 0; time <= app.fixed_ms; time += app.period_ms)
		take_frames(&app, time);
	sm_layout_compute(&app.layout, (float)strtoul(argv[2], NULL, 10), (float)strtoul(argv[3], NULL, 10));
	error = sm_atlas_build(&app.atlas, &app.sans, &app.mono, app.layout.scale);
	if (error != 0)
		return 1;

	/* The scene. */
	error = sm_scene_build(&app, app.fixed_ms);
	if (error != 0)
		return 1;

	/* PREVIEW_BENCH=N: the scene built N times more, and the mean time of one (the CPU's share of a frame). */
	bench = 0;
	bench_text = getenv("PREVIEW_BENCH");
	if (bench_text != NULL)
		bench = (unsigned)strtoul(bench_text, NULL, 10);
	clock_gettime(CLOCK_MONOTONIC, &begin);
	for (round = 0; round < bench; round++)
		(void)sm_scene_build(&app, app.fixed_ms + round * 16U);
	clock_gettime(CLOCK_MONOTONIC, &end);
	if (bench != 0U)
		printf("preview: scene %.3f ms each (%u builds)\n", ((double)(end.tv_sec - begin.tv_sec) * 1000.0 + (double)(end.tv_nsec - begin.tv_nsec) / 1e6) / bench, bench);

	/* The file: the sizes, the vertices, the draws and the atlas's pixels. */
	file = fopen(argv[1], "wb");
	if (file == NULL)
		return 1;
	header[0] = (uint32_t)app.layout.width;
	header[1] = (uint32_t)app.layout.height;
	header[2] = (uint32_t)app.scene.vertex_count;
	header[3] = (uint32_t)app.scene.draw_count;
	header[4] = (uint32_t)app.atlas.width;
	header[5] = (uint32_t)app.atlas.height;
	fwrite(header, sizeof(header), 1, file);
	fwrite(app.scene.vertices, sizeof(float) * SM_VERTEX_FLOATS, app.scene.vertex_count, file);
	fwrite(app.scene.draws, sizeof(app.scene.draws[0]), app.scene.draw_count, file);
	fwrite(app.atlas.pixels, sizeof(uint32_t), (size_t)app.atlas.width * (size_t)app.atlas.height, file);
	fclose(file);
	printf("preview: %zu vertices, %zu draws, level %s\n", app.scene.vertex_count, app.scene.draw_count, sm_level_name(app.level));
	return 0;
}

/* Takes the frames due, as the monitor does. */
static void
take_frames(
	struct sm_app *app,
	uint64_t now)
{
	struct sm_frame frame;
	enum sm_level level;
	int took;

	/* Each frame due. */
	for (;;) {
		took = sm_source_take(&app->source, now, &frame);
		if (!took)
			break;
		app->frame = frame;
		app->have_frame = 1;
		app->frame_at_ms = frame.time_ms;
		sm_history_add(&app->history, &app->source.info, &frame);
		level = sm_rules_update(&app->rules, &app->source.info, &frame, app->source.kind == SM_SOURCE_SIM);
		if (level != app->level && app->event_count < SM_EVENTS_MAX) {
			app->events[app->event_count].time_ms = frame.time_ms;
			app->events[app->event_count].level = level;
			(void)snprintf(app->events[app->event_count].text, sizeof(app->events[0].text), "%s: %s", sm_level_name(level), app->rules.summary);
			app->event_count++;
			app->level = level;
		}
	}
}
