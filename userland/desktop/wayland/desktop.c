/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The desktop surface (keiland_desktop_v1, ws094-p002, plan/ws094/design.md
 * §3): one client's surface that lies over the wallpaper and under every
 * window, on every virtual desktop, where Files shows the icons of
 * ~/Desktop.
 *
 * keiland_desktop_manager_v1.get_desktop_surface(id, surface, token) gives
 * a surface the role, once, to the client that shows the token the
 * compositor gave the program it started (KEILAND_DESKTOP_TOKEN); the new
 * keiland_desktop_surface_v1 hears configure(serial, x, y, width, height):
 * where on the output it is (the work area under the system bar), and the
 * client acknowledges it before it draws.  The surface has no window: it
 * is not in the windows' list, Wiseview, the system bar or the focus
 * cycle.  Its image is laid over the wallpaper by its alpha, moves with
 * the desktop layer while App Home opens, and is under the windows' glass.
 *
 * A press where no window is (and none of the compositor's own places)
 * goes to the desktop surface, which then has the keyboard until a window
 * is pressed, raised or mapped.  A finger does the same through the
 * pointer's press, and a drag and drop over no window is the desktop's.
 *
 * A login session starts the desktop program (/bin/files --desktop, ws094-p003)
 * with a new token unless /etc/keiland/desktop says "off", and so does any
 * compositor that --desktop-client names a program for; it is started
 * again two seconds after it ends, at most four times a minute.
 */

#include "desktop.h"
#include "extras.h"
#include "menu.h"
#include "popup.h"

#include "userland/desktop/paths.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* Marks a parameter a function's signature requires but it does not use. */
#define UNUSED_PARAMETER(name)	((void)(name))

/* The requests of keiland_desktop_manager_v1 and of keiland_desktop_surface_v1, and the surface's one event. */
#define DESKTOP_MANAGER_DESTROY		0U
#define DESKTOP_MANAGER_GET		1U
#define DESKTOP_SURFACE_DESTROY		0U
#define DESKTOP_SURFACE_ACK		1U
#define DESKTOP_SURFACE_CONFIGURE	0U

/* The manager's errors: the role taken or the surface not free for it; a token that is not the one given. */
#define DESKTOP_ERROR_ROLE		0U
#define DESKTOP_ERROR_TOKEN		1U

/* The program a login session starts, and the file that turns it off. */
#define DESKTOP_COMMAND			KEILAND_BINDIR "/files --desktop"
#define DESKTOP_SWITCH			KEILAND_SYSCONFDIR "/keiland/desktop"

/* How long after it ends the program is started again, and how many starts a minute are allowed. */
#define DESKTOP_RESTART_MS		2000U
#define DESKTOP_STARTS			4U
#define DESKTOP_STARTS_MS		60000U

/* The token's random bytes (written in hex, twice as many characters), and the longest token taken. */
#define DESKTOP_TOKEN_BYTES		16U
#define DESKTOP_TOKEN_MAX		64U

/* The longest command line the desktop program is started with. */
#define DESKTOP_LINE_MAX		512U

/*
 * The one desktop surface of the compositor and the program that shows it.
 *
 * surface and object are the surface with the role and its
 * keiland_desktop_surface_v1 (both NULL when no client has the role); they
 * are cleared when either goes (zwl_desktop_object_gone).  x, y, width and
 * height are the place last configured, serial the configure's.  focused
 * says the desktop has the keyboard, since focus_top was the top window
 * (another top window takes it back).  command is the program to start
 * (NULL: none), token the token it is given (fixed says --desktop-token
 * chose it), pid the program running (0: none), gone_ms when it ended,
 * starts and starts_ms how many starts the minute since starts_ms has had.
 * drawn says the program's first image was drawn since its start (logged
 * once with its time, ws094-p009).
 */
struct desktop_state {
	struct zwl_object *surface;
	struct zwl_object *object;
	uint32_t serial;
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	int focused;
	struct zwl_object *focus_top;
	const char *command;
	char token[DESKTOP_TOKEN_MAX + 1U];
	int fixed;
	int decided;
	pid_t pid;
	uint64_t gone_ms;
	unsigned starts;
	uint64_t starts_ms;
	int limited;
	int drawn;
};

/*
 * The desktop surface and its program, for the compositor's life (one
 * compositor has one output and one desktop).  It starts empty; the
 * command-line options and the first display pass fill it.
 */
static struct desktop_state desk;

static int desktop_get(struct zwl_object *manager, const unsigned char *bytes, size_t size);
static int desktop_string(const unsigned char *bytes, size_t size, size_t offset, const char **text);
static void desktop_place(struct zwl_server *server, int32_t *x, int32_t *y, int32_t *width, int32_t *height);
static int desktop_configure(struct zwl_server *server);
static void desktop_start(struct zwl_server *server);
static void desktop_watch(struct zwl_server *server);
static int desktop_switched_off(void);
static void desktop_new_token(void);
static uint32_t desktop_word(const unsigned char *bytes, size_t offset);

/*
 * Takes a command-line option of the desktop: --desktop-client=COMMAND
 * (the program to start, also outside a login session; "none" for no
 * program) or --desktop-token=TOKEN (a fixed token, for tests that start
 * their own client).  Returns 1 when the argument was one, 0 when it was
 * another option, EINVAL for a bad value.
 */
int
zwl_desktop_option(
	struct zwl_server *server,
	const char *argument)
{
	size_t length;
	int match;

	UNUSED_PARAMETER(server);

	/* The program to start. */
	match = strncmp(argument, "--desktop-client=", 17);
	if (match == 0) {
		/* "none" starts no program, even in a login session. */
		desk.command = argument + 17;
		match = strcmp(desk.command, "none");
		if (match == 0)
			desk.command = "";
		desk.decided = 1;
		return 1;
	}

	/* Another option. */
	match = strncmp(argument, "--desktop-token=", 16);
	if (match != 0)
		return 0;

	/* A token that fits, not empty. */
	length = strlen(argument + 16);
	if (length == 0U || length > DESKTOP_TOKEN_MAX)
		return EINVAL;

	/* The fixed token. */
	snprintf(desk.token, sizeof(desk.token), "%s", argument + 16);
	desk.fixed = 1;

	/* Succeeded: the option was the desktop's. */
	return 1;
}

/*
 * Carries out a request of keiland_desktop_manager_v1 or of
 * keiland_desktop_surface_v1.
 */
int
zwl_desktop_request(
	struct zwl_object *object,
	uint32_t opcode,
	const unsigned char *bytes,
	size_t size)
{
	int error;

	/* The manager: it goes, or it gives a surface the role. */
	if (object->kind == ZWL_DESKTOP_MANAGER) {
		/* destroy. */
		if (opcode == DESKTOP_MANAGER_DESTROY && size == 0U) {
			zwl_object_destroy(object);
			return 0;
		}

		/* Only get_desktop_surface is left. */
		if (opcode != DESKTOP_MANAGER_GET)
			return EPROTO;
		error = desktop_get(object, bytes, size);
		if (error != 0)
			return error;

		/* Succeeded: the surface is the desktop's. */
		return 0;
	}

	/* The desktop surface goes; the surface shows nothing more. */
	if (opcode == DESKTOP_SURFACE_DESTROY) {
		if (size != 0U)
			return EPROTO;
		zwl_object_destroy(object);
		return 0;
	}

	/* ack_configure: the configure the client drew for. */
	if (opcode != DESKTOP_SURFACE_ACK || size != 4U)
		return EPROTO;
	printf("ZWL DESKTOP ack serial=%u\n", desktop_word(bytes, 0U));

	/* Succeeded: the acknowledgement is taken. */
	return 0;
}

/*
 * Unties an object that is going from the desktop: the surface with the
 * role or its keiland_desktop_surface_v1 ends the role, and the keyboard
 * goes back to the windows.
 */
void
zwl_desktop_object_gone(
	struct zwl_object *object)
{
	struct zwl_server *server;

	/* Only the role's surface and its object end the role. */
	if (object != desk.surface && object != desk.object)
		return;

	/* The role ends; the windows have the keyboard again. */
	server = object->client->server;
	printf("ZWL DESKTOP gone client=%llu\n", (unsigned long long)object->client->number);
	zwl_desktop_unfocus(server);
	desk.surface = NULL;
	desk.object = NULL;
	desk.focus_top = NULL;
	server->dirty = 1;

	/* Succeeded: the role has ended. */
	return;
}

/*
 * Moves the desktop on with the display pass: starts (or starts again) its
 * program, tells the surface a new place when the output changed, and
 * answers its frame callbacks while it is not drawn.
 */
void
zwl_desktop_tick(
	struct zwl_server *server)
{
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	int error;

	/* The program: started, watched, started again. */
	desktop_watch(server);
	desktop_start(server);

	/* Without the surface nothing else is to do. */
	if (desk.surface == NULL || desk.surface->dead)
		return;

	/* A new place (the output's size, the look) is configured again. */
	desktop_place(server, &x, &y, &width, &height);
	if (x != desk.x ||
	    y != desk.y ||
	    width != desk.width ||
	    height != desk.height) {
		error = desktop_configure(server);
		if (error != 0)
			printf("ZWL DESKTOP configure-failed errno=%d\n", error);
	}

	/* A frame that does not draw the desktop (a fullscreen window, the lock screen) still answers its callbacks. */
	if (!server->windowed ||
	    server->locked ||
	    server->greeter)
		zwl_callbacks_done(&desk.surface->committed_callbacks);

	/* Succeeded: the desktop is in step with the pass. */
	return;
}

/*
 * Draws the desktop surface's image where it is, over the wallpaper and
 * under the windows, by its alpha; in the glass look it moves with the
 * desktop layer (App Home).
 */
void
zwl_desktop_draw(
	struct zwl_server *server,
	VkCommandBuffer command)
{
	const struct zwl_import *image;
	struct zwl_import alpha;
	struct glass_shape shape;
	struct zwl_object *surface;
	uint32_t width;
	uint32_t height;

	/* Only a surface with an image. */
	surface = zwl_desktop_surface(server);
	if (surface == NULL)
		return;
	image = zwl_compose_surface_image(surface);
	if (image == NULL)
		return;

	/* The first image since the program started, with its time (the tests read it). */
	if (!desk.drawn) {
		desk.drawn = 1;
		printf("ZWL DESKTOP drawn at_ms=%llu\n", (unsigned long long)zwl_milliseconds());
	}

	/* Its size (its viewport's, else its image's). */
	zwl_surface_size(surface, &width, &height);
	if (width == 0U || height == 0U) {
		width = image->width;
		height = image->height;
	}

	/* The plain look: a quad by its alpha. */
	if (!server->glass) {
		alpha = *image;
		alpha.draw = ZWL_DRAW_ALPHA;
		zwl_compose_surface_quad(server, command, surface, &alpha, surface->x, surface->y);
		return;
	}

	/* The glass look: the image, by its alpha unless it is opaque, with the layer. */
	glass_shape_init(&shape, (float)surface->x, (float)surface->y, (float)width, (float)height);
	zwl_viewport_source(surface, shape.uv);
	shape.mode = MODE_IMAGE;
	shape.set = image->set;
	if (image->draw == ZWL_DRAW_OPAQUE)
		shape.opaque = 1.0f;
	glass_shape_draw(server, command, &shape);

	/* Succeeded: the desktop is drawn. */
	return;
}

/*
 * Returns the desktop surface when it has an image to show, NULL
 * otherwise.
 */
struct zwl_object *
zwl_desktop_surface(
	struct zwl_server *server)
{
	struct zwl_object *surface;

	UNUSED_PARAMETER(server);

	/* No surface, or one that is going. */
	surface = desk.surface;
	if (surface == NULL)
		return NULL;
	if (surface->dead || surface->client->fatal)
		return NULL;

	/* A surface without an image yet shows nothing. */
	if (surface->current == NULL)
		return NULL;

	/* Succeeded: the surface to show. */
	return surface;
}

/*
 * Tells whether a surface is the desktop's (1) or not (0), whether or not
 * it has an image yet.
 */
int
zwl_desktop_is(
	const struct zwl_object *surface)
{
	/* Only the surface with the role. */
	if (surface == NULL || surface != desk.surface)
		return 0;

	/* It is the desktop's. */
	return 1;
}

/*
 * Chooses the surface that has the keyboard at a display pass: the desktop
 * while it was pressed last and the top window is the same as then,
 * otherwise the top window (which then takes the keyboard back).
 */
struct zwl_object *
zwl_desktop_front(
	struct zwl_server *server,
	struct zwl_object *top)
{
	struct zwl_object *surface;

	/* The desktop was not pressed last. */
	if (!desk.focused)
		return top;

	/* A desktop surface that went, or another top window, gives the keyboard back. */
	surface = zwl_desktop_surface(server);
	if (surface == NULL || top != desk.focus_top) {
		desk.focused = 0;
		printf("ZWL DESKTOP unfocus via=window\n");
		return top;
	}

	/* Succeeded: the desktop keeps the keyboard. */
	return surface;
}

/*
 * Takes a press where no window is (after the compositor's own places): the
 * desktop surface under the pointer gets the keyboard and hears the press.
 * Returns 1 when the desktop takes it, 0 when there is no desktop there.
 */
int
zwl_desktop_press(
	struct zwl_server *server)
{
	struct zwl_object *surface;

	/* A desktop surface under the pointer. */
	surface = zwl_desktop_at(server, server->pointer_x, server->pointer_y);
	if (surface == NULL)
		return 0;

	/* It has the keyboard until another window comes on top or is pressed. */
	if (!desk.focused) {
		desk.focused = 1;
		desk.focus_top = zwl_top_window(server);
		printf("ZWL DESKTOP focus client=%llu surface=%u\n", (unsigned long long)surface->client->number, surface->id);
	}

	/* The focus follows now, so that the press reaches the desktop. */
	server->front_surface = surface;
	zwl_seat_focus(server);
	server->dirty = 1;

	/* Succeeded: the desktop hears the press. */
	return 1;
}

/*
 * Gives the keyboard back to the top window when the desktop has it (a
 * window was pressed).
 */
void
zwl_desktop_unfocus(
	struct zwl_server *server)
{
	/* Only a desktop with the keyboard gives it back. */
	if (!desk.focused)
		return;

	/* The top window takes it at once, before the press that caused this is delivered. */
	desk.focused = 0;
	server->front_surface = zwl_top_window(server);
	zwl_seat_focus(server);
	printf("ZWL DESKTOP unfocus via=press\n");

	/* Succeeded: the top window has the keyboard. */
	return;
}

/*
 * Finds the desktop surface at a point of the output where no window is
 * (for a press, a finger, a drag and drop's target); NULL when a window,
 * its title bar or the system bar is there, or there is no desktop.
 */
struct zwl_object *
zwl_desktop_at(
	struct zwl_server *server,
	int32_t x,
	int32_t y)
{
	struct zwl_object *surface;
	struct zwl_object *window;
	uint32_t width;
	uint32_t height;

	/* A desktop with an image, in the windows' mode. */
	surface = zwl_desktop_surface(server);
	if (surface == NULL || !server->windowed)
		return NULL;

	/* The point must be on it. */
	zwl_surface_size(surface, &width, &height);
	if (x < surface->x || y < surface->y)
		return NULL;
	if (x >= surface->x + (int32_t)width || y >= surface->y + (int32_t)height)
		return NULL;

	/* No window there: in the glass look any window or title bar at the point, in the plain look any window at all. */
	if (server->glass) {
		window = zwl_glass_window_at(server, x, y);
	} else {
		window = zwl_top_window(server);
	}

	/* A window covers the desktop there. */
	if (window != NULL)
		return NULL;

	/* Succeeded: the desktop is at the point. */
	return surface;
}

/* Gives a surface the desktop's role (one surface, for the token given), and configures it. */
static int
desktop_get(
	struct zwl_object *manager,
	const unsigned char *bytes,
	size_t size)
{
	struct zwl_object *surface;
	struct zwl_object *created;
	struct zwl_server *server;
	const char *token;
	uint32_t id;
	uint32_t surface_id;
	int differs;
	int error;

	/* The new ID, the surface and the token. */
	server = manager->client->server;
	if (size < 12U)
		return EPROTO;
	id = desktop_word(bytes, 0U);
	surface_id = desktop_word(bytes, 4U);
	surface = zwl_find(manager->client, surface_id);
	if (surface == NULL || surface->kind != ZWL_SURFACE)
		return EPROTO;
	error = desktop_string(bytes, size, 8U, &token);
	if (error != 0)
		return error;

	/* The token must be the one given; without one given no client may have the role. */
	differs = 1;
	if (desk.token[0] != '\0')
		differs = strcmp(token, desk.token);
	if (differs != 0) {
		printf("ZWL DESKTOP refused client=%llu reason=token\n", (unsigned long long)manager->client->number);
		(void)zwl_error_code(manager->client, manager->id, DESKTOP_ERROR_TOKEN, "not the desktop's token");
		return EPROTO;
	}

	/* One desktop surface, on a surface with no other role. */
	if (desk.surface != NULL ||
	    surface->role != NULL ||
	    surface->sub_role != NULL ||
	    surface->cursor_role) {
		printf("ZWL DESKTOP refused client=%llu reason=role\n", (unsigned long long)manager->client->number);
		(void)zwl_error_code(manager->client, manager->id, DESKTOP_ERROR_ROLE, "the desktop has a surface, or the surface has a role");
		return EPROTO;
	}

	/* The desktop surface, tied to its surface. */
	created = zwl_create(manager->client, id, ZWL_DESKTOP_SURFACE, manager->version);
	if (created == NULL)
		return EPROTO;
	created->surface = surface;
	desk.surface = surface;
	desk.object = created;

	/* Where it is, told to the client. */
	error = desktop_configure(server);
	if (error != 0)
		return error;

	/* The log the tests read. */
	printf("ZWL DESKTOP role client=%llu surface=%u x=%d y=%d width=%d height=%d\n", (unsigned long long)surface->client->number, surface->id, desk.x, desk.y, desk.width, desk.height);

	/* Succeeded: the surface is the desktop's. */
	return 0;
}

/* Reads a protocol string at an offset (its length with the end, then the bytes padded to a word); EPROTO when it does not fit. */
static int
desktop_string(
	const unsigned char *bytes,
	size_t size,
	size_t offset,
	const char **text)
{
	uint32_t length;
	size_t padded;

	/* The length, which counts the terminating zero. */
	if (offset + 4U > size)
		return EPROTO;
	length = desktop_word(bytes, offset);
	if (length == 0U || length > DESKTOP_TOKEN_MAX + 1U)
		return EPROTO;

	/* The bytes, padded to a whole word, must be the rest of the request. */
	padded = ((size_t)length + 3U) & ~(size_t)3U;
	if (offset + 4U + padded != size)
		return EPROTO;

	/* The string must end where its length says. */
	if (bytes[offset + 4U + length - 1U] != '\0')
		return EPROTO;

	/* Succeeded: the string. */
	*text = (const char *)(bytes + offset + 4U);
	return 0;
}

/* Works out where the desktop is: the output under the system bar in the glass look, the whole output otherwise. */
static void
desktop_place(
	struct zwl_server *server,
	int32_t *x,
	int32_t *y,
	int32_t *width,
	int32_t *height)
{
	int32_t right;
	int32_t bottom;

	/* What the on-screen keyboard's panel takes at the right or the bottom (keyboard.c, ws102-p007). */
	zwl_keyboard_reserved(&right, &bottom);

	/* The left edge, the whole width less the keyboard's column. */
	*x = 0;
	*width = (int32_t)server->width - right;

	/* Under the system bar in the glass look. */
	*y = 0;
	if (server->glass)
		*y = ZWL_GLASS_BAR;

	/* The rest of the height, less the keyboard's row. */
	*height = (int32_t)server->height - *y - bottom;

	/* Succeeded: the place is given. */
	return;
}

/* Tells the desktop surface its place (configure) and moves it there. */
static int
desktop_configure(
	struct zwl_server *server)
{
	uint32_t words[5];
	int error;

	/* The place. */
	desktop_place(server, &desk.x, &desk.y, &desk.width, &desk.height);
	desk.surface->x = desk.x;
	desk.surface->y = desk.y;

	/* configure(serial, x, y, width, height). */
	desk.serial = zwl_next_serial(server);
	words[0] = desk.serial;
	words[1] = (uint32_t)desk.x;
	words[2] = (uint32_t)desk.y;
	words[3] = (uint32_t)desk.width;
	words[4] = (uint32_t)desk.height;
	error = zwl_emit(desk.object->client, desk.object->id, DESKTOP_SURFACE_CONFIGURE, words, sizeof(words));
	if (error != 0)
		return error;

	/* Succeeded: the client knows where it is. */
	server->dirty = 1;
	return 0;
}

/*
 * Starts the desktop program when there should be one and none runs: in a
 * login session unless /etc/keiland/desktop is "off", or when
 * --desktop-client named one; two seconds after the last one ended, and
 * not more than DESKTOP_STARTS times a minute.
 */
static void
desktop_start(
	struct zwl_server *server)
{
	char line[DESKTOP_LINE_MAX];
	uint64_t now;
	int off;

	/* Whether a program is wanted is decided once: the option, else a login session's switch. */
	if (!desk.decided) {
		desk.decided = 1;
		desk.command = NULL;
		if (server->session) {
			/* A session shows the desktop unless it is switched off. */
			off = desktop_switched_off();
			if (!off)
				desk.command = DESKTOP_COMMAND;
		}
	}

	/* No program wanted, or one running, or too many starts already. */
	if (desk.command == NULL || desk.command[0] == '\0')
		return;
	if (desk.pid > 0 || desk.limited)
		return;

	/* Not before two seconds after the last one ended. */
	now = zwl_milliseconds();
	if (desk.gone_ms != 0U && now - desk.gone_ms < DESKTOP_RESTART_MS)
		return;

	/* A new minute of starts, or one more in this minute; too many stop the starts. */
	if (desk.starts == 0U || now - desk.starts_ms >= DESKTOP_STARTS_MS) {
		desk.starts = 0U;
		desk.starts_ms = now;
	}

	/* Too many starts in the minute stop the starts for good. */
	if (desk.starts >= DESKTOP_STARTS) {
		desk.limited = 1;
		printf("ZWL DESKTOP start-limit starts=%u\n", desk.starts);
		return;
	}

	/* A new token for each start, unless a fixed one was given. */
	if (!desk.fixed)
		desktop_new_token();

	/* The program, its token in its environment only. */
	snprintf(line, sizeof(line), "KEILAND_DESKTOP_TOKEN=%s exec %s", desk.token, desk.command);
	desk.pid = zwl_spawn(server, line);
	if (desk.pid < 0) {
		printf("ZWL DESKTOP start-failed errno=%d\n", errno);
		desk.starts++;
		desk.pid = 0;
		desk.gone_ms = now;
		return;
	}

	/* Every start counts toward the minute's limit, a failed one too (above). */
	desk.starts++;

	/* The log the tests read; its first image is logged when drawn. */
	desk.drawn = 0;
	printf("ZWL DESKTOP start pid=%d command=%s at_ms=%llu\n", (int)desk.pid, desk.command, (unsigned long long)zwl_milliseconds());

	/* Succeeded: the program is running. */
	return;
}

/* Notices that the desktop program ended (its own wait, or App Home's collecting every child). */
static void
desktop_watch(
	struct zwl_server *server)
{
	pid_t ended;
	int status;

	UNUSED_PARAMETER(server);

	/* No program running. */
	if (desk.pid <= 0)
		return;

	/* Still running: nothing to do. */
	ended = waitpid(desk.pid, &status, WNOHANG);
	if (ended == 0)
		return;

	/* Ended, or collected already (ECHILD); another error leaves it running. */
	if (ended < 0 && errno != ECHILD)
		return;

	/* The program is gone; it is started again after a while. */
	printf("ZWL DESKTOP exited pid=%d\n", (int)desk.pid);
	desk.pid = 0;
	desk.gone_ms = zwl_milliseconds();

	/* Succeeded: the end is noted. */
	return;
}

/* Tells whether /etc/keiland/desktop turns the desktop off (its first word is "off"). */
static int
desktop_switched_off(
	void)
{
	char text[8];
	ssize_t count;
	int descriptor;
	int match;

	/* Without the file the desktop is on. */
	descriptor = open(DESKTOP_SWITCH, O_RDONLY);
	if (descriptor < 0)
		return 0;

	/* Its first bytes. */
	memset(text, 0, sizeof(text));
	count = read(descriptor, text, sizeof(text) - 1U);
	if (count < 3) {
		close(descriptor);
		return 0;
	}

	/* The file is not needed any more. */
	close(descriptor);

	/* "off", alone on its line or followed by a space, turns it off. */
	match = strncmp(text, "off", 3U);
	if (match != 0)
		return 0;
	if (text[3] != '\0' &&
	    text[3] != '\n' &&
	    text[3] != ' ')
		return 0;

	/* The desktop is switched off. */
	return 1;
}

/* Makes a new random token, written in hex. */
static void
desktop_new_token(
	void)
{
	static const char digits[] = "0123456789abcdef";
	unsigned char bytes[DESKTOP_TOKEN_BYTES];
	unsigned index;

	/* Random bytes from the kernel's generator. */
	arc4random_buf(bytes, sizeof(bytes));

	/* Each byte as two hex digits. */
	for (index = 0; index < DESKTOP_TOKEN_BYTES; index++) {
		desk.token[index * 2U] = digits[bytes[index] >> 4];
		desk.token[index * 2U + 1U] = digits[bytes[index] & 0x0fU];
	}

	/* The token ends after them. */
	desk.token[DESKTOP_TOKEN_BYTES * 2U] = '\0';

	/* Succeeded: a new token. */
	return;
}

/* Reads one possibly unaligned native-endian protocol word. */
static uint32_t
desktop_word(
	const unsigned char *bytes,
	size_t offset)
{
	uint32_t word;

	/* The word, copied out byte by byte. */
	memcpy(&word, bytes + offset, sizeof(word));

	/* Succeeded: the word in the host's order. */
	return word;
}
