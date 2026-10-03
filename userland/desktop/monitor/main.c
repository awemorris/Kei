/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The System Monitor (plan/ws134/design.md): a Keiland window that shows
 * the machine's CPU, GPU, memory, network and disks as plates in layers,
 * with the system's state graded at its centre.
 *
 *	monitor [--source=sim|replay:FILE] [--seed=N] [--period-ms=N] [--fps=N]
 *		[--size=WxH] [--cpus=N] [--gpus=N] [--calm] [--clock=fixed:MS]
 *		[--range=0..3] [--timeout-s=N] [--token=T]
 *
 * The window is libkeiui's (KUI_PRESENT_NONE) and the drawing the
 * monitor's own Vulkan (render.c), like Notes.  A frame is drawn only once
 * the compositor has shown the last one: the monitor asks for a frame
 * callback with each frame and waits for it before the next, so a hidden
 * window costs nothing and never makes the WSI's present wait (design.md
 * section 4.3).
 *
 * The log (stdout) is what the tests read: ZMON READY, SAMPLE, TEXT (each
 * plate's value as drawn), LEVEL, VISIBLE, FRAME (the frame rate and
 * times every 5 seconds), MEM (the monitor's own allocations every 30
 * seconds) and DONE.  --clock=fixed:MS stops the clock at MS after
 * playing the source up to it, so the picture is the same every run.
 */

#include "app.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "userland/desktop/paths.h"

/* The fonts: the interface's and the monospaced one for the values. */
#define MAIN_FONT		KEILAND_DATADIR "/fonts/keiland.ttf"
#define MAIN_FONT_MONO		KEILAND_DATADIR "/fonts/keiland-mono.ttf"
#define MAIN_FONT_FALLBACK	KEILAND_DATADIR "/fonts/keiland-fallback.ttf"

/* The evdev codes of the keys the monitor takes. */
#define MAIN_KEY_ESC		1U
#define MAIN_KEY_Q		16U
#define MAIN_KEY_LEFT		105U
#define MAIN_KEY_RIGHT		106U

/* How long without the compositor's frame callback means the window is hidden, and the reports' periods. */
#define MAIN_HIDDEN_MS		1000U
#define MAIN_FRAME_REPORT_MS	5000U
#define MAIN_MEMORY_REPORT_MS	30000U

/*
 * The titlebar's controls: the time ranges, as text pills (a segmented
 * group's faces are icons in zdesktop, and a text control in one shows as
 * "...").
 */
#define MAIN_CONTROL_RANGE	1U

/* The history the simulation fills before the window shows, in milliseconds (the 5 minutes' range). */
#define MAIN_PREFILL_MS		300000U

static const char *const main_range_labels[SM_RANGES] = { "1 min", "5 min", "15 min", "1 h" };

static int main_options(struct sm_app *app, int argc, char **argv);
static int main_option(struct sm_app *app, const char *argument);
static int main_open(struct sm_app *app);
static void main_close(struct sm_app *app);
static void main_titlebar(struct sm_app *app);
static void main_control_activated(void *data, struct keiland_titlebar *titlebar, uint32_t id, uint32_t detail, struct wl_seat *seat, uint32_t serial);
static void main_frame_done(void *data, struct wl_callback *callback, uint32_t time);
static uint64_t main_now(const struct sm_app *app);
static void main_take_frames(struct sm_app *app, uint64_t now);
static void main_event(struct sm_app *app, uint64_t time_ms, enum sm_level level, const char *text);
static void main_input(struct sm_app *app);
static int main_resize(struct sm_app *app);
static int main_draw(struct sm_app *app, uint64_t now);
static int main_timeout(const struct sm_app *app, uint64_t now);
static void main_reports(struct sm_app *app, uint64_t now);
static void main_set_range(struct sm_app *app, unsigned range);

static const struct keiland_titlebar_listener main_titlebar_listener = {
	main_control_activated, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static const struct wl_callback_listener main_frame_listener = {
	main_frame_done
};

/*
 * Runs the monitor until its window closes, its time runs out or the
 * connection breaks.
 */
int
main(
	int argc,
	char **argv)
{
	static struct sm_app app;
	uint64_t elapsed;
	uint64_t now;
	int status;
	int timeout;

	/* The options. */
	status = main_options(&app, argc, argv);
	if (status != 0)
		return 2;

	/* The window, the drawing and the source. */
	status = main_open(&app);
	if (status != 0) {
		main_close(&app);
		printf("ZMON FAILED\n");
		return 1;
	}

	/* The main loop: wait, take what arrived, draw when allowed and needed. */
	while (!app.quit) {
		/* Waits for the compositor until the next thing that is due. */
		now = main_now(&app);
		timeout = main_timeout(&app, now);
		status = kui_window_dispatch(app.window, timeout);
		if (status != 0) {
			printf("ZMON DISCONNECTED\n");
			break;
		}

		/* The window's input and the source's new frames. */
		main_input(&app);
		now = main_now(&app);
		main_take_frames(&app, now);

		/* A window that has not shown a frame for a while is hidden. */
		if (!app.frame_allowed && app.visible && now >= app.frame_asked_ms + MAIN_HIDDEN_MS) {
			app.visible = 0;
			printf("ZMON VISIBLE 0\n");
		}

		/* A frame when the last one was shown and something moved. */
		status = main_draw(&app, now);
		if (status != 0)
			break;

		/* The reports, and the end of the run. */
		main_reports(&app, now);
		elapsed = (kui_clock_us() - app.start_us) / 1000U;
		if (app.timeout_ms != 0U && elapsed >= app.timeout_ms)
			app.quit = 1;
		fflush(stdout);
	}

	/* The last line, and everything released. */
	printf("ZMON DONE frames=%llu\n", (unsigned long long)app.frames);
	fflush(stdout);
	main_close(&app);
	return 0;
}

/* Reads the command line; returns 0, or 2 after printing the usage. */
static int
main_options(
	struct sm_app *app,
	int argc,
	char **argv)
{
	int index;
	int status;

	/* The defaults (design.md section 4.3). */
	app->seed = 1;
	app->period_ms = 1000;
	app->fps = 30;
	app->width = 1200;
	app->height = 760;
	app->cpus = 0;
	app->gpus = 1;
	app->range = 1;

	/* Each option. */
	for (index = 1; index < argc; index++) {
		status = main_option(app, argv[index]);
		if (status != 0) {
			fprintf(stderr, "usage: monitor [--source=sim|replay:FILE] [--seed=N] [--period-ms=N] [--fps=N] [--size=WxH]\n"
				"               [--cpus=N] [--gpus=N] [--calm] [--clock=fixed:MS] [--range=0..3] [--timeout-s=N] [--token=T]\n");
			return 2;
		}
	}

	/* Sane bounds. */
	if (app->period_ms < 100U)
		app->period_ms = 100U;
	if (app->fps == 0U || app->fps > 60U)
		app->fps = 30U;
	return 0;
}

/* Reads one option; returns 0, or -1 for one the monitor does not know. */
static int
main_option(
	struct sm_app *app,
	const char *argument)
{
	static const char *const names[] = {
		"--source=", "--seed=", "--period-ms=", "--fps=", "--size=", "--cpus=", "--gpus=", "--calm", "--clock=fixed:",
		"--range=", "--timeout-s=", "--token="
	};
	const char *value;
	size_t length;
	unsigned option;
	int match;
	int read;

	/* Which option the argument starts with; its value follows. */
	value = NULL;
	for (option = 0; option < sizeof(names) / sizeof(names[0]); option++) {
		length = strlen(names[option]);
		match = strncmp(argument, names[option], length);
		if (match == 0) {
			value = argument + length;
			break;
		}
	}

	/* An unknown option. */
	if (value == NULL)
		return -1;

	/* Each option's value. */
	switch (option) {
	case 0:
		/* A recording, or the simulation. */
		match = strncmp(value, "replay:", 7);
		if (match == 0)
			app->replay_path = value + 7;
		break;
	case 1:
		app->seed = strtoull(value, NULL, 10);
		break;
	case 2:
		app->period_ms = (unsigned)strtoul(value, NULL, 10);
		break;
	case 3:
		app->fps = (unsigned)strtoul(value, NULL, 10);
		break;
	case 4:
		/* The window's size, WIDTHxHEIGHT. */
		read = sscanf(value, "%ux%u", &app->width, &app->height);
		if (read != 2)
			return -1;
		break;
	case 5:
		app->cpus = (unsigned)strtoul(value, NULL, 10);
		break;
	case 6:
		app->gpus = (unsigned)strtoul(value, NULL, 10);
		break;
	case 7:
		app->calm = 1;
		break;
	case 8:
		app->fixed_clock = 1;
		app->fixed_ms = strtoull(value, NULL, 10);
		break;
	case 9:
		app->range = (unsigned)strtoul(value, NULL, 10) % SM_RANGES;
		break;
	case 10:
		app->timeout_ms = strtoull(value, NULL, 10) * 1000U;
		break;
	default:
		app->token = value;
		break;
	}

	/* Succeeded: the option is read. */
	return 0;
}

/* Opens the window and its titlebar, the fonts and the atlas, the renderer, and the source; returns 0 or -1. */
static int
main_open(
	struct sm_app *app)
{
	struct kui_window_options options;
	const char *source_name;
	const char *token;
	uint32_t width;
	uint32_t height;
	uint64_t time;
	int error;
	VkResult result;

	/* The window, drawn by the monitor's own Vulkan. */
	app->start_us = kui_clock_us();
	memset(&options, 0, sizeof(options));
	options.title = "System Monitor";
	options.application = "monitor";
	options.width = app->width;
	options.height = app->height;
	options.present = KUI_PRESENT_NONE;
	app->window = kui_window_open(&options);
	if (app->window == NULL) {
		fprintf(stderr, "monitor: no window (errno %d)\n", errno);
		return -1;
	}

	/* Its objects, its size and its titlebar. */
	app->display = kui_window_display(app->window);
	app->surface = kui_window_surface(app->window);
	kui_window_size(app->window, &width, &height);
	app->width = width;
	app->height = height;
	main_titlebar(app);

	/* The fonts: the interface's, and the monospaced one (the interface's again when it is missing). */
	error = kui_text_open(&app->sans, MAIN_FONT, MAIN_FONT_FALLBACK);
	if (error != 0) {
		fprintf(stderr, "monitor: %s: error %d\n", MAIN_FONT, error);
		return -1;
	}

	/* The monospaced one, or the interface's again. */
	error = kui_text_open(&app->mono, MAIN_FONT_MONO, MAIN_FONT_FALLBACK);
	if (error != 0)
		error = kui_text_open(&app->mono, MAIN_FONT, MAIN_FONT_FALLBACK);
	if (error != 0)
		return -1;

	/* The renderer. */
	result = sm_renderer_open(&app->renderer, app->display, app->surface, width, height);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "monitor: %s failed (%d)\n", app->renderer.operation, (int)result);
		return -1;
	}

	/* The layout, the atlas at its scale, and its image. */
	sm_layout_compute(&app->layout, (float)app->renderer.extent.width, (float)app->renderer.extent.height);
	error = sm_atlas_build(&app->atlas, &app->sans, &app->mono, app->layout.scale);
	if (error != 0)
		return -1;
	result = sm_renderer_atlas(&app->renderer, &app->atlas);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "monitor: %s failed (%d)\n", app->renderer.operation, (int)result);
		return -1;
	}

	/* The source: a recording, or the simulation. */
	if (app->replay_path != NULL) {
		error = sm_source_open_replay(&app->source, app->replay_path, app->period_ms);
		if (error != 0) {
			fprintf(stderr, "monitor: %s: error %d\n", app->replay_path, error);
			return -1;
		}
	} else {
		(void)sm_source_open_sim(&app->source, app->seed, app->period_ms, app->cpus, app->gpus, app->calm);

		/* The simulated GPU is named after the device that draws the window. */
		if (app->source.info.gpu_count != 0U)
			(void)snprintf(app->source.info.gpu_name[0], SM_NAME_MAX, "%s", app->renderer.device_name);
	}

	/* The history and the rules start empty and calm. */
	sm_history_init(&app->history);
	sm_rules_init(&app->rules);
	app->level = SM_LEVEL_NORMAL;
	app->frame_allowed = 1;
	app->visible = 1;
	app->dirty = 1;

	/* A stopped clock plays the source up to its time first; the simulation fills its graphs' history before it shows. */
	if (app->fixed_clock) {
		for (time = 0; time <= app->fixed_ms; time += app->period_ms)
			main_take_frames(app, time);
	} else if (app->replay_path == NULL) {
		for (time = 0; time < MAIN_PREFILL_MS; time += app->period_ms)
			main_take_frames(app, time);
		app->clock_offset_ms = MAIN_PREFILL_MS;
	}

	/* The tests' first line. */
	source_name = "sim";
	if (app->replay_path != NULL)
		source_name = "replay";
	token = "-";
	if (app->token != NULL)
		token = app->token;
	printf("ZMON READY width=%u height=%u source=%s cpus=%u gpus=%u device=\"%s\" token=%s\n", app->renderer.extent.width,
	       app->renderer.extent.height, source_name, app->source.info.cpu_count, app->source.info.gpu_count,
	       app->renderer.device_name, token);
	fflush(stdout);
	return 0;
}

/* Releases everything the monitor holds. */
static void
main_close(
	struct sm_app *app)
{
	/* The drawing, the data and the window. */
	sm_renderer_close(&app->renderer);
	sm_scene_release(&app->scene);
	sm_atlas_release(&app->atlas);
	sm_source_close(&app->source);
	if (app->frame_callback != NULL)
		wl_callback_destroy(app->frame_callback);
	if (app->titlebar != NULL)
		keiland_titlebar_destroy(app->titlebar);
	kui_text_close(&app->sans);
	kui_text_close(&app->mono);
	if (app->window != NULL)
		kui_window_close(app->window);
	app->window = NULL;
}

/* Gives the window its titlebar: the time ranges as a segmented group of controls (design.md section 2.1). */
static void
main_titlebar(
	struct sm_app *app)
{
	unsigned range;

	/* A compositor without the titlebar presentation leaves the plain title. */
	app->titlebar = keiland_titlebar_create(app->display, kui_window_toplevel(app->window), &main_titlebar_listener, app);
	if (app->titlebar == NULL) {
		printf("ZMON TITLEBAR none errno=%d\n", errno);
		return;
	}

	/* The four ranges, the current one checked. */
	(void)keiland_titlebar_begin(app->titlebar);
	(void)keiland_titlebar_set_mode(app->titlebar, KEILAND_TITLEBAR_CONTROLS);
	for (range = 0; range < SM_RANGES; range++) {
		(void)keiland_titlebar_add_control(app->titlebar, MAIN_CONTROL_RANGE + range, KEILAND_CONTROL_GENERIC, KEILAND_PRIORITY_NORMAL,
						   0U, main_range_labels[range]);
		(void)keiland_titlebar_set_control_state(app->titlebar, MAIN_CONTROL_RANGE + range, 1, range == app->range);
	}

	/* Shown together. */
	(void)keiland_titlebar_commit(app->titlebar);
}

/* A titlebar control was chosen: a time range. */
static void
main_control_activated(
	void *data,
	struct keiland_titlebar *titlebar,
	uint32_t id,
	uint32_t detail,
	struct wl_seat *seat,
	uint32_t serial)
{
	struct sm_app *app;

	UNUSED_PARAMETER(titlebar);
	UNUSED_PARAMETER(detail);
	UNUSED_PARAMETER(seat);
	UNUSED_PARAMETER(serial);

	/* One of the ranges. */
	app = data;
	if (id >= MAIN_CONTROL_RANGE && id < MAIN_CONTROL_RANGE + SM_RANGES)
		main_set_range(app, id - MAIN_CONTROL_RANGE);
}

/* The compositor showed the last frame: the next may be drawn. */
static void
main_frame_done(
	void *data,
	struct wl_callback *callback,
	uint32_t time)
{
	struct sm_app *app;

	UNUSED_PARAMETER(time);

	/* The callback is spent. */
	app = data;
	wl_callback_destroy(callback);
	app->frame_callback = NULL;
	app->frame_allowed = 1;

	/* A hidden window that shows again says so. */
	if (!app->visible) {
		app->visible = 1;
		app->dirty = 1;
		printf("ZMON VISIBLE 1\n");
	}
}

/* The monitor's clock in milliseconds since it started (the stopped one in a test). */
static uint64_t
main_now(
	const struct sm_app *app)
{
	/* A stopped clock stays. */
	if (app->fixed_clock)
		return app->fixed_ms;

	/* Succeeded: the time since the start (after the history the simulation filled). */
	return (kui_clock_us() - app->start_us) / 1000U + app->clock_offset_ms;
}

/* Takes every frame the source has due, into the history, the rules and the events. */
static void
main_take_frames(
	struct sm_app *app,
	uint64_t now)
{
	struct sm_frame frame;
	enum sm_level level;
	char text[80];
	int took;

	/* Each frame due. */
	for (;;) {
		took = sm_source_take(&app->source, now, &frame);
		if (!took)
			break;
		app->frame = frame;
		app->have_frame = 1;
		app->frame_at_ms = now;
		if (app->fixed_clock)
			app->frame_at_ms = frame.time_ms;
		sm_history_add(&app->history, &app->source.info, &frame);
		app->dirty = 1;

		/* The state, and an event when it changes. */
		level = sm_rules_update(&app->rules, &app->source.info, &frame, app->source.kind == SM_SOURCE_SIM);
		if (level != app->level) {
			if (level > app->level)
				(void)snprintf(text, sizeof(text), "%s: %s", sm_level_name(level), app->rules.summary);
			else
				(void)snprintf(text, sizeof(text), "Back to %s", sm_level_name(level));
			main_event(app, frame.time_ms, level, text);
			printf("ZMON LEVEL t=%llu level=%s cause=\"%s\"\n", (unsigned long long)frame.time_ms, sm_level_name(level), app->rules.summary);
			app->level = level;
		}

		/* The sample's line. */
		printf("ZMON SAMPLE t=%llu cpu=%.3f used=%llu rx=%.0f tx=%.0f read=%.0f write=%.0f latency=%.2f gpu=%.3f valid=0x%x simulated=0x%x\n",
		       (unsigned long long)frame.time_ms, frame.cpu, (unsigned long long)frame.memory_used, frame.rx_rate, frame.tx_rate,
		       frame.read_rate, frame.write_rate, frame.disk_latency_ms, frame.gpu_busy[0], frame.valid, frame.simulated);
	}
}

/* Adds an event to the Events strip's list. */
static void
main_event(
	struct sm_app *app,
	uint64_t time_ms,
	enum sm_level level,
	const char *text)
{
	struct sm_event *event;

	/* The next slot of the ring. */
	event = &app->events[app->event_count % SM_EVENTS_MAX];
	event->time_ms = time_ms;
	event->level = level;
	(void)snprintf(event->text, sizeof(event->text), "%s", text);
	app->event_count++;
}

/* Takes the window's input: its size, its close, and the keys. */
static void
main_input(
	struct sm_app *app)
{
	struct kui_window_event event;
	int status;
	int took;

	/* Each input queued. */
	for (;;) {
		took = kui_window_take(app->window, &event);
		if (!took)
			break;
		switch (event.kind) {
		case KUI_WINDOW_RESIZE:
			status = main_resize(app);
			if (status != 0)
				app->quit = 1;
			break;
		case KUI_WINDOW_CLOSE:
			app->quit = 1;
			break;
		case KUI_WINDOW_KEY:
			/* A key's press: the ranges, and the way out. */
			if (!event.pressed)
				break;
			if (event.code == MAIN_KEY_LEFT && app->range > 0U)
				main_set_range(app, app->range - 1U);
			else if (event.code == MAIN_KEY_RIGHT && app->range + 1U < SM_RANGES)
				main_set_range(app, app->range + 1U);
			else if (event.code == MAIN_KEY_Q && (event.modifiers & KUI_MOD_CTRL) != 0U)
				app->quit = 1;
			break;
		default:
			break;
		}
	}
}

/* Follows the window's new size: the swapchain, the layout and (at a new scale) the atlas. Returns 0 or -1. */
static int
main_resize(
	struct sm_app *app)
{
	uint32_t width;
	uint32_t height;
	float scale;
	int error;
	VkResult result;

	/* The size the compositor gave. */
	kui_window_size(app->window, &width, &height);
	if (width == 0U || height == 0U)
		return 0;
	app->width = width;
	app->height = height;

	/* The swapchain at it. */
	result = sm_renderer_resize(&app->renderer, width, height);
	if (result != VK_SUCCESS) {
		fprintf(stderr, "monitor: %s failed (%d)\n", app->renderer.operation, (int)result);
		return -1;
	}

	/* The layout, and the atlas when the text's scale changed. */
	scale = app->layout.scale;
	sm_layout_compute(&app->layout, (float)app->renderer.extent.width, (float)app->renderer.extent.height);
	if (app->layout.scale != scale) {
		error = sm_atlas_build(&app->atlas, &app->sans, &app->mono, app->layout.scale);
		if (error != 0)
			return -1;
		result = sm_renderer_atlas(&app->renderer, &app->atlas);
		if (result != VK_SUCCESS)
			return -1;
	}

	/* Succeeded: the next frame at the new size. */
	printf("ZMON RESIZE width=%u height=%u\n", app->renderer.extent.width, app->renderer.extent.height);
	app->dirty = 1;
	return 0;
}

/* Draws a frame when the compositor allows one and something needs it; returns 0, or -1 when drawing failed for good. */
static int
main_draw(
	struct sm_app *app,
	uint64_t now)
{
	uint64_t wait_us;
	uint64_t now_us;
	int moving;
	int error;
	VkResult result;

	/* Not before the compositor showed the last frame. */
	if (!app->frame_allowed)
		return 0;

	/* The graphs slide between samples (not with the clock stopped): a frame every 1/fps while nothing else changed. */
	moving = 0;
	if (!app->fixed_clock && app->have_frame && app->visible)
		moving = 1;
	if (!app->dirty) {
		/* Nothing moves, or its next frame is not due yet. */
		if (!moving)
			return 0;
		now_us = kui_clock_us();
		if (now_us < app->next_frame_us)
			return 0;
	}

	/* The scene. */
	error = sm_scene_build(app, now);
	if (error != 0)
		return -1;

	/* The next frame waits for the compositor's callback for this one, asked before the present commits it. */
	app->frame_callback = wl_surface_frame(app->surface);
	if (app->frame_callback != NULL) {
		(void)wl_callback_add_listener(app->frame_callback, &main_frame_listener, app);
		app->frame_allowed = 0;
		app->frame_asked_ms = now;
	}

	/* The frame; an outdated swapchain or a lost surface is made again and the frame drawn next time. */
	wait_us = 0;
	result = sm_renderer_draw(&app->renderer, &app->scene, &wait_us);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		printf("ZMON SWAPCHAIN outdated\n");
		error = main_resize(app);
		app->frame_allowed = 1;
		return error;
	}

	/* A lost surface is made again with its swapchain. */
	if (result == VK_ERROR_SURFACE_LOST_KHR) {
		printf("ZMON SURFACE lost\n");
		result = sm_renderer_recover(&app->renderer, app->display, app->surface, app->width, app->height);
		app->frame_allowed = 1;
		app->dirty = 1;
		if (result != VK_SUCCESS)
			return -1;
		return 0;
	}

	/* Any other failure ends the monitor. */
	if (result != VK_SUCCESS) {
		fprintf(stderr, "monitor: %s failed (%d)\n", app->renderer.operation, (int)result);
		return -1;
	}

	/* Counted for the frame report; the next frame not before 1/fps. */
	app->frames++;
	app->frame_wait_us += wait_us;
	app->next_frame_us = kui_clock_us() + 1000000U / app->fps;
	app->dirty = 0;
	return 0;
}

/* How long the loop may wait, in milliseconds: until the next sample, the next frame, or the hidden check. */
static int
main_timeout(
	const struct sm_app *app,
	uint64_t now)
{
	uint64_t due;
	uint64_t now_us;
	int wait;

	/* A frame waiting to be drawn and allowed is due now. */
	if (app->dirty && app->frame_allowed)
		return 0;

	/* With the clock stopped only the compositor wakes the loop (and the run's end). */
	if (app->fixed_clock)
		return 1000;

	/* The next sample. */
	due = app->source.next_ms;
	if (app->source.kind == SM_SOURCE_REPLAY)
		due = app->source.replay.pending.time_ms;
	wait = 1000;
	if (due > now && due - now < (uint64_t)wait)
		wait = (int)(due - now);
	else if (due <= now)
		wait = 0;

	/* The next frame of the sliding graphs, when one is allowed. */
	if (app->frame_allowed && app->have_frame && app->visible) {
		now_us = kui_clock_us();
		if (app->next_frame_us <= now_us)
			return 0;
		if ((app->next_frame_us - now_us) / 1000U < (uint64_t)wait)
			wait = (int)((app->next_frame_us - now_us) / 1000U);
	}

	/* Succeeded: the wait. */
	return wait;
}

/* Prints the frame report every 5 seconds and the memory report every 30. */
static void
main_reports(
	struct sm_app *app,
	uint64_t now)
{
	static uint64_t last_frames;
	static uint64_t last_us;
	uint64_t now_us;
	uint64_t frames;
	uint64_t heap;
	double seconds;
	double wait_ms;

	UNUSED_PARAMETER(now);

	/* The frame rate and the mean wait for the GPU since the last report. */
	now_us = kui_clock_us();
	if (last_us == 0U)
		last_us = now_us;
	if (now_us >= last_us + (uint64_t)MAIN_FRAME_REPORT_MS * 1000U) {
		frames = app->frames - last_frames;
		seconds = (double)(now_us - last_us) / 1000000.0;
		wait_ms = 0.0;
		if (frames != 0U)
			wait_ms = (double)app->frame_wait_us / 1000.0 / (double)frames;
		printf("ZMON FRAME fps=%.1f wait_ms=%.2f visible=%d\n", (double)frames / seconds, wait_ms, app->visible);
		app->frame_wait_us = 0;
		last_frames = app->frames;
		last_us = now_us;
	}

	/* The monitor's own allocations: the scene, the atlas, the history, and the device memory. */
	if (now_us / 1000U >= app->memory_report_ms + MAIN_MEMORY_REPORT_MS) {
		heap = (uint64_t)app->scene.vertex_capacity * SM_VERTEX_FLOATS * sizeof(float) +
		    (uint64_t)app->scene.draw_capacity * sizeof(struct sm_draw) +
		    (uint64_t)app->atlas.width * (uint64_t)app->atlas.height * sizeof(uint32_t) + sizeof(app->history);
		printf("ZMON MEM heap=%llu vulkan=%llu\n", (unsigned long long)heap, (unsigned long long)app->renderer.allocated_bytes);
		app->memory_report_ms = now_us / 1000U;
	}
}

/* Shows another time range: the graphs and the titlebar's checked control. */
static void
main_set_range(
	struct sm_app *app,
	unsigned range)
{
	unsigned index;

	/* The range. */
	app->range = range;
	app->dirty = 1;
	printf("ZMON RANGE %s\n", main_range_labels[range]);

	/* The titlebar shows it checked. */
	if (app->titlebar == NULL)
		return;
	(void)keiland_titlebar_begin(app->titlebar);
	for (index = 0; index < SM_RANGES; index++)
		(void)keiland_titlebar_set_control_state(app->titlebar, MAIN_CONTROL_RANGE + index, 1, index == range);
	(void)keiland_titlebar_commit(app->titlebar);
}
