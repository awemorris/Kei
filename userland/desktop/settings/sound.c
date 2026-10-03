/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The sound's volume as the Sound page shows and sets it (ws100-p005): the
 * same volume the system bar's popup sets (userland/desktop/wayland/
 * volume.c), through the same ways.
 *
 *   - audiod holds the volume during the session (libkeiland's
 *     keiland_audio_*); a change is sent to it, and a change made in the
 *     system bar comes back from it.  Nothing is written to a file while
 *     the user changes it (BUG-161, ws100-p012): zdesktop keeps the volume
 *     in the preferences (sound.volume, sound.muted in desktop.conf) once,
 *     at the session's end, and applies it to audiod at the next login.
 *   - The short feedback sound follows the system bar's rules: when a
 *     change is final (a drag let go, mute turned off), at most one every
 *     250 milliseconds while dragging, and never when mute is turned on.
 */

#include "settings.h"

#include <stdio.h>

/* How long a drag holds back its volumes and its sounds, in milliseconds (the system bar's). */
#define SOUND_SEND_MS		50U
#define SOUND_FEEDBACK_MS	250U

/* How often the page follows audiod while it shows, in milliseconds. */
#define SOUND_POLL_MS		250

/* The keys the volume is kept under (the system bar's). */
#define SOUND_KEY_VOLUME	"sound.volume"
#define SOUND_KEY_MUTED		"sound.muted"

static void sound_set(struct se_app *app, int value, int muted, int final);
static void sound_send(struct se_app *app);
static void sound_feedback(struct se_app *app);

/*
 * Starts following audiod's volume; until it reports, the page shows the
 * kept volume.
 */
void
se_sound_open(
	struct se_app *app)
{
	struct se_sound *sound;

	/* The kept volume, or all of it. */
	sound = &app->sound;
	sound->value = 100;
	sound->muted = 0;
	if (app->look.preferences != NULL) {
		sound->value = keiland_preferences_get_int(app->look.preferences, SOUND_KEY_VOLUME, 100, 0, 100);
		sound->muted = keiland_preferences_get_int(app->look.preferences, SOUND_KEY_MUTED, 0, 0, 1);
	}

	/* The link to audiod (it connects when audiod runs). */
	sound->audio = keiland_audio_open();
	se_log("SOUND open value=%d muted=%d link=%d", sound->value, sound->muted, sound->audio != NULL);
}

/*
 * Reads what audiod has reported, and sends a volume or a sound a drag
 * held back.
 */
void
se_sound_poll(
	struct se_app *app,
	uint64_t now)
{
	struct se_sound *sound;
	unsigned changed;

	/* No link, nothing to follow. */
	sound = &app->sound;
	if (sound->audio == NULL)
		return;

	/* What arrived. */
	changed = 0;
	(void)keiland_audio_update(sound->audio, &changed);
	if (changed != 0U) {
		keiland_audio_get_state(sound->audio, &sound->state);
		app->dirty = 1;
		se_log("SOUND report reachable=%u device=%u value=%u muted=%u", sound->state.reachable, sound->state.device, sound->state.left, sound->state.muted);

		/* A report shows the volume (set here or in the system bar), unless a drag or a held volume leads. */
		if ((changed & KEILAND_AUDIO_CHANGED_VOLUME) != 0U && !sound->dragging && !sound->send_waiting) {
			sound->value = (int)sound->state.left;
			sound->muted = (int)sound->state.muted;
		}
	}

	/* A volume held back. */
	if (sound->send_waiting && now - sound->sent_at >= SOUND_SEND_MS)
		sound_send(app);

	/* A sound held back. */
	if (sound->feedback_waiting && now - sound->feedback_at >= SOUND_FEEDBACK_MS)
		sound_feedback(app);
}

/*
 * Reports how long the main loop may sleep for the sound (-1: as long as it
 * likes): soon while something is held back, a little while the page shows.
 */
int
se_sound_wait(
	const struct se_app *app)
{
	/* Something held back. */
	if (app->sound.send_waiting || app->sound.feedback_waiting)
		return (int)SOUND_SEND_MS;

	/* The page shows: it follows the system bar's changes. */
	if (app->page == SE_PAGE_SOUND && app->sound.audio != NULL)
		return SOUND_POLL_MS;

	/* Nothing due. */
	return -1;
}

/*
 * Stops following audiod (a volume held back is sent first).
 */
void
se_sound_close(
	struct se_app *app)
{
	/* The last volume, then the link. */
	if (app->sound.audio == NULL)
		return;
	if (app->sound.send_waiting)
		sound_send(app);
	keiland_audio_close(app->sound.audio);
	app->sound.audio = NULL;
}

/*
 * Tells whether the volume can be set: audiod runs, with a device.
 */
int
se_sound_available(
	const struct se_app *app)
{
	/* audiod, and its device. */
	if (app->sound.audio == NULL || !app->sound.state.reachable)
		return 0;
	if (!app->sound.state.device)
		return 0;

	/* Succeeded: there is sound. */
	return 1;
}

/*
 * Carries out a click on the Sound page: the mute switch.
 */
void
se_sound_press(
	struct se_app *app,
	int index)
{
	int available;

	/* Only the switch, and only while there is sound. */
	available = se_sound_available(app);
	if (index != SE_SOUND_MUTE || !available)
		return;

	/* Mute turns (the sound only when it is turned off). */
	sound_set(app, app->sound.value, !app->sound.muted, 1);
}

/*
 * Follows a drag on the volume's slider: the volume moves with the
 * pointer, is sent now and then while held, and is final when let go.
 */
void
se_sound_drag(
	struct se_app *app,
	int index,
	int x,
	unsigned phase)
{
	float fraction;
	int available;
	int value;

	/* Only the slider, and only while there is sound. */
	available = se_sound_available(app);
	if (index != SE_SOUND_VOLUME || !available)
		return;

	/* The volume under the pointer. */
	fraction = se_slider_fraction(&app->sound.slider, x);
	value = (int)(fraction * 100.0f + 0.5f);
	if (value < 0)
		value = 0;
	if (value > 100)
		value = 100;

	/* Held: it moves; let go: final. */
	app->sound.dragging = 1;
	if (phase == SE_DRAG_END) {
		app->sound.dragging = 0;
		sound_set(app, value, app->sound.muted, 1);
		return;
	}

	/* Still held: shown, and sent now and then. */
	sound_set(app, value, app->sound.muted, 0);
}

/* Sets the volume shown: sent (now when final, else at most every SOUND_SEND_MS), a sound (not for mute), and kept when final. */
static void
sound_set(
	struct se_app *app,
	int value,
	int muted,
	int final)
{
	struct se_sound *sound;

	/* Nothing changes, except at a drag's end. */
	sound = &app->sound;
	if (value == sound->value && muted == sound->muted && !final)
		return;

	/* Shown at once. */
	sound->value = value;
	sound->muted = muted;
	sound->send_waiting = 1;
	app->dirty = 1;
	se_log("SOUND set value=%d muted=%d final=%d", value, muted, final);

	/* Sent now when final, or when a drag's wait is over. */
	if (final || app->now - sound->sent_at >= SOUND_SEND_MS)
		sound_send(app);

	/* The sound: now when final, during a drag at most every SOUND_FEEDBACK_MS; never when muted. */
	if (!muted) {
		if (final || app->now - sound->feedback_at >= SOUND_FEEDBACK_MS)
			sound_feedback(app);
		else
			sound->feedback_waiting = 1;
	}
}

/* Sends the volume shown to audiod. */
static void
sound_send(
	struct se_app *app)
{
	struct se_sound *sound;
	int error;

	/* The request; one that cannot go is logged (the next report shows audiod's). */
	sound = &app->sound;
	sound->send_waiting = 0;
	sound->sent_at = app->now;
	error = keiland_audio_set_volume(sound->audio, (unsigned)sound->value, (unsigned)sound->value, (unsigned)sound->muted);
	if (error != 0)
		se_log("SOUND send errno=%d", error);
}

/* Asks audiod for the feedback sound at the volume now. */
static void
sound_feedback(
	struct se_app *app)
{
	int error;

	/* Once now. */
	app->sound.feedback_waiting = 0;
	app->sound.feedback_at = app->now;
	error = keiland_audio_feedback(app->sound.audio);
	se_log("SOUND feedback error=%d", error);
}
