/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The sound volume in the glass look's system bar (ws100-p004,
 * plan/ws100/design.md section 3.4): an icon left of the network's that
 * shows the volume (a speaker with no to three waves, a cross when muted,
 * struck through and pale with no sound), and a popup under it with a
 * slider (0 to 100) and a mute switch.
 *
 * A click on the icon opens the popup; a press or drag on the slider sets
 * the volume, a press on the mute row switches it; a click elsewhere, or
 * Esc, closes it.  The wheel over the icon (or the open popup) moves the
 * volume by VOLUME_WHEEL_STEP a notch.  Each change plays audiod's short
 * feedback sound (a drag at most every VOLUME_FEEDBACK_MS, and once at its
 * end), and is kept in the user's preferences (sound.volume, sound.muted)
 * a moment after it settles.  The preferences are applied when audiod is
 * reached and whenever they change.
 *
 * All of it goes through libkeiland (keiland_audio_*): zdesktop never
 * speaks audiod's protocol, and nothing here waits for it.
 */

#include "glass.h"
#include "titlebar.h"
#include <keiland.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A parameter a function does not use. */
#define UNUSED_PARAMETER(name)	((void)(name))

/* The evdev code of Esc. */
#define VOLUME_KEY_ESC		1U

/* The popup's size, padding, rows and corner radius. */
#define VOLUME_POPUP_WIDTH	260
#define VOLUME_PADDING		14
#define VOLUME_TITLE_HEIGHT	34
#define VOLUME_SLIDER_HEIGHT	34
#define VOLUME_ROW_HEIGHT	34
#define VOLUME_NOTE_HEIGHT	26
#define VOLUME_RADIUS		12.0f

/* The slider's track and knob. */
#define VOLUME_TRACK_HEIGHT	6
#define VOLUME_KNOB		18

/* The wheel: percent a notch (zwl_seat_axis counts notches, as evdev's wheel does). */
#define VOLUME_WHEEL_STEP	5

/* How often a drag sends the volume, how often it plays the feedback sound, and when a change is kept, in milliseconds. */
#define VOLUME_SEND_MS		50U
#define VOLUME_FEEDBACK_MS	250U
#define VOLUME_SAVE_MS		1000U

/*
 * How long after zdesktop kept the volume a change of the file is taken as
 * that write read back, in milliseconds: the preferences' watcher looks
 * once a second and the event loop takes its reading at its next pass, and
 * the two keys are written one after the other (BUG-153).
 */
#define VOLUME_ECHO_MS		3000U

/* The preferences' keys. */
#define VOLUME_KEY_VOLUME	"sound.volume"
#define VOLUME_KEY_MUTED	"sound.muted"

/*
 * The volume's side of the system bar: audiod's link in libkeiland (made
 * on the desktop's first tick), what it last reported, the volume shown
 * (while a drag or a wheel leads, ahead of audiod's report), whether the
 * popup is open and where, where the icon was drawn, whether the slider
 * is dragged, the times of the last send and feedback, what is waiting to
 * be sent or played, when the change is kept, and whether the preferences
 * have been applied to this connection.
 *
 * saved, saved_value and saved_muted are what zdesktop itself last kept
 * in the preferences (saved is 0 until it has kept anything), and
 * saved_ms when.  A change of the file that brings these back, or comes
 * within VOLUME_ECHO_MS of that write, is zdesktop's own write read again
 * and is not applied (BUG-153: it arrived up to two seconds after the
 * write and set audiod back to the kept volume over a newer click).
 */
struct volume_view {
	struct keiland_audio *audio;
	unsigned opened;
	struct keiland_audio_state state;
	unsigned value;
	unsigned muted;
	unsigned open;
	int32_t popup_x;
	int32_t popup_y;
	int32_t popup_height;
	int32_t icon_x;
	int32_t icon_y;
	int32_t icon_width;
	int32_t icon_height;
	unsigned icon_logged;
	unsigned dragging;
	uint64_t sent_ms;
	uint64_t feedback_ms;
	unsigned send_waiting;
	unsigned feedback_waiting;
	uint64_t save_ms;
	unsigned applied;
	unsigned saved;
	unsigned saved_value;
	unsigned saved_muted;
	uint64_t saved_ms;
};

/*
 * The one view of the volume.  Only the event loop's thread touches it (the
 * ticks, the drawing, the input).
 */
static struct volume_view volume_view;

static void volume_open_popup(struct zwl_server *server);
static void volume_close_popup(struct zwl_server *server, const char *via);
static void volume_set(struct zwl_server *server, unsigned value, unsigned muted, const char *via, unsigned final);
static void volume_send(struct zwl_server *server);
static void volume_apply_preferences(struct zwl_server *server, unsigned from_file);
static void volume_save(struct zwl_server *server);
static int volume_sound(void);
static int volume_in_icon(int32_t x, int32_t y);
static int volume_in_popup(int32_t x, int32_t y);
static unsigned volume_slider_value(int32_t x);
static int32_t volume_slider_top(void);
static int32_t volume_mute_top(void);
static void volume_draw_switch(struct zwl_server *server, VkCommandBuffer command, int32_t right, int32_t middle, unsigned on, float fade);

/*
 * Reads what audiod has reported since the last tick, sends a volume a drag
 * held back, plays a feedback sound held back, and keeps a change whose
 * moment has come.  The link is made on the desktop's first tick.
 */
void
zwl_volume_tick(
	struct zwl_server *server)
{
	unsigned changed;
	uint64_t now;

	/* The link, once (libkeiland connects to audiod when it can). */
	if (!volume_view.opened) {
		volume_view.opened = 1U;
		volume_view.audio = keiland_audio_open();
		volume_view.icon_x = -1;
		volume_view.value = 100U;
	}

	/* No link could be made (no memory): the icon says there is no sound. */
	if (volume_view.audio == NULL)
		return;

	/* What arrived. */
	(void)keiland_audio_update(volume_view.audio, &changed);
	if (changed != 0U) {
		keiland_audio_get_state(volume_view.audio, &volume_view.state);
		server->dirty = 1;

		/* audiod reached or lost: the preferences are applied once per connection. */
		if ((changed & KEILAND_AUDIO_CHANGED_REACHABLE) != 0U) {
			printf("ZWL VOLUME reachable=%u device=%u\n", volume_view.state.reachable, volume_view.state.device);
			if (!volume_view.state.reachable)
				volume_view.applied = 0U;
		}

		/* A report shows the volume, unless a drag or a wheel leads. */
		if ((changed & KEILAND_AUDIO_CHANGED_VOLUME) != 0U && !volume_view.dragging && !volume_view.send_waiting) {
			volume_view.value = volume_view.state.left;
			volume_view.muted = volume_view.state.muted;
		}

		/* A connection with the volume known takes the preferences. */
		if (volume_view.state.reachable && !volume_view.applied && (changed & KEILAND_AUDIO_CHANGED_VOLUME) != 0U) {
			volume_view.applied = 1U;
			volume_apply_preferences(server, 0U);
		}
	}

	/* A volume or a sound held back. */
	now = zwl_milliseconds();
	if (volume_view.send_waiting && now - volume_view.sent_ms >= VOLUME_SEND_MS)
		volume_send(server);
	if (volume_view.feedback_waiting && now - volume_view.feedback_ms >= VOLUME_FEEDBACK_MS) {
		volume_view.feedback_waiting = 0U;
		volume_view.feedback_ms = now;
		(void)keiland_audio_feedback(volume_view.audio);
		printf("ZWL VOLUME feedback at_ms=%llu via=held\n", (unsigned long long)now);
	}

	/* A settled change is kept. */
	if (volume_view.save_ms != 0U && now >= volume_view.save_ms) {
		volume_view.save_ms = 0U;
		volume_save(server);
	}
}

/*
 * Applies the preferences' volume again after the file changed
 * (preferences.c): a volume kept elsewhere (Settings' Sound page).  A
 * change of the user's here that is not kept yet is newer than the file
 * and wins; the file follows it when it is kept (BUG-153).
 */
void
zwl_volume_preferences(
	struct zwl_server *server)
{
	/* Only once audiod has been reached (it is applied on reaching otherwise). */
	if (volume_view.audio == NULL || !volume_view.applied)
		return;

	/* A drag, or a change waiting to be kept, is newer than what the file holds. */
	if (volume_view.dragging || volume_view.save_ms != 0U) {
		printf("ZWL VOLUME preferences skipped reason=newer value=%u muted=%u\n", volume_view.value, volume_view.muted);
		return;
	}

	/* The preferences' volume, unless it is only zdesktop's own last write read back. */
	volume_apply_preferences(server, 1U);
}

/*
 * Draws the volume's icon in the system bar at x (its left edge), in the
 * bar's ink.
 */
void
zwl_volume_draw_icon(
	struct zwl_server *server,
	VkCommandBuffer command,
	int32_t x,
	const float *ink)
{
	static const float blue[4] = { 0.25f, 0.52f, 0.98f, 0.28f };
	float faint[4];
	unsigned icon;
	int sound;

	/* Where a click opens the popup (a little larger than the drawing). */
	volume_view.icon_x = x - 5;
	volume_view.icon_y = 3;
	volume_view.icon_width = 30;
	volume_view.icon_height = ZWL_GLASS_BAR - 6;
	if (!volume_view.icon_logged) {
		volume_view.icon_logged = 1U;
		printf("ZWL VOLUME icon x=%d y=%d width=%d height=%d\n", volume_view.icon_x, volume_view.icon_y, volume_view.icon_width, volume_view.icon_height);
	}

	/* While the popup is open its icon has a pale blue back. */
	if (volume_view.open)
		glass_draw_solid(server, command, (float)volume_view.icon_x, (float)volume_view.icon_y, (float)volume_view.icon_width, (float)volume_view.icon_height, 7.0f, blue);

	/* No sound: a pale speaker, struck through. */
	sound = volume_sound();
	if (!sound) {
		memcpy(faint, ink, sizeof(faint));
		faint[3] *= 0.35f;
		glass_draw_icon(server, command, GLASS_ICON_VOLUME_0, x, 7, 20U, faint);
		glass_draw_solid(server, command, (float)(x + 1), 16.0f, 18.0f, 2.0f, 1.0f, ink);
		return;
	}

	/* Muted, or as many waves as the volume is loud. */
	icon = GLASS_ICON_VOLUME_MUTED;
	if (!volume_view.muted) {
		icon = GLASS_ICON_VOLUME_3;
		if (volume_view.value <= 66U)
			icon = GLASS_ICON_VOLUME_2;
		if (volume_view.value <= 33U)
			icon = GLASS_ICON_VOLUME_1;
		if (volume_view.value == 0U)
			icon = GLASS_ICON_VOLUME_0;
	}

	/* The icon. */
	glass_draw_icon(server, command, icon, x, 7, 20U, ink);
}

/*
 * Draws the open popup under the icon: its shadow, its glass, the title
 * with the volume, the slider and the mute switch, or the reason there is
 * no sound.
 */
void
zwl_volume_draw_popup(
	struct zwl_server *server,
	VkCommandBuffer command)
{
	static const float dark[4] = { 0.12f, 0.16f, 0.24f, 1.0f };
	static const float soft[4] = { 0.34f, 0.38f, 0.46f, 1.0f };
	static const float track[4] = { 0.62f, 0.66f, 0.72f, 0.55f };
	static const float blue[4] = { 0.25f, 0.52f, 0.98f, 1.0f };
	static const float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const float edge[4] = { 0.12f, 0.16f, 0.24f, 0.25f };
	struct glass_shape shape;
	char text[32];
	float fade;
	float fill[4];
	int32_t left;
	int32_t width;
	int32_t top;
	int32_t knob;
	int sound;

	/* Only an open popup. */
	if (!volume_view.open)
		return;

	/* The shadow. */
	glass_shape_init(&shape, (float)volume_view.popup_x, (float)volume_view.popup_y + 6.0f, (float)VOLUME_POPUP_WIDTH, (float)volume_view.popup_height);
	shape.quad[0] -= 40.0f;
	shape.quad[1] -= 40.0f;
	shape.quad[2] += 80.0f;
	shape.quad[3] += 80.0f;
	shape.mode = MODE_SHADOW;
	shape.radius = VOLUME_RADIUS;
	shape.soft = 18.0f;
	shape.color[0] = 0.10f;
	shape.color[1] = 0.18f;
	shape.color[2] = 0.35f;
	shape.color[3] = 0.24f;
	glass_shape_draw(server, command, &shape);

	/* The glass, as white as the network's menu. */
	glass_shape_init(&shape, (float)volume_view.popup_x, (float)volume_view.popup_y, (float)VOLUME_POPUP_WIDTH, (float)volume_view.popup_height);
	shape.mode = MODE_GLASS;
	shape.radius = VOLUME_RADIUS;
	shape.color[0] = 1.0f;
	shape.color[1] = 1.0f;
	shape.color[2] = 1.0f;
	shape.color[3] = 0.86f;
	shape.edge = 0.85f;
	glass_shape_draw(server, command, &shape);

	/* The title, and the volume at the right (Muted, or the percent). */
	left = volume_view.popup_x + VOLUME_PADDING;
	width = VOLUME_POPUP_WIDTH - 2 * VOLUME_PADDING;
	top = volume_view.popup_y + VOLUME_PADDING / 2;
	glass_draw_text(server, command, SIZE_TITLE, left, top + 22, "Sound", width, dark);
	sound = volume_sound();
	if (sound) {
		(void)snprintf(text, sizeof(text), "%u%%", volume_view.value);
		if (volume_view.muted)
			(void)snprintf(text, sizeof(text), "Muted");
		glass_draw_text(server, command, SIZE_BAR, left + width - glass_text_width(server, SIZE_BAR, text), top + 22, text, width, soft);
	}

	/* The controls are pale and do nothing without sound. */
	fade = 1.0f;
	if (!sound)
		fade = 0.35f;

	/* The slider: the track, its filled part and the knob at the volume. */
	top = volume_slider_top();
	memcpy(fill, blue, sizeof(fill));
	fill[3] *= fade;
	glass_draw_solid(server, command, (float)left, (float)(top + (VOLUME_SLIDER_HEIGHT - VOLUME_TRACK_HEIGHT) / 2), (float)width, (float)VOLUME_TRACK_HEIGHT, 3.0f, track);
	knob = left + (int32_t)((unsigned)(width - VOLUME_KNOB) * volume_view.value / 100U);
	glass_draw_solid(server, command, (float)left, (float)(top + (VOLUME_SLIDER_HEIGHT - VOLUME_TRACK_HEIGHT) / 2), (float)(knob - left + VOLUME_KNOB / 2), (float)VOLUME_TRACK_HEIGHT, 3.0f, fill);
	glass_draw_solid(server, command, (float)knob - 1.0f, (float)(top + (VOLUME_SLIDER_HEIGHT - VOLUME_KNOB) / 2) - 1.0f, (float)VOLUME_KNOB + 2.0f, (float)VOLUME_KNOB + 2.0f, (float)VOLUME_KNOB / 2.0f + 1.0f, edge);
	glass_draw_solid(server, command, (float)knob, (float)(top + (VOLUME_SLIDER_HEIGHT - VOLUME_KNOB) / 2), (float)VOLUME_KNOB, (float)VOLUME_KNOB, (float)VOLUME_KNOB / 2.0f, white);

	/* The mute row: its name and switch. */
	top = volume_mute_top();
	glass_draw_text(server, command, SIZE_BAR, left, top + 22, "Mute", width, dark);
	volume_draw_switch(server, command, left + width, top + VOLUME_ROW_HEIGHT / 2, volume_view.muted, fade);

	/* Without sound, the reason under the controls. */
	if (!sound) {
		top += VOLUME_ROW_HEIGHT;
		if (!volume_view.state.reachable) {
			glass_draw_text(server, command, SIZE_BAR, left, top + 18, "Sound service is not running", width, soft);
		} else {
			glass_draw_text(server, command, SIZE_BAR, left, top + 18, "No sound output", width, soft);
		}
	}
}

/*
 * Handles a pointer button for the volume: a press on the icon opens or
 * closes the popup; while it is open, a press on the slider sets the volume
 * and follows a drag, a press on the mute row switches it, and a press
 * elsewhere closes the popup.  Returns 1 when the button was the volume's.
 */
int
zwl_volume_button(
	struct zwl_server *server,
	uint32_t button,
	uint32_t state)
{
	int32_t y;
	int32_t slider;
	int32_t mute;
	int inside;
	int sound;

	/* With the popup closed, only a left press on the icon. */
	if (!volume_view.open) {
		/* A release, or another button, goes on. */
		if (state == 0U || button != ZWL_BUTTON_LEFT)
			return 0;

		/* A press off the icon goes on. */
		inside = volume_in_icon(server->pointer_x, server->pointer_y);
		if (!inside)
			return 0;

		/* The popup opens. */
		volume_open_popup(server);
		return 1;
	}

	/* A release ends a drag: the last volume goes, with its sound. */
	if (state == 0U) {
		if (volume_view.dragging) {
			volume_view.dragging = 0U;
			volume_set(server, volume_view.value, volume_view.muted, "slider", 1U);
		}

		/* Every release is the popup's. */
		return 1;
	}

	/* A press on the icon closes it. */
	inside = volume_in_icon(server->pointer_x, server->pointer_y);
	if (inside) {
		volume_close_popup(server, "icon");
		return 1;
	}

	/* A press outside the popup closes it and goes no further. */
	inside = volume_in_popup(server->pointer_x, server->pointer_y);
	if (!inside) {
		volume_close_popup(server, "outside");
		return 1;
	}

	/* Without sound, or with another button, the controls do nothing. */
	sound = volume_sound();
	if (!sound || button != ZWL_BUTTON_LEFT)
		return 1;

	/* The rows' tops, and where the press is. */
	y = server->pointer_y;
	slider = volume_slider_top();
	mute = volume_mute_top();

	/* The slider: the volume under the pointer, then a drag. */
	if (y >= slider && y < slider + VOLUME_SLIDER_HEIGHT) {
		volume_view.dragging = 1U;
		volume_set(server, volume_slider_value(server->pointer_x), volume_view.muted, "slider", 0U);
		return 1;
	}

	/* The mute row switches mute. */
	if (y >= mute && y < mute + VOLUME_ROW_HEIGHT) {
		volume_set(server, volume_view.value, !volume_view.muted, "mute", 1U);
		return 1;
	}

	/* Succeeded: the press was the popup's. */
	return 1;
}

/*
 * Follows the pointer while the popup is open: a drag moves the volume.
 * Returns 1 when the motion was the volume's.
 */
int
zwl_volume_motion(
	struct zwl_server *server)
{
	/* A closed popup does not follow the pointer. */
	if (!volume_view.open)
		return 0;

	/* A drag sets the volume under the pointer. */
	if (volume_view.dragging)
		volume_set(server, volume_slider_value(server->pointer_x), volume_view.muted, "slider", 0U);

	/* Succeeded: the motion was the popup's. */
	return 1;
}

/*
 * Handles a key while the popup is open: Esc closes it, and the others are
 * the popup's too.  Returns 1 when the key was the volume's.
 */
int
zwl_volume_key(
	struct zwl_server *server,
	uint32_t key,
	uint32_t state)
{
	/* A closed popup takes no key. */
	if (!volume_view.open)
		return 0;

	/* Esc, pressed, closes it. */
	if (key == VOLUME_KEY_ESC && state != 0U)
		volume_close_popup(server, "key");

	/* Succeeded: the key was the popup's. */
	return 1;
}

/*
 * Moves the volume with the wheel over the icon or the open popup,
 * VOLUME_WHEEL_STEP a notch (up louder).  Returns 1 when the wheel was the
 * volume's.
 */
int
zwl_volume_axis(
	struct zwl_server *server,
	int32_t vertical,
	int32_t horizontal)
{
	int32_t notches;
	int32_t value;
	int over;
	int sound;

	UNUSED_PARAMETER(horizontal);

	/* Only over the icon, or the open popup. */
	over = volume_in_icon(server->pointer_x, server->pointer_y);
	if (!over && volume_view.open)
		over = volume_in_popup(server->pointer_x, server->pointer_y);
	if (!over)
		return 0;

	/* Without sound the wheel does nothing, and goes no further. */
	sound = volume_sound();
	if (!sound || vertical == 0)
		return 1;

	/* The notches; down (positive, in Wayland's direction) is quieter. */
	notches = vertical;

	/* The new volume within 0..100. */
	value = (int32_t)volume_view.value - notches * VOLUME_WHEEL_STEP;
	if (value < 0)
		value = 0;
	if (value > 100)
		value = 100;
	volume_set(server, (unsigned)value, volume_view.muted, "wheel", 1U);

	/* Succeeded: the wheel was the volume's. */
	return 1;
}

/*
 * Tells whether the popup is open (the look is not still while it is).
 */
int
zwl_volume_is_open(
	void)
{
	/* Open or not. */
	return (int)volume_view.open;
}

/* Opens the popup under the icon, kept on the output. */
static void
volume_open_popup(
	struct zwl_server *server)
{
	int sound;

	/* Its place: under the icon, its right edge not past the output's. */
	volume_view.popup_x = volume_view.icon_x + volume_view.icon_width / 2 - VOLUME_POPUP_WIDTH / 2;
	if (volume_view.popup_x + VOLUME_POPUP_WIDTH > (int32_t)server->width - 8)
		volume_view.popup_x = (int32_t)server->width - 8 - VOLUME_POPUP_WIDTH;
	if (volume_view.popup_x < 8)
		volume_view.popup_x = 8;
	volume_view.popup_y = ZWL_GLASS_BAR + 6;

	/* Its height: the title, the slider, the mute row, and a note without sound. */
	volume_view.popup_height = VOLUME_PADDING + VOLUME_TITLE_HEIGHT + VOLUME_SLIDER_HEIGHT + VOLUME_ROW_HEIGHT;
	sound = volume_sound();
	if (!sound)
		volume_view.popup_height += VOLUME_NOTE_HEIGHT;

	/* Open. */
	volume_view.open = 1U;
	volume_view.dragging = 0U;
	server->dirty = 1;
	printf("ZWL VOLUME popup open x=%d y=%d width=%d height=%d slider=%d mute=%d sound=%d\n", volume_view.popup_x, volume_view.popup_y, VOLUME_POPUP_WIDTH, volume_view.popup_height, volume_slider_top(), volume_mute_top(), sound);
}

/* Closes the popup. */
static void
volume_close_popup(
	struct zwl_server *server,
	const char *via)
{
	/* A drag in progress ends with its volume. */
	if (volume_view.dragging) {
		volume_view.dragging = 0U;
		volume_set(server, volume_view.value, volume_view.muted, "slider", 1U);
	}

	/* Closed. */
	volume_view.open = 0U;
	server->dirty = 1;
	printf("ZWL VOLUME popup close via=%s\n", via);
}

/*
 * Shows and sends a volume: a drag's steps are sent at most every
 * VOLUME_SEND_MS and play a sound at most every VOLUME_FEEDBACK_MS; a final
 * one (a wheel notch, the end of a drag, mute) is sent now and plays now,
 * except that switching mute on plays nothing.  The change is kept a moment
 * after it settles.
 */
static void
volume_set(
	struct zwl_server *server,
	unsigned value,
	unsigned muted,
	const char *via,
	unsigned final)
{
	uint64_t now;

	/* Nothing to do when nothing changes, except at a drag's end. */
	if (value == volume_view.value && muted == volume_view.muted && !final)
		return;

	/* Shown at once. */
	volume_view.value = value;
	volume_view.muted = muted;
	volume_view.send_waiting = 1U;
	server->dirty = 1;
	printf("ZWL VOLUME set value=%u muted=%u via=%s final=%u at_ms=%llu\n", value, muted, via, final, (unsigned long long)zwl_milliseconds());

	/* Sent now when final, or when a drag's wait is over. */
	now = zwl_milliseconds();
	if (final || now - volume_view.sent_ms >= VOLUME_SEND_MS)
		volume_send(server);

	/* The sound: now when final (not for muting), during a drag at most every VOLUME_FEEDBACK_MS. */
	if (!muted) {
		if (final || now - volume_view.feedback_ms >= VOLUME_FEEDBACK_MS) {
			volume_view.feedback_waiting = 0U;
			volume_view.feedback_ms = now;
			(void)keiland_audio_feedback(volume_view.audio);
			printf("ZWL VOLUME feedback at_ms=%llu via=%s\n", (unsigned long long)now, via);
		} else {
			volume_view.feedback_waiting = 1U;
		}
	}

	/* Kept a moment after it settles. */
	volume_view.save_ms = now + VOLUME_SAVE_MS;
}

/* Sends the volume shown to audiod. */
static void
volume_send(
	struct zwl_server *server)
{
	int error;

	UNUSED_PARAMETER(server);

	/* The request; one that cannot go is logged and dropped (the next report shows audiod's). */
	volume_view.send_waiting = 0U;
	volume_view.sent_ms = zwl_milliseconds();
	error = keiland_audio_set_volume(volume_view.audio, volume_view.value, volume_view.value, volume_view.muted);
	if (error != 0)
		printf("ZWL VOLUME send errno=%d\n", error);
}

/*
 * Sends the preferences' volume, when they have one, to audiod.  With
 * from_file (the file changed), the values zdesktop kept itself last are
 * its own write read back and are not sent again.
 */
static void
volume_apply_preferences(
	struct zwl_server *server,
	unsigned from_file)
{
	int32_t value;
	int32_t muted;
	char text[16];
	int error;

	/* Without preferences (the login screen), audiod's volume stays. */
	if (server->preferences == NULL)
		return;

	/* A volume that is not set leaves audiod's as it is. */
	error = keiland_preferences_get(server->preferences, VOLUME_KEY_VOLUME, text, sizeof(text));
	if (error != 0)
		return;
	value = keiland_preferences_get_int(server->preferences, VOLUME_KEY_VOLUME, 100, 0, 100);
	muted = keiland_preferences_get_int(server->preferences, VOLUME_KEY_MUTED, 0, 0, 1);

	/*
	 * zdesktop's own last write, read back (its values, or a reading taken
	 * while it was being written): nothing new.  Sending it would undo
	 * whatever audiod was set to since (BUG-153).
	 */
	if (from_file && volume_view.saved) {
		/* The values zdesktop kept. */
		if ((unsigned)value == volume_view.saved_value && (unsigned)muted == volume_view.saved_muted)
			return;

		/* A change of the file soon after zdesktop's write is that write. */
		if (zwl_milliseconds() - volume_view.saved_ms < VOLUME_ECHO_MS) {
			printf("ZWL VOLUME preferences skipped reason=echo value=%d muted=%d\n", value, muted);
			return;
		}
	}

	/* The same as audiod's: nothing to send. */
	if ((unsigned)value == volume_view.state.left && (unsigned)muted == volume_view.state.muted)
		return;

	/* Shown and sent, without a sound and without keeping it again. */
	volume_view.value = (unsigned)value;
	volume_view.muted = (unsigned)muted;
	volume_send(server);
	server->dirty = 1;
	printf("ZWL VOLUME preferences value=%d muted=%d\n", value, muted);
}

/* Keeps the volume in the preferences (sound.volume and sound.muted). */
static void
volume_save(
	struct zwl_server *server)
{
	char text[16];
	int error;

	/* Without preferences (the login screen) nothing is kept. */
	if (server->preferences == NULL)
		return;

	/* The two keys. */
	(void)snprintf(text, sizeof(text), "%u", volume_view.value);
	error = keiland_preferences_set(server->preferences, VOLUME_KEY_VOLUME, text);
	if (error == 0) {
		(void)snprintf(text, sizeof(text), "%u", volume_view.muted);
		error = keiland_preferences_set(server->preferences, VOLUME_KEY_MUTED, text);
	}

	/* What was kept, so that the file read back with it is known as zdesktop's own (BUG-153). */
	if (error == 0) {
		volume_view.saved = 1U;
		volume_view.saved_value = volume_view.value;
		volume_view.saved_muted = volume_view.muted;
		volume_view.saved_ms = zwl_milliseconds();
	}

	/* The log line the tests read. */
	printf("ZWL VOLUME saved value=%u muted=%u error=%d\n", volume_view.value, volume_view.muted, error);
}

/* Tells whether there is sound: audiod reached, with a device. */
static int
volume_sound(
	void)
{
	/* audiod, and its device. */
	if (!volume_view.state.reachable)
		return 0;
	if (!volume_view.state.device)
		return 0;

	/* Succeeded: there is sound. */
	return 1;
}

/* Tells whether a point is on the icon's area. */
static int
volume_in_icon(
	int32_t x,
	int32_t y)
{
	/* Not drawn yet. */
	if (volume_view.icon_x < 0)
		return 0;

	/* Within its rectangle. */
	if (x < volume_view.icon_x || x >= volume_view.icon_x + volume_view.icon_width)
		return 0;
	if (y < volume_view.icon_y || y >= volume_view.icon_y + volume_view.icon_height)
		return 0;

	/* Succeeded: on the icon. */
	return 1;
}

/* Tells whether a point is on the open popup. */
static int
volume_in_popup(
	int32_t x,
	int32_t y)
{
	/* Within its rectangle. */
	if (x < volume_view.popup_x || x >= volume_view.popup_x + VOLUME_POPUP_WIDTH)
		return 0;
	if (y < volume_view.popup_y || y >= volume_view.popup_y + volume_view.popup_height)
		return 0;

	/* Succeeded: on the popup. */
	return 1;
}

/* Gives the volume at a point of the slider, 0 at the track's left end and 100 at its right. */
static unsigned
volume_slider_value(
	int32_t x)
{
	int32_t left;
	int32_t width;
	int32_t offset;

	/* The knob's centre runs from the track's left end to its right end. */
	left = volume_view.popup_x + VOLUME_PADDING + VOLUME_KNOB / 2;
	width = VOLUME_POPUP_WIDTH - 2 * VOLUME_PADDING - VOLUME_KNOB;
	offset = x - left;
	if (offset < 0)
		offset = 0;
	if (offset > width)
		offset = width;

	/* Succeeded: the percent, rounded. */
	return (unsigned)((offset * 100 + width / 2) / width);
}

/* Gives the slider's row top. */
static int32_t
volume_slider_top(
	void)
{
	/* Under the title. */
	return volume_view.popup_y + VOLUME_PADDING / 2 + VOLUME_TITLE_HEIGHT;
}

/* Gives the mute row's top. */
static int32_t
volume_mute_top(
	void)
{
	/* Under the slider. */
	return volume_slider_top() + VOLUME_SLIDER_HEIGHT;
}

/* Draws a switch, its right edge at right, on or off, faded when it cannot be used. */
static void
volume_draw_switch(
	struct zwl_server *server,
	VkCommandBuffer command,
	int32_t right,
	int32_t middle,
	unsigned on,
	float fade)
{
	float pill[4] = { 0.62f, 0.66f, 0.72f, 1.0f };
	float white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	int32_t x;

	/* The pill: blue when on. */
	if (on) {
		pill[0] = 0.25f;
		pill[1] = 0.52f;
		pill[2] = 0.98f;
	}

	/* Faded when it cannot be used, and drawn. */
	pill[3] *= fade;
	white[3] *= fade;
	x = right - 36;
	glass_draw_solid(server, command, (float)x, (float)(middle - 10), 36.0f, 20.0f, 10.0f, pill);

	/* The knob, right when on. */
	if (on) {
		glass_draw_solid(server, command, (float)(x + 18), (float)(middle - 8), 16.0f, 16.0f, 8.0f, white);
	} else {
		glass_draw_solid(server, command, (float)(x + 2), (float)(middle - 8), 16.0f, 16.0f, 8.0f, white);
	}
}
