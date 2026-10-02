/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The system's input method, /usr/libexec/keiland-ime (ws095-p004,
 * plan/ws095/design.md section 5).
 *
 * zdesktop starts it on a socket pair named by WAYLAND_SOCKET and starts it
 * again if it dies.  It binds the seat and the three globals only it is
 * shown (the input method manager, the virtual keyboard manager and
 * zdesktop's status), makes its languages (direct input first, then
 * Japanese), and serves the keyboard until the connection ends.
 */

#include "program.h"

#include "userland/desktop/paths.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Where the Japanese dictionaries are installed (the package ime-dict-ja). */
#define MAIN_SYSTEM_DICTIONARY		KEILAND_DATADIR "/kei/ime/ja/SKK-JISYO.X"
#define MAIN_SUPPLEMENT_DICTIONARY	KEILAND_DATADIR "/kei/ime/ja/SKK-JISYO.kei"

static void main_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
static void main_global_remove(void *data, struct wl_registry *registry, uint32_t name);
static int main_engines(struct program *program);
static void main_user_path(char *path, size_t size);
static void main_warm(struct program *program, struct ime_engine *engine);
static int main_serve(struct program *program);

/*
 * The registry's events.
 */
static const struct wl_registry_listener main_registry_listener = {
	main_global,
	main_global_remove
};

int
main(
	void)
{
	struct program program;
	int status;
	unsigned i;

	memset(&program, 0, sizeof(program));
	setvbuf(stdout, NULL, _IOLBF, 0);

	/* The engines' output is large; it lives on the heap. */
	program.out = malloc(sizeof(*program.out));
	if (program.out == NULL) {
		printf("KEI-IME FAILED step=memory\n");
		return 1;
	}

	/* The connection zdesktop gave (WAYLAND_SOCKET). */
	program.display = wl_display_connect(NULL);
	if (program.display == NULL) {
		printf("KEI-IME FAILED step=connect errno=%d\n", errno);
		return 1;
	}

	/* The globals: the seat and the input method's own. */
	program.registry = wl_display_get_registry(program.display);
	if (program.registry == NULL) {
		printf("KEI-IME FAILED step=registry\n");
		return 1;
	}

	status = wl_registry_add_listener(program.registry, &main_registry_listener, &program);
	if (status != 0) {
		printf("KEI-IME FAILED step=registry-listener\n");
		return 1;
	}

	status = wl_display_roundtrip(program.display);
	if (status < 0) {
		printf("KEI-IME FAILED step=roundtrip errno=%d\n", errno);
		return 1;
	}

	/* Without every global there is nothing to serve. */
	if (program.seat == NULL || program.method_manager == NULL ||
	    program.keyboard_manager == NULL || program.status_manager == NULL) {
		printf("KEI-IME FAILED step=globals\n");
		return 1;
	}

	/* The languages. */
	status = main_engines(&program);
	if (status != 0) {
		printf("KEI-IME FAILED step=engines\n");
		return 1;
	}

	/* The input method's objects. */
	status = program_method_start(&program);
	if (status != 0) {
		printf("KEI-IME FAILED step=method\n");
		return 1;
	}

	status = wl_display_roundtrip(program.display);
	if (status < 0) {
		printf("KEI-IME FAILED step=roundtrip errno=%d\n", errno);
		return 1;
	}

	/* The candidate window; without it the program still converts, only the candidates are not shown. */
	status = program_popup_start(&program);
	printf("KEI-IME POPUP ready=%d error=%d\n", program.popup.ready, status);

	/* Serves the keyboard until the connection ends or another input method holds the seat. */
	printf("KEI-IME READY languages=%u\n", program.engine_count);
	while (!program.unavailable) {
		status = main_serve(&program);
		if (status != 0)
			break;
	}

	/* The engines go with the program. */
	for (i = 0; i < program.engine_count; i++)
		program.engines[i].ops->destroy(&program.engines[i]);

	printf("KEI-IME DONE\n");
	free(program.out);
	wl_display_disconnect(program.display);

	/* Succeeded: the program ends with its connection. */
	return 0;
}

/*
 * Binds the globals the program needs.
 */
static void
main_global(
	void *data,
	struct wl_registry *registry,
	uint32_t name,
	const char *interface,
	uint32_t version)
{
	struct program *program;
	int order;

	UNUSED_PARAMETER(version);

	program = data;

	/* The seat. */
	order = strcmp(interface, "wl_seat");
	if (order == 0) {
		program->seat = wl_registry_bind(registry, name, &wl_seat_interface, 1);
		return;
	}

	/* The input method manager. */
	order = strcmp(interface, "zwp_input_method_manager_v2");
	if (order == 0) {
		program->method_manager = wl_registry_bind(registry, name, &zwp_input_method_manager_v2_interface, 1);
		return;
	}

	/* The virtual keyboard manager. */
	order = strcmp(interface, "zwp_virtual_keyboard_manager_v1");
	if (order == 0) {
		program->keyboard_manager = wl_registry_bind(registry, name, &zwp_virtual_keyboard_manager_v1_interface, 1);
		return;
	}

	/* zdesktop's status. */
	order = strcmp(interface, "keiland_ime_status_manager_v1");
	if (order == 0) {
		program->status_manager = wl_registry_bind(registry, name, &keiland_ime_status_manager_v1_interface, 1);
		return;
	}

	/* The compositor, for the candidate window's surface. */
	order = strcmp(interface, "wl_compositor");
	if (order == 0) {
		program->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
		return;
	}

	/* Shared memory, for its buffers. */
	order = strcmp(interface, "wl_shm");
	if (order == 0)
		program->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
}

/*
 * Ignores a global that goes; zdesktop's globals do not.
 */
static void
main_global_remove(
	void *data,
	struct wl_registry *registry,
	uint32_t name)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(registry);
	UNUSED_PARAMETER(name);
}

/*
 * Makes the languages: direct input, then Japanese with its dictionaries.
 *
 * Returns 0, or -1 when an engine cannot be made.
 */
static int
main_engines(
	struct program *program)
{
	struct ja_config config;
	char user[1024];
	int error;

	/* Direct input comes first: the program starts in it. */
	error = ime_direct_create(&program->engines[0]);
	if (error != 0)
		return -1;

	program->engine_count = 1;

	/* Japanese, with the system dictionary, the supplement and the user's. */
	main_user_path(user, sizeof(user));
	config.system_dictionary = MAIN_SYSTEM_DICTIONARY;
	config.supplement_dictionary = MAIN_SUPPLEMENT_DICTIONARY;
	config.user_dictionary = user;
	error = ja_engine_create(&program->engines[1], &config);
	if (error != 0)
		return -1;

	program->engine_count = 2;

	/* A first conversion is made and dropped, so that the first key typed does not wait for the code to be read in. */
	main_warm(program, &program->engines[1]);

	/* Succeeded: the languages are ready. */
	return 0;
}

/*
 * Types a reading into an engine, converts it and drops it: the engine's
 * code and the dictionaries' pages are read in before the first real key.
 */
static void
main_warm(
	struct program *program,
	struct ime_engine *engine)
{
	static const char letters[] = "kyouhaiitenkidesu";
	struct ime_key key;
	size_t i;

	/* Each letter, then Space to convert. */
	key.modifiers = 0;
	for (i = 0; letters[i] != '\0'; i++) {
		key.code = 30U;
		key.character = (unsigned char)letters[i];
		engine->ops->key(engine, &key, program->out);
	}

	key.code = IME_KEY_SPACE;
	key.character = ' ';
	engine->ops->key(engine, &key, program->out);

	/* Nothing of it is kept. */
	engine->ops->reset(engine, false, program->out);
}

/*
 * Gives the user dictionary's path, making its directory
 * (~/.config/kei/ime, the user's alone).
 */
static void
main_user_path(
	char *path,
	size_t size)
{
	const char *home;
	char directory[1024];

	/* Without a home the choices are not kept. */
	home = getenv("HOME");
	if (home == NULL || home[0] == '\0') {
		snprintf(path, size, "/nonexistent/ja-user.dict");
		return;
	}

	/* Each level of ~/.config/kei/ime, made when missing. */
	snprintf(directory, sizeof(directory), "%s/.config", home);
	(void)mkdir(directory, 0700);
	snprintf(directory, sizeof(directory), "%s/.config/kei", home);
	(void)mkdir(directory, 0700);
	snprintf(directory, sizeof(directory), "%s/.config/kei/ime", home);
	(void)mkdir(directory, 0700);

	/* The file in it. */
	snprintf(path, size, "%s/ja-user.dict", directory);
}

/*
 * Serves one turn of the connection: the events queued, then a wait for
 * more no longer than the held key's next repeat, then the repeat when it
 * is due.
 *
 * Returns 0, or -1 when the connection ended.
 */
static int
main_serve(
	struct program *program)
{
	struct pollfd descriptor;
	int timeout;
	int status;

	/* The events already read. */
	status = wl_display_dispatch_pending(program->display);
	if (status < 0)
		return -1;

	/* What the program sends, sent now. */
	(void)wl_display_flush(program->display);

	/* Waits for zdesktop, or until the held key repeats. */
	timeout = program_repeat_timeout(program, program_clock_ms());
	descriptor.fd = wl_display_get_fd(program->display);
	descriptor.events = POLLIN;
	descriptor.revents = 0;
	status = poll(&descriptor, 1, timeout);
	if (status < 0 && errno != EINTR)
		return -1;

	/* zdesktop's events, read and handled. */
	if (status > 0) {
		status = wl_display_dispatch(program->display);
		if (status < 0)
			return -1;
	}

	/* The held key's repeat. */
	program_repeat_due(program, program_clock_ms());

	/* Succeeded: the connection goes on. */
	return 0;
}
