/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The text input and input method protocols in zdesktop (ws095-p004,
 * plan/ws095/design.md sections 2 to 4).
 *
 * text-input.c keeps each application's zwp_text_input_v3 and follows the
 * keyboard focus; input-method.c starts the system's input method, serves
 * its zwp_input_method_v2, keyboard grab, virtual keyboard and status, and
 * decides where each key goes.
 */

#ifndef ZWL_IME_H
#define ZWL_IME_H

#include "zwl.h"

#include <vulkan/vulkan.h>

/* The evdev codes whose presses are remembered, so that a release goes where its press went. */
#define ZWL_IME_KEYS		768U

/* The longest text one message carries (the protocols' 4000 bytes), with its NUL. */
#define ZWL_IME_TEXT_MAX	4001U

/*
 * Where the press of a key went, so that its release goes there too.
 */
enum zwl_ime_route {
	ZWL_IME_ROUTE_NONE,
	ZWL_IME_ROUTE_GRAB,
	ZWL_IME_ROUTE_TAKEN
};

/*
 * One application's zwp_text_input_v3.
 *
 * The state a request sets waits in the pending fields until the client's
 * commit; the current fields are what the input method is told.  The
 * record lives as long as the object and is freed with it.
 */
struct zwl_text_input {
	struct zwl_text_input *next;
	struct zwl_object *object;
	struct zwl_object *surface;
	unsigned pending_enable;
	unsigned pending_disable;
	char *pending_text;
	unsigned pending_text_set;
	int32_t pending_cursor;
	int32_t pending_anchor;
	uint32_t pending_cause;
	uint32_t pending_hint;
	uint32_t pending_purpose;
	int32_t pending_rectangle[4];
	unsigned enabled;
	char *text;
	int32_t cursor;
	int32_t anchor;
	uint32_t cause;
	uint32_t hint;
	uint32_t purpose;
	int32_t rectangle[4];
	uint32_t commits;
};

/*
 * The system's input method: its process and connection, its objects, the
 * text input it is activated for, and where each held key went.
 *
 * It is made by zwl_ime_start when the program exists, and lives for the
 * compositor's lifetime; the connection and the objects come and go with
 * the process, which is started again after a crash a few times.
 */
struct zwl_ime {
	pid_t pid;
	struct zwl_client *client;
	uint64_t starts[3];
	unsigned start_index;
	uint64_t restart_ms;
	unsigned given_up;
	struct zwl_object *method;
	struct zwl_object *grab;
	struct zwl_object *keyboard;
	unsigned keyboard_keymap;
	struct zwl_object *status;
	struct zwl_object *popups[4];
	char language[16];
	char label[16];
	int32_t indicator_x;
	unsigned indicator_shown;
	unsigned composing;
	struct zwl_text_input *active;
	unsigned activated;
	uint32_t done_count;
	char *pending_commit;
	char *pending_preedit;
	int32_t pending_begin;
	int32_t pending_end;
	uint32_t pending_before;
	uint32_t pending_after;
	char *preedit_shown;
	unsigned char route[ZWL_IME_KEYS];
	unsigned char keyboard_down[ZWL_IME_KEYS];
	unsigned char keyboard_taken[ZWL_IME_KEYS];
	unsigned watching;
	uint64_t watch_ms;
	unsigned bypass;
};

/* text-input.c */
int zwl_text_input_request(struct zwl_object *object, uint32_t opcode, const unsigned char *bytes, size_t size);
void zwl_text_input_object_gone(struct zwl_object *object);
void zwl_text_input_focus(struct zwl_server *server, struct zwl_object *previous);
struct zwl_text_input *zwl_text_input_current(struct zwl_server *server);
void zwl_text_input_deliver(struct zwl_text_input *input, const char *preedit, int32_t begin, int32_t end, const char *commit, uint32_t before, uint32_t after);

/* input-method.c */
void zwl_ime_start(struct zwl_server *server);
void zwl_ime_tick(struct zwl_server *server, uint64_t now);
int zwl_ime_request(struct zwl_object *object, uint32_t opcode, const unsigned char *bytes, size_t size);
void zwl_ime_object_gone(struct zwl_object *object);
void zwl_ime_client_gone(struct zwl_client *client);
int zwl_ime_global_visible(struct zwl_client *client, enum zwl_kind kind);
int zwl_ime_key_early(struct zwl_server *server, uint32_t time, uint32_t key, uint32_t state);
int zwl_ime_key_grab(struct zwl_server *server, uint32_t time, uint32_t key, uint32_t state, int composing_only);
void zwl_ime_modifiers(struct zwl_server *server);
void zwl_ime_focus(struct zwl_server *server, struct zwl_object *previous);
void zwl_ime_update(struct zwl_server *server, struct zwl_text_input *committed);
void zwl_ime_text_input_gone(struct zwl_server *server, struct zwl_text_input *input);
void zwl_ime_surface_commit(struct zwl_object *surface);
void zwl_ime_popup_draw(struct zwl_server *server, VkCommandBuffer command);
int32_t zwl_ime_indicator_width(struct zwl_server *server);
void zwl_ime_indicator_draw(struct zwl_server *server, VkCommandBuffer command, int32_t x, const float *ink);
int zwl_ime_indicator_button(struct zwl_server *server, uint32_t button, uint32_t state);

#endif
