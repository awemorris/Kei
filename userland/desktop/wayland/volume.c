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
 * end).
 *
 * During a session audiod alone holds the volume: the system bar and
 * Settings' Sound page both set it there and follow what it reports, and
 * nothing is written to a file while the user changes it (BUG-161,
 * ws100-p012; 2026-10-03 user: no I/O for each change, keep it at the end
 * of the session).  The user's preferences (sound.volume, sound.muted) are
 * read once, when audiod is first reached, and applied; the volume is
 * written back once, when the session ends (Log Out, or zdesktop told to
 * stop), and only when it differs from what the file holds.  A volume
 * changed after a power cut or a crash is lost, which the user accepted.
 *
 * All of it goes through libkeiland-backend (kl_backend_audio_*, ws131-p004):
 * zdesktop never speaks audiod's protocol, and nothing here waits for it.
 * The preferences stay libkeiland's until the compositor keeps them itself
 * (ws131-p010).
 */

#include "glass.h"
#include "titlebar.h"
#include <keiland.h>

#include "userland/desktop/libkeiland-backend/keiland-backend.h"

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

/* How often a drag sends the volume, and how often it plays the feedback sound, in milliseconds. */
#define VOLUME_SEND_MS		50U
#define VOLUME_FEEDBACK_MS	250U

/* The preferences' keys. */
#define VOLUME_KEY_VOLUME	"sound.volume"
#define VOLUME_KEY_MUTED	"sound.muted"

/*
 * The volume's side of the system bar: audiod's link in libkeiland (made
 * on the desktop's first tick), what it last reported, the volume shown
 * (while a drag or a wheel leads, ahead of audiod's report), whether the
 * popup is open and where, where the icon was drawn, whether the slider
 * is dragged, the times of the last send and feedback, and what is
 * waiting to be sent or played.
 *
 * restored is set once the preferences' volume has been applied (on the
 * first connection to audiod); an audiod reached again later gets the
 * session's volume instead.  kept, kept_value and kept_muted are what the
 * file holds as far as zdesktop knows (read at the start, written at the
 * end), so that the end writes only a volume that changed.
 */
struct volume_view {
	struct kl_backend_audio *audio;
	unsigned opened;
	struct kl_backend_audio_state state;
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
	unsigned applied;
	unsigned restored;
	unsigned kept;
	unsigned kept_value;
	unsigned kept_muted;
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
static void volume_restore(struct zwl_server *server);
static int volume_sound(void);
static int volume_in_icon(int32_t x, int32_t y);
static int volume_in_popup(int32_t x, int32_t y);
static unsigned volume_slider_value(int32_t x);
static int32_t volume_slider_top(void);
static int32_t volume_mute_top(void);
static void volume_draw_switch(struct zwl_server *server, VkCommandBuffer command, int32_t right, int32_t middle, unsigned on, float fade);

/*
 * Reads what audiod has reported since the last tick, sends a volume a drag
 * held back, and plays a feedback sound held back.  The link is made on
 * the desktop's first tick.
 */
void
zwl_volume_tick(
	struct zwl_server *server)
{
	unsigned changed;
	uint64_t now;

	/* The link, once (the backend connects to audiod when it can). */
	if (!volume_view.opened) {
		volume_view.opened = 1U;
		volume_view.audio = kl_backend_audio_open();
		volume_view.icon_x = -1;
		volume_view.value = 100U;
	}

	/* No link could be made (no memory): the icon says there is no sound. */
	if (volume_view.audio == NULL)
		return;

	/* What arrived. */
	(void)kl_backend_audio_update(volume_view.audio, &changed);
	if (changed != 0U) {
		kl_backend_audio_get_state(volume_view.audio, &volume_view.state);
		server->dirty = 1;

		/* audiod reached or lost: the volume is given once per connection. */
		if ((changed & KL_BACKEND_AUDIO_CHANGED_REACHABLE) != 0U) {
			printf("ZWL VOLUME reachable=%u device=%u\n", volume_view.state.reachable, volume_view.state.device);
			if (!volume_view.state.reachable)
				volume_view.applied = 0U;
		}

		/* A report shows the volume, unless a drag or a wheel leads. */
		if ((changed & KL_BACKEND_AUDIO_CHANGED_VOLUME) != 0U && !volume_view.dragging && !volume_view.send_waiting) {
			volume_view.value = volume_view.state.left;
			volume_view.muted = volume_view.state.muted;
		}

		/* A connection with the volume known takes the preferences' volume, or the session's when reached again. */
		if (volume_view.state.reachable && !volume_view.applied && (changed & KL_BACKEND_AUDIO_CHANGED_VOLUME) != 0U) {
			volume_view.applied = 1U;
			volume_restore(server);
		}
	}

	/* A volume or a sound held back. */
	now = zwl_milliseconds();
	if (volume_view.send_waiting && now - volume_view.sent_ms >= VOLUME_SEND_MS)
		volume_send(server);
	if (volume_view.feedback_waiting && now - volume_view.feedback_ms >= VOLUME_FEEDBACK_MS) {
		volume_view.feedback_waiting = 0U;
		volume_view.feedback_ms = now;
		(void)kl_backend_audio_feedback(volume_view.audio);
		printf("ZWL VOLUME feedback at_ms=%llu via=held\n", (unsigned long long)now);
	}
}

/*
 * Writes the session's volume to the preferences, once, when the session
 * ends (Log Out, or zdesktop told to stop; BUG-161).  Nothing is written
 * when the file already holds it, or without preferences (the login
 * screen) or before the volume was known.
 */
void
zwl_volume_keep(
	struct zwl_server *server,
	const char *why)
{
	unsigned value;
	unsigned muted;
	char text[16];
	int error;

	/* Without preferences, or before audiod ever reported, there is nothing to keep. */
	if (server->preferences == NULL || !volume_view.restored)
		return;

	/* audiod's volume when it is reached, else the last one shown. */
	value = volume_view.value;
	muted = volume_view.muted;
	if (volume_view.state.reachable) {
		value = volume_view.state.left;
		muted = volume_view.state.muted;
	}

	/* What the file holds already is not written again. */
	if (volume_view.kept && value == volume_view.kept_value && muted == volume_view.kept_muted) {
		printf("ZWL VOLUME kept value=%u muted=%u why=%s write=0\n", value, muted, why);
		return;
	}

	/* The two keys, once. */
	(void)snprintf(text, sizeof(text), "%u", value);
	error = keiland_preferences_set(server->preferences, VOLUME_KEY_VOLUME, text);
	if (error == 0) {
		(void)snprintf(text, sizeof(text), "%u", muted);
		error = keiland_preferences_set(server->preferences, VOLUME_KEY_MUTED, text);
	}
	if (error == 0) {
		volume_view.kept = 1U;
		volume_view.kept_value = value;
		volume_view.kept_muted = muted;
	}

	/* The log line the tests read. */
	printf("ZWL VOLUME kept value=%u muted=%u why=%s write=1 error=%d\n", value, muted, why, error);
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

	/* Sent now when final, or when a drag's wait is over (audiod holds it; nothing is written). */
	now = zwl_milliseconds();
	if (final || now - volume_view.sent_ms >= VOLUME_SEND_MS)
		volume_send(server);

	/* The sound: now when final (not for muting), during a drag at most every VOLUME_FEEDBACK_MS. */
	if (!muted) {
		if (final || now - volume_view.feedback_ms >= VOLUME_FEEDBACK_MS) {
			volume_view.feedback_waiting = 0U;
			volume_view.feedback_ms = now;
			(void)kl_backend_audio_feedback(volume_view.audio);
			printf("ZWL VOLUME feedback at_ms=%llu via=%s\n", (unsigned long long)now, via);
		} else {
			volume_view.feedback_waiting = 1U;
		}
	}
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
	error = kl_backend_audio_set_volume(volume_view.audio, volume_view.value, volume_view.value, volume_view.muted);
	if (error != 0)
		printf("ZWL VOLUME send errno=%d\n", error);
}

/*
 * Gives a newly reached audiod its volume: the preferences' on the first
 * connection of the session (the volume kept at the end of the last one),
 * the session's on a later one (an audiod that came back).
 */
static void
volume_restore(
	struct zwl_server *server)
{
	int32_t value;
	int32_t muted;
	char text[16];
	int error;

	/* audiod came back: it gets the volume the session had. */
	if (volume_view.restored) {
		if (volume_view.value != volume_view.state.left || volume_view.muted != volume_view.state.muted) {
			volume_send(server);
			printf("ZWL VOLUME restored value=%u muted=%u from=session\n", volume_view.value, volume_view.muted);
		}
		return;
	}
	volume_view.restored = 1U;

	/* Without preferences (the login screen), or without a kept volume, audiod's stays. */
	if (server->preferences == NULL)
		return;
	error = keiland_preferences_get(server->preferences, VOLUME_KEY_VOLUME, text, sizeof(text));
	if (error != 0)
		return;
	value = keiland_preferences_get_int(server->preferences, VOLUME_KEY_VOLUME, 100, 0, 100);
	muted = keiland_preferences_get_int(server->preferences, VOLUME_KEY_MUTED, 0, 0, 1);

	/* What the file holds, so that the end writes only a change. */
	volume_view.kept = 1U;
	volume_view.kept_value = (unsigned)value;
	volume_view.kept_muted = (unsigned)muted;

	/* The same as audiod's: nothing to send. */
	if ((unsigned)value == volume_view.state.left && (unsigned)muted == volume_view.state.muted)
		return;

	/* Shown and sent, without a sound. */
	volume_view.value = (unsigned)value;
	volume_view.muted = (unsigned)muted;
	volume_send(server);
	server->dirty = 1;
	printf("ZWL VOLUME preferences value=%d muted=%d\n", value, muted);
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
