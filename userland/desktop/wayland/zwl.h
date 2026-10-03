/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Shared state for the Wayland compositor.
 *
 * zdesktop draws in window mode (WS035 compositing design, D0): a background
 * and every window, bottom to top, fullscreen ones too, with Vulkan into a
 * VK_KHR_display swapchain (compose.c); a window's image is imported once
 * per wl_buffer (import.c).  The direct scanout of a fullscreen window's
 * image (fullscreen mode) was removed in ws099-p015.  The WS014/WS029 scope had
 * no input; WS031 p013 extends it with one seat ("seat0", wl_seat v5):
 *
 * - Every /dev/input/eventN reporting REL_X+REL_Y or ABS_X+ABS_Y is a
 *   pointer and every one reporting KEY_A and KEY_Z is a keyboard.  Nodes are
 *   scanned at start-up and again every ZWL_INPUT_SCAN_MS; a node that fails
 *   a read is closed.  Capabilities follow the open nodes.
 * - Focus is the surface currently on the display; its client's pointer and
 *   keyboard objects get enter when it is shown and leave when it is replaced,
 *   unmapped or destroyed.  Clients without seat objects get nothing.
 * - Pointer positions are integer surface pixels (0..width-1, 0..height-1) sent
 *   as wl_fixed.  An absolute device's range is mapped linearly onto the
 *   surface; relative motion is added and clamped; the start is the centre.
 * - Buttons are Linux BTN_* codes.  A wheel notch is axis value 15.0 (negative
 *   is up/left) with axis_source wheel and axis_discrete +-1 for v5 pointers.
 *   Each evdev report ends with wl_pointer.frame for v5 pointers.
 * - The keyboard sends keymap format no_keymap with a /dev/null descriptor of
 *   size 0, evdev key codes, depressed modifiers (shift 0x1, ctrl 0x4,
 *   alt 0x8, meta 0x40), and repeat_info rate 0 (no client repeat).
 *   Kernel autorepeat events are not forwarded.
 * - wl_pointer.set_cursor is accepted and ignored; nothing draws a cursor and
 *   a cursor surface cannot be committed.
 * - A touch screen (multitouch protocol B) is offered as wl_touch (touch.c,
 *   WS079 p013); the compositor's own gestures see its fingers first.
 */
#ifndef ZWL_H
#define ZWL_H

#include "zwl-gpu.h"
#include "zwl-evdev.h"
#include "zwl-input.h"
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

/* Bound each connection's wire, descriptor, object and queued-event storage. */
#define ZWL_WIRE_MAX		65532U
#define ZWL_RIGHTS_MAX		32U
#define ZWL_OBJECT_MAX		4096U
#define ZWL_OUTPUT_MAX		1048576U

/* The first ID of the range the compositor gives the objects it makes for a client. */
#define ZWL_SERVER_ID_FIRST	0xff000000U

/* How many cursor images zdesktop draws for the shapes clients ask for (cursor.c). */
#define ZWL_CURSOR_IMAGES	10U

/* Bound the evdev nodes the seat reads and the events one report may carry. */
#define ZWL_INPUT_MAX		16U
#define ZWL_INPUT_FRAME_MAX	64U
#define ZWL_INPUT_PATH_MAX	64U

/* Rescan period for evdev nodes that appear after start-up, in milliseconds. */
#define ZWL_INPUT_SCAN_MS	2000U

/* A window's place when the client chooses its size: cascaded from the centre by this step. */
#define ZWL_CASCADE_STEP	32

/*
 * The glass look (glass.c): the system bar's height, a title bar's height,
 * the gap between a title bar and its body, the margin at the output's edges,
 * and so the highest a body may be.  BTN_LEFT is the left mouse button.
 */
#define ZWL_GLASS_BAR		34
#define ZWL_GLASS_TITLE		44
#define ZWL_GLASS_GAP		8
#define ZWL_GLASS_MARGIN	12
#define ZWL_GLASS_TOP		(ZWL_GLASS_BAR + ZWL_GLASS_MARGIN + ZWL_GLASS_TITLE + ZWL_GLASS_GAP)

/* Where a docked (maximized) window's body starts: just under the system bar (shell.c, keyboard.c). */
#define ZWL_GLASS_DOCK_TOP	(ZWL_GLASS_BAR + 4)
#define ZWL_BUTTON_LEFT		0x110U
#define ZWL_TITLE_MAX		64U

struct zwl_server;
struct zwl_client;
struct zwl_object;
struct zwl_compose;
struct kl_backend;
struct zwl_import;
struct zwl_panels;
struct zwl_ime;

/* Each live protocol identity has one immutable interface and negotiated version. */
enum zwl_kind {
	ZWL_DISPLAY,
	ZWL_REGISTRY,
	ZWL_COMPOSITOR,
	ZWL_SURFACE,
	ZWL_REGION,
	ZWL_CALLBACK,
	ZWL_BUFFER,
	ZWL_OUTPUT,
	ZWL_WM,
	ZWL_XDG_SURFACE,
	ZWL_TOPLEVEL,
	ZWL_FACTORY,
	ZWL_GPU_OBJECT,
	ZWL_SEAT,
	ZWL_POINTER,
	ZWL_KEYBOARD,
	ZWL_SHM,
	ZWL_SHM_POOL,
	ZWL_MENU_MANAGER,
	ZWL_MENU,
	ZWL_TOPLEVEL_MENU,
	ZWL_POSITIONER,
	ZWL_POPUP,
	ZWL_SUBCOMPOSITOR,
	ZWL_SUBSURFACE,
	ZWL_TITLEBAR_MANAGER,
	ZWL_TITLEBAR,
	ZWL_DATA_MANAGER,
	ZWL_DATA_SOURCE,
	ZWL_DATA_DEVICE,
	ZWL_DATA_OFFER,
	ZWL_PRIMARY_MANAGER,
	ZWL_PRIMARY_SOURCE,
	ZWL_PRIMARY_DEVICE,
	ZWL_PRIMARY_OFFER,
	ZWL_DECORATION_MANAGER,
	ZWL_DECORATION,
	ZWL_CURSOR_SHAPE_MANAGER,
	ZWL_CURSOR_SHAPE_DEVICE,
	ZWL_VIEWPORTER,
	ZWL_VIEWPORT,
	ZWL_GLASS_MANAGER,
	ZWL_GLASS,
	ZWL_CONTEXT_MENU,
	ZWL_TABLET_MANAGER,
	ZWL_TABLET_SEAT,
	ZWL_TABLET,
	ZWL_TABLET_TOOL,
	ZWL_TOUCH,
	/* The text input and input method protocols (text-input.c, input-method.c, ws095-p004). */
	ZWL_TEXT_INPUT_MANAGER,
	ZWL_TEXT_INPUT,
	ZWL_INPUT_METHOD_MANAGER,
	ZWL_INPUT_METHOD,
	ZWL_INPUT_POPUP,
	ZWL_KEYBOARD_GRAB,
	ZWL_VIRTUAL_KEYBOARD_MANAGER,
	ZWL_VIRTUAL_KEYBOARD,
	ZWL_IME_STATUS_MANAGER,
	ZWL_IME_STATUS,
	/* The desktop surface (desktop.c, ws094-p002). */
	ZWL_DESKTOP_MANAGER,
	ZWL_DESKTOP_SURFACE,
	/* The keyboard inset (inset.c, ws102-p015). */
	ZWL_KEYBOARD_INSET_MANAGER,
	ZWL_KEYBOARD_INSET,
	/* The editing operations (edit.c, ws102-p017). */
	ZWL_EDIT_MANAGER,
	ZWL_EDIT,
	/* KDE's server decoration, which GTK declares its decoration with (decoration.c, ws114-p008). */
	ZWL_KDE_DECORATION_MANAGER,
	ZWL_KDE_DECORATION,
};

/*
 * The editing operations (edit.c, ws102-p017; keiland_edit_v1's actions,
 * in its order) and a window's state's bits (keiland_edit_v1.set_state).
 */
#define ZWL_EDIT_COPY			0U
#define ZWL_EDIT_CUT			1U
#define ZWL_EDIT_PASTE			2U
#define ZWL_EDIT_UNDO			3U
#define ZWL_EDIT_REDO			4U
#define ZWL_EDIT_SELECT_ALL		5U
#define ZWL_EDIT_SELECT_BEGIN		6U
#define ZWL_EDIT_SELECT_END		7U
#define ZWL_EDIT_ACTIONS		8U
#define ZWL_EDIT_HAS_SELECTION		1U
#define ZWL_EDIT_CAN_PASTE		2U
#define ZWL_EDIT_CAN_UNDO		4U
#define ZWL_EDIT_CAN_REDO		8U
#define ZWL_EDIT_SELECTING		16U
#define ZWL_EDIT_FLAGS_ALL		31U

/*
 * Where a contact the edge gestures hear comes from (ws079-p010): the
 * pointer's left button, the tip of a pen, or a finger.
 */
enum zwl_contact_source {
	ZWL_CONTACT_POINTER,
	ZWL_CONTACT_PEN,
	ZWL_CONTACT_TOUCH
};

/* The wl_shm formats (ARGB8888 has alpha; XRGB8888's top byte is unused). */
#define ZWL_SHM_ARGB8888	0U
#define ZWL_SHM_XRGB8888	1U

/*
 * A wl_shm_pool's memory: the client's fd, mapped read-only once at
 * creation and again at resize.  The pool object and each buffer made from
 * it hold a reference; the mapping goes with the last.
 */
struct zwl_pool {
	int fd;
	void *map;
	size_t size;
	unsigned references;
};

/* Where a wl_shm buffer's pixels are in its pool. */
struct zwl_shm_buffer {
	struct zwl_pool *pool;
	uint32_t offset;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t format;
};

/*
 * The output queue retains unsent bytes through short writes and EAGAIN.
 *
 * A packet may own one descriptor, which travels as SCM_RIGHTS with the
 * packet's first byte and is closed once that byte has been sent; -1 means
 * the event carries no descriptor.
 */
struct zwl_packet {
	struct zwl_packet *next;
	size_t size;
	size_t sent;
	int descriptor;
	unsigned char bytes[1];
};

/*
 * One open evdev node the seat reads.
 *
 * A slot is in use while live is set; the server's fixed table keeps a slot's
 * address stable while the event loop holds it in a poll snapshot.  Events
 * are gathered into frame[] until SYN_REPORT, then applied together.
 */
struct zwl_input_device {
	int fd;
	unsigned live;
	unsigned pointer;
	unsigned keyboard;
	unsigned absolute;
	/* A pen tablet (tablet.c, WS079 p003): its reports go to the tablet, not to apply_frame. */
	unsigned tablet;
	/* A touch screen (touch.c, WS079 p013): its reports go to the touch screen, not to apply_frame. */
	unsigned touch;
	unsigned discarding;
	int32_t abs_x_minimum;
	int32_t abs_x_maximum;
	int32_t abs_y_minimum;
	int32_t abs_y_maximum;
	int32_t abs_x;
	int32_t abs_y;
	unsigned frame_count;
	struct input_event frame[ZWL_INPUT_FRAME_MAX];
	/* The evdev time of the report being applied (its SYN_REPORT), in microseconds (WS081). */
	uint64_t frame_time_us;
	char path[ZWL_INPUT_PATH_MAX];
};

/* One acquire fence: a fence fd and the payload generation the image waits for. */
#define ZWL_FENCE_MAX 4U
struct zwl_fence {
	int fd;
	uint64_t generation;
};

/*
 * One client-owned protocol object; destroyed buffers remain until all pending,
 * current and scanout holds are gone. Surface state is double-buffered.
 */
struct zwl_object {
	struct zwl_object *next;
	struct zwl_client *client;
	uint32_t id;
	enum zwl_kind kind;
	uint32_t version;
	unsigned dead;
	unsigned holds;
	unsigned busy;
	struct zwl_object *surface;
	struct zwl_object *role;
	struct zwl_object *top;
	/*
	 * An xdg_surface's xdg_wm_base, the binding get_xdg_surface was asked
	 * of (ws035-p132, BUG-112): only its own live xdg_surfaces keep that
	 * binding from being destroyed.  Compared, never followed: a binding
	 * outlives every live xdg_surface made from it.
	 */
	struct zwl_object *wm_base;
	struct zwl_object *pending;
	struct zwl_object *queued;
	struct zwl_object *current;
	struct zwl_object *callbacks;
	struct zwl_object *committed_callbacks;
	struct zwl_object *callback_next;
	unsigned attached;
	unsigned ready;
	unsigned configured;
	unsigned acknowledged;
	uint32_t configure_serial;
	uint64_t commit_order;
	/* A buffer's Vulkan image for window mode. */
	struct zwl_import *import;
	/* The OS module retains buffer descriptors or protocol params until final object retirement. */
	void *gpu_private;
	/* A surface's window: place, stacking (map order, lowest at the bottom), virtual desktop and fullscreen state. */
	unsigned mapped;
	uint64_t map_order;
	unsigned desktop;
	unsigned minimized;
	int32_t x;
	int32_t y;
	unsigned fullscreen;
	uint32_t window_width;
	uint32_t window_height;
	int32_t window_x;
	int32_t window_y;
	/*
	 * A window that was placed as a window (so window_x and window_y hold a
	 * place to come back to from fullscreen), and one to be centred at its
	 * next image of a new size (it left a fullscreen it started in, ws035-p138).
	 */
	unsigned placed;
	unsigned place_pending;
	/* A surface whose current image has not been shown yet. */
	unsigned fresh;
	/*
	 * A window's parent (xdg_toplevel.set_parent: the parent's surface,
	 * NULL for none or once it has gone), and for a sheet (ws090-p014,
	 * sheet.c) when it began to show under the parent (0: not shown yet).
	 */
	struct zwl_object *parent_window;
	uint64_t sheet_ms;
	/* The glass look: the toplevel's title and application ID, and a maximized window's place and size to go back to. */
	char title[ZWL_TITLE_MAX];
	char app_id[64];
	/* A keiland_edit_v1's operations (bit 1 << ZWL_EDIT_*) and state (ZWL_EDIT_HAS_SELECTION ...; edit.c). */
	uint32_t edit_actions;
	uint32_t edit_flags;
	unsigned maximized;
	int32_t restore_x;
	int32_t restore_y;
	uint32_t restore_width;
	uint32_t restore_height;
	/* A wl_shm buffer's place in its pool (NULL for a GPU buffer), and a pool object's memory. */
	struct zwl_shm_buffer *shm;
	struct zwl_pool *pool;
	/* A surface's damage in buffer pixels, pending and committed (x0, y0, x1, y1), and whether any was given. */
	int32_t damage[4];
	unsigned damaged;
	int32_t committed_damage[4];
	unsigned committed_damaged;
	/* A surface's copy of its wl_shm image that window mode samples, and whether it must be copied again. */
	struct zwl_import *shm_image;
	unsigned shm_upload;
	/* A surface used as the pointer's cursor (wl_pointer.set_cursor). */
	unsigned cursor_role;
	/* A window told its frame is done whose next commit the next frame waits for a moment. */
	unsigned awaited;
	/*
	 * Acquire fences (keiland_gpu_buffer_v1 revision two): those for the next
	 * commit, and the committed ones the queued image waits for.
	 */
	struct zwl_fence acquire[ZWL_FENCE_MAX];
	unsigned acquire_count;
	struct zwl_fence fences[ZWL_FENCE_MAX];
	unsigned fence_count;
	/* When the queued fences were committed, and whether a pass found one still pending. */
	uint64_t fence_ms;
	unsigned fence_waited;
	/*
	 * The System Menu (menu.c): an xdg_menu_v1's model; a toplevel's
	 * xdg_toplevel_menu_v1 (whose own top names the toplevel back); and the
	 * xdg_menu_v1 an xdg_toplevel_menu_v1 shows.  Each link is cleared from
	 * both ends when either object goes.
	 */
	struct zwl_menu_model *menu_model;
	struct zwl_object *toplevel_menu;
	struct zwl_object *shown_menu;
	/*
	 * The Titlebar Presentation (titlebar.c, WS070 p008): a
	 * keiland_titlebar_v1's model, and a toplevel's keiland_titlebar_v1 (whose own
	 * top names the toplevel back).  Each link is cleared from both ends
	 * when either object goes.
	 */
	struct zwl_titlebar_model *titlebar_model;
	struct zwl_object *titlebar;
	/*
	 * xdg_popup (popup.c, ws035-p076): an xdg_positioner's rules; a popup's
	 * parent surface (NULL once the parent has gone), its place relative to
	 * the parent's window geometry and its size, the order it was made in
	 * (popups are drawn in that order), whether it asked for the seat's
	 * grab, and whether it was closed (popup_done: it is not drawn or hit
	 * any more, and waits for its client to destroy it).  A surface's window
	 * geometry (xdg_surface.set_window_geometry: x, y, width, height),
	 * pending and committed, and whether one was set.
	 */
	struct zwl_positioner *positioner;
	struct zwl_object *popup_parent;
	int32_t popup_x;
	int32_t popup_y;
	int32_t popup_width;
	int32_t popup_height;
	uint64_t popup_order;
	unsigned popup_grab;
	unsigned popup_closed;
	int32_t pending_geometry[4];
	unsigned pending_geometry_set;
	int32_t geometry[4];
	unsigned geometry_set;
	/*
	 * A toplevel's requests (toplevel.c, ws035-p076): the smallest and
	 * largest size the client can draw (0 for no limit); the edges a resize
	 * drags and the right and bottom edges on the output that stay while
	 * the left or top edge is dragged (the anchor; no edges when there is
	 * none); the serial of the configure sent when the resize ended, and
	 * when it ended (ms, ws035-p128: an image drawn before the client read
	 * that configure may still come after it acknowledged it), and the last
	 * serial the client acknowledged (xdg_surface.ack_configure).
	 */
	int32_t min_width;
	int32_t min_height;
	int32_t max_width;
	int32_t max_height;
	uint32_t resize_edges;
	int32_t resize_right;
	int32_t resize_bottom;
	uint32_t resize_final_serial;
	uint64_t resize_end_ms;
	uint32_t acked_serial;
	/*
	 * Sub-surfaces (subsurface.c, ws035-p077).  A surface with the role has
	 * its wl_subsurface (whose own surface field names the surface back)
	 * and its parent; a parent has its children, bottom to top, linked by
	 * sub_next, each below or above the parent.  The position applied, the
	 * one set for the parent's next commit and whether one was set, and the
	 * synchronized mode.  A synchronized sub-surface's commit is cached
	 * (the buffer and whether one was attached, the frame callbacks) until
	 * its parent's state is applied.  Each link is cleared from both ends
	 * when either object goes.
	 */
	struct zwl_object *sub_role;
	struct zwl_object *sub_parent;
	struct zwl_object *sub_children;
	struct zwl_object *sub_next;
	unsigned sub_above;
	int32_t sub_x;
	int32_t sub_y;
	int32_t sub_pending_x;
	int32_t sub_pending_y;
	unsigned sub_moved;
	unsigned sub_sync;
	unsigned sub_cached;
	struct zwl_object *sub_cached_buffer;
	unsigned sub_cached_attached;
	struct zwl_object *sub_cached_callbacks;
	/*
	 * The clipboard (data.c, ws035-p079): a wl_data_source's MIME types
	 * (allocated strings, freed with it); a wl_data_offer's source (NULL
	 * once the source has gone).
	 */
	char **mime_types;
	unsigned mime_count;
	struct zwl_object *data_source;
	/* An offer of zdesktop's own selection, an item of the clipboard's history (clipboard.c). */
	unsigned data_offered;
	/*
	 * Drag and drop (data.c, ws035-p084): a source's actions (set_actions),
	 * and for an offer made for a drag: the actions its target takes and
	 * the one it prefers, the action last told, whether it accepted a type,
	 * and whether it was dropped on (its finish is then awaited).
	 */
	uint32_t dnd_actions;
	uint32_t dnd_preferred;
	uint32_t dnd_action;
	unsigned dnd_offer;
	unsigned dnd_accepted;
	unsigned dnd_dropped;
	/*
	 * ws035-p080: a toplevel's zxdg_toplevel_decoration_v1 and the
	 * decoration's toplevel (each cleared from both ends when either
	 * goes); a wp_cursor_shape_device_v1's wl_pointer (NULL once it has
	 * gone).  A surface's wp_viewport (whose own surface field names it
	 * back), and the viewport's state, pending and applied by the commit:
	 * the source rectangle in 24.8 fixed point (x, y, width, height; a
	 * width of 0 for none) and the destination size (0 for none), and
	 * whether the pending state changed since the last commit.
	 */
	struct zwl_object *decoration;
	struct zwl_object *decoration_toplevel;

	/*
	 * The toplevel owns its decoration negotiation and outstanding configure
	 * snapshots until teardown. Preferred zero means no explicit xdg choice;
	 * configured is the offered mode and committed is the visible mode.
	 * An acknowledged snapshot waits for the next surface commit. Generation
	 * changes invalidate mode proposals from a destroyed/replaced decoration;
	 * reset withdraws SSD at the next commit even without a new configure.
	 */
	uint32_t decoration_preferred;
	uint32_t decoration_configured;
	uint32_t decoration_committed;
	uint32_t decoration_acked_mode;
	uint64_t decoration_generation;
	unsigned decoration_acked;
	unsigned decoration_reset;
	struct zwl_decoration_configure *decoration_configures;
	/*
	 * ws114-p008: a toplevel whose xdg-decoration object was destroyed keeps
	 * the client's decoration (withdrawn, until a new object is made).  A
	 * surface's org_kde_kwin_server_decoration, and on that object the
	 * surface it decorates and the mode the client asked for (KDE_MODE_*,
	 * decoration.c); each is cleared from both ends when either goes.
	 */
	unsigned decoration_withdrawn;
	struct zwl_object *kde_decoration;
	struct zwl_object *kde_surface;
	uint32_t kde_mode;
	struct zwl_object *shape_pointer;
	struct zwl_object *viewport;
	int32_t pending_source[4];
	int32_t source[4];
	int32_t pending_destination[2];
	int32_t destination[2];
	unsigned viewport_changed;
	/*
	 * ws035-p083 (panels.c): a surface's keiland_glass_v1 (whose own surface
	 * field names it back; each cleared from both ends when either goes),
	 * and the record of its glass panels, pending and applied by the
	 * commit, which the surface owns from its first glass to its end.
	 */
	struct zwl_object *glass;
	struct zwl_panels *panels;
	/*
	 * The tablet protocol (tablet.c, WS079 p003): the number of the
	 * zwp_tablet_seat_v2 a tablet or a tool object was announced on (a
	 * seat's own number; unique for the compositor's life), and the slot of
	 * the tablet device and of the tool the object stands for
	 * (ZWL_TABLET_SLOT_NONE once the device or the tool has gone).
	 */
	uint64_t tablet_seat_number;
	unsigned tablet_slot;
	unsigned tool_slot;
};

/* One stream has independent byte and fd FIFOs, plus its own protocol namespace. */
struct zwl_client {
	struct zwl_client *next;
	struct zwl_server *server;
	int fd;
	uint64_t number;
	unsigned fatal;
	uint64_t fatal_time;
	struct zwl_object *objects;
	unsigned object_count;
	unsigned char input[ZWL_WIRE_MAX];
	size_t input_size;
	int rights[ZWL_RIGHTS_MAX];
	unsigned right_count;
	struct zwl_packet *output_head;
	struct zwl_packet *output_tail;
	size_t output_bytes;
	/*
	 * The ping (toplevel.c): the serial of the ping waiting for its answer
	 * (0 for none) and when it was sent, and whether the client has left a
	 * ping unanswered too long (its title bars say it is not responding).
	 */
	uint32_t ping_serial;
	uint64_t ping_ms;
	unsigned unresponsive;
	/*
	 * The next ID from the server's range (0xff000000 and up) for an object
	 * the compositor makes for this client (a wl_data_offer, data.c).
	 */
	uint32_t server_id_next;
	/*
	 * Nonzero for the connection of the system's input method, which
	 * zdesktop made itself (input-method.c, ws095-p004): only it sees and
	 * binds the input method's globals.
	 */
	unsigned ime;
	/*
	 * Nonzero once the client bound KDE's server decoration manager
	 * (decoration.c, ws114-p008).  Under KDE's protocol a window is the
	 * compositor's to decorate only through a decoration object, so such a
	 * client's windows without one keep their own decoration: GTK4 binds
	 * the manager and makes an object only for a window it wants
	 * decorated.
	 */
	unsigned kde_bound;
};

/* Cycle counts of the event loop, reported every few seconds (ZWL PERF). */
struct zwl_perf {
	uint64_t window_start_ms;
	uint64_t window_start_cycles;
	uint64_t poll_cycles;
	uint64_t work_cycles;
	uint32_t passes;
	uint32_t timeouts;
	/* Window mode: frames completed, and their time from the start of drawing to the fence. */
	uint32_t compose_frames;
	uint64_t compose_cycles;
	uint64_t compose_draw_cycles;
	uint64_t compose_acquire_cycles;
	uint64_t compose_present_cycles;
	/* wl_shm: images copied, and the CPU time of the copies. */
	uint32_t shm_copies;
	uint64_t shm_copy_cycles;
};

uint64_t zwl_cycles(void);

/* The user's preferences (libkeiland), opened by preferences.c. */
struct keiland_preferences;

/*
 * The compositor: one per process, alive from start to exit.
 *
 * It alone owns its Vulkan device and output and the connections of its
 * clients.
 */
struct zwl_server {
	struct zwl_perf perf;
	int listener;
	char socket_path[108];
	/* An explicit socket overrides the OS module's runtime-directory default. */
	unsigned socket_given;
	dev_t socket_device;
	ino_t socket_inode;
	unsigned socket_owned;
	struct zwl_client *clients;
	struct zwl_object *front_surface;
	uint64_t frame;
	uint64_t commit_order;
	uint64_t client_serial;
	uint32_t serial;
	uint32_t width;
	uint32_t height;
	/* The display mode's refresh in millihertz (from Vulkan, compose.c), told to clients by wl_output. */
	uint32_t refresh;
	/* What the Vulkan device can take, against which a GPU buffer's description is checked (compose.c fills it). */
	struct zwl_gpu_limits gpu_limits;
	uint64_t timeout_ms;
	uint64_t max_frames;
	/* Nonzero with --log-frames: every presentation and buffer release is printed (for the tests that read them). */
	unsigned log_frames;
	/*
	 * The graphical login (ws035-p095): with --greeter zdesktop draws the
	 * login screen (greeter.c), opens no socket and asks sessiond on
	 * auth_fd; with --session it is a login session, which has no deadline
	 * and ends with App Home's Log Out.  size_given: --width or --height
	 * was given, so the display's preferred size is not used.  control_fd:
	 * the session's descriptor to sessiond (--control-fd, -1 for none);
	 * auth_fd and control_fd are handed to libkeiland-backend, which speaks
	 * on them (ws131-p006);
	 * handed_over: the display's hand-over (handoff.c, ws035-p101) is done;
	 * logout_ms: when Log Out asked sessiond for a greeter (0: it did not).
	 */
	unsigned greeter;
	unsigned session;
	int auth_fd;
	int control_fd;
	unsigned handed_over;
	uint64_t logout_ms;
	/*
	 * The lock screen (ws035-p102): whether it shows, how long without
	 * input locks the session (--lock-idle, 0: never), and when the last
	 * input came.
	 */
	unsigned locked;
	uint64_t lock_idle_ms;
	unsigned lock_idle_given;
	uint64_t lock_input_ms;
	unsigned size_given;
	unsigned failed;
	struct zwl_input_device inputs[ZWL_INPUT_MAX];
	uint64_t input_scan_time;
	uint64_t input_events;
	uint64_t seat_events;
	unsigned capabilities;
	struct zwl_object *focus;
	int32_t pointer_x;
	int32_t pointer_y;
	unsigned modifier_keys;
	uint32_t modifiers;
	/* The locked modifiers (Caps Lock 0x2, Num Lock 0x10), each toggled by a press of its key (ws035-p078). */
	uint32_t locked_modifiers;
	/* Window mode: the Vulkan output, whether a frame is due, and the fence fd of the frame in flight. */
	struct zwl_compose *compose;
	/* The operating system's side (libkeiland-backend, WS131): opened before the OS resources, closed after them; NULL before. */
	struct kl_backend *backend;
	/* OS device authority can pause composition; zedBSD always leaves this zero. */
	unsigned os_paused;
	unsigned windowed;
	unsigned dirty;
	/*
	 * The damage (damage.c, ws035-p055): whether only a part of the output
	 * changed since the last frame, and that part (left, top, right,
	 * bottom).  dirty, set by any other change, draws the whole output.
	 */
	unsigned damaged;
	int32_t damage[4];
	int frame_fd;
	uint64_t map_order;
	uint32_t windows;
	uint64_t mode_switch_ms;
	/*
	 * An opening or closing of App Home or Wiseview waiting for its first
	 * frame (ws099-p002, C5): what it is (NULL for none) and when it was
	 * asked for.  The next frame is drawn without the frame pacing's wait,
	 * and its submission is logged once (ZWL FIRST_FRAME).
	 */
	const char *transition;
	uint64_t transition_ms;
	/*
	 * Frame pacing: after a frame, the windows it told are waited for, until
	 * all have committed or half the last frame's time (at most 50 ms) has
	 * passed, so that a quick client does not start the next frame without
	 * a slower one.
	 */
	unsigned awaiting;
	uint64_t frame_done_ms;
	uint64_t frame_wait_ms;
	/*
	 * Nonzero while a move of the pointer waits for the frame that shows the
	 * cursor at its new place: that frame does not wait for the windows
	 * (WS099's C6, the pointer's move to its display; ws075-p026).
	 */
	unsigned pointer_moved;
	/* The on-screen keyboard's glass shows the scene under it blurred (--keyboard-blur, ws075-p029). */
	unsigned keyboard_blur;
	/* The glass look: on, its font, the window being moved and where it was taken, the clock's minute. */
	unsigned glass;
	const char *font_path;
	const char *fallback_font_path;
	const char *wallpaper_path;
	float window_opacity;
	/*
	 * The user's preferences (preferences.c, ws089-p007): the file, when a
	 * login's desktop reads it (NULL for the login screen, or no home), and
	 * the last time it was looked at.  window_opacity_started and
	 * wallpaper_started are what the command line gave, which a key removed
	 * returns to; wallpaper_path above is the picture shown, and
	 * wallpaper_chosen the preferences' (empty for the command line's).  The pointer's speed is a
	 * percentage of the relative pointer's movement, with the hundredths
	 * of a pixel carried over; natural turns the wheel round.  The
	 * keyboards' repeat is what wl_keyboard.repeat_info tells a keyboard
	 * bound from then on.
	 */
	struct keiland_preferences *preferences;
	uint64_t preferences_checked_ms;
	float window_opacity_started;
	const char *wallpaper_started;
	char wallpaper_chosen[256];
	int32_t pointer_speed;
	int32_t pointer_natural;
	int64_t pointer_remainder_x;
	int64_t pointer_remainder_y;
	int32_t repeat_rate;
	int32_t repeat_delay_ms;
	struct zwl_object *drag;
	int32_t drag_dx;
	int32_t drag_dy;
	int64_t clock_minute;
	/*
	 * The glass look's shell (shell.c): where a move started, the first press
	 * of a double click, a docked window whose title is being pulled down,
	 * and the dock animation (the window, when it started, which way, and the
	 * body's rectangles at its start and end: x, y, width, height).
	 */
	int32_t drag_start_x;
	int32_t drag_start_y;
	struct zwl_object *click_surface;
	uint64_t click_ms;
	/*
	 * The presses of the latest run of quick clicks on click_surface's
	 * floating title bar (1, 2 or 3), and a window whose double click is
	 * waiting to dock it: it docks at dock_due_ms unless a third press
	 * comes first and sends it to the back instead (ws079-p013).
	 */
	unsigned click_count;
	struct zwl_object *dock_waiting;
	uint64_t dock_due_ms;
	struct zwl_object *pull;
	int32_t pull_start_y;
	int32_t pull_distance;
	struct zwl_object *anim;
	uint64_t anim_start_ms;
	unsigned anim_docking;
	int32_t anim_from[4];
	int32_t anim_to[4];
	/*
	 * Whether the last frame left the system bar out to keep a fullscreen
	 * window whole (ws035-p119, shell.c); only the change is logged.
	 */
	unsigned bar_hidden;
	/*
	 * Wiseview (shell.c): how far it is open (0 closed, 1 open) when settled,
	 * a gesture from the bottom edge and where it started, the animation to
	 * a settled value (from, to, when it started), and the window that was
	 * on top when it opened.
	 */
	float wiseview;
	unsigned wiseview_gesture;
	int32_t wiseview_start_y;
	unsigned wiseview_moving;
	float wiseview_from;
	float wiseview_to;
	uint64_t wiseview_start_ms;
	struct zwl_object *wiseview_current;
	/* A press on a Wiseview tile that may become its drag to a desktop (ws035-p072): the window, where it started, whether it moved. */
	struct zwl_object *wiseview_press;
	int32_t wiseview_press_x;
	int32_t wiseview_press_y;
	unsigned wiseview_dragging;
	/*
	 * The desktop layer's place while App Home pushes it aside: every glass
	 * shape drawn with layer_on is moved to layer_x, layer_y and scaled by
	 * layer_scale (glass.c); shell.c turns it on around the desktop only.
	 */
	unsigned layer_on;
	float layer_x;
	float layer_y;
	float layer_scale;
	/*
	 * App Home (home.c): how far it is open (0 closed, 1 open) when settled;
	 * a press in the top-left corner that may become the gesture, where it
	 * started and whether it has moved far enough to be one, and how far it
	 * is open by it; the animation to a settled value (from, to, when it
	 * started); what has been typed, and the selected application.
	 */
	float home;
	unsigned home_press;
	unsigned home_dragging;
	int32_t home_start_x;
	int32_t home_start_y;
	float home_drag;
	unsigned home_moving;
	float home_from;
	float home_to;
	uint64_t home_start_ms;
	char home_query[48];
	unsigned home_query_length;
	int home_selected;
	/*
	 * A press at the bottom edge while Home shows, which may become the
	 * swipe up that closes Home (ws079-p010): whether it has moved far
	 * enough to be one, where it started, and how far Home was open then.
	 */
	unsigned home_bottom_press;
	unsigned home_bottom_dragging;
	int32_t home_bottom_start_y;
	float home_bottom_from;
	/*
	 * The virtual desktops (ws035-p065): the one shown; a press at the
	 * left or right edge that may become the swipe (where it started,
	 * whether it has moved enough) and the swipe's offset in pixels; the
	 * slide to a desktop (from and to as desktop positions, when it
	 * started).
	 */
	unsigned desktop;
	unsigned desktop_press;
	unsigned desktop_dragging;
	int32_t desktop_start_x;
	int32_t desktop_offset;
	unsigned desktop_moving;
	float desktop_from;
	float desktop_to;
	uint64_t desktop_start_ms;
	/*
	 * App Home's pages (ws035-p071): the page shown; a press on Home that
	 * may become a page drag (where it started, the application under it,
	 * whether it has moved enough) and the drag's offset in pixels; the
	 * snap to a page (from and to as page positions, when it started).
	 */
	unsigned home_page;
	unsigned home_page_press;
	unsigned home_page_dragging;
	int32_t home_page_start_x;
	int32_t home_page_start_y;
	int home_page_app;
	int32_t home_page_offset;
	unsigned home_page_moving;
	float home_page_from;
	float home_page_to;
	uint64_t home_page_start_ms;
	/*
	 * The last launch: the application (its icon grows as Home closes),
	 * and whether its first window is still to grow from the icon, since
	 * when, and the icon's rectangle.
	 */
	int home_launch_app;
	unsigned home_launching;
	uint64_t home_launch_ms;
	int32_t home_launch_rect[4];
	/* The cursor: a client's surface, zdesktop's arrow when there is none, or hidden. */
	struct zwl_object *cursor_surface;
	/*
	 * The client whose request (a cursor surface, none to hide it, or a
	 * shape) the cursor state is; NULL for zdesktop's own.  Its state is
	 * shown only while the pointer is over that client's window (BUG-118);
	 * cleared when the state goes back to the arrow or the client goes.
	 */
	struct zwl_client *cursor_client;
	/* What the log last said about showing that client's cursor (0 none said, 1 not shown, 2 shown), for the tests. */
	unsigned cursor_client_logged;
	int32_t cursor_hotspot_x;
	int32_t cursor_hotspot_y;
	unsigned cursor_hidden;
	struct zwl_import *arrow;
	/*
	 * Nonzero from the start until the pointer first moves (input.c): the
	 * cursor is not drawn before, so a touch screen shows no arrow resting
	 * in the middle of the greeter or the desktop (ws035-p116).
	 */
	unsigned pointer_unmoved;
	/*
	 * The surface whose client was told the pointer entered it (seat.c): it
	 * hears the pointer's events.  It is the focused window, or the
	 * sub-surface of it under the pointer (ws035-p077); while a popup's grab
	 * has the pointer, the surface of the grab's chain under it, or none.
	 * It is cleared before that surface is freed.
	 */
	struct zwl_object *pointer_surface;
	/*
	 * Popups (popup.c): the topmost popup holding the seat's grab (NULL for
	 * none); whether the grab has the pointer (from its first popup shown
	 * until the grab ends); a button whose press dismissed the popups (its
	 * release is eaten too); the order the next popup is made in.
	 */
	struct zwl_object *popup_grab;
	unsigned pointer_grabbed;
	uint32_t popup_eaten_button;
	uint64_t popup_order;
	/*
	 * The pointer's buttons held now (bit n for BTN_LEFT + n), and the serial
	 * of the last press sent to a client, which a move or a resize must name
	 * (toplevel.c).  The window being resized (NULL for none), where the
	 * pointer was and the window's size when the resize started.
	 */
	uint32_t buttons_down;
	uint32_t press_serial;
	/*
	 * Borrowed identity and actual button of the last press delivered to a
	 * client. Surface teardown and that button's release clear this origin.
	 * Accepted xdg move/resize alone copies the origin into the interactive
	 * state; server-owned SSD gestures never establish client ownership.
	 * The window and origin may differ for a subsurface. Both are cleared
	 * before either identity is freed, or when the operation ends.
	 */
	struct zwl_object *press_surface;
	uint32_t press_button;
	/*
	 * Where the pointer was on the output when that press was delivered.  A
	 * client's move or resize names the press and may arrive after the
	 * pointer has gone on; it is anchored here, so the motion made while the
	 * client was answering is not lost (BUG-125).
	 */
	int32_t press_x;
	int32_t press_y;
	struct zwl_object *interactive_window;
	struct zwl_object *interactive_surface;
	uint32_t interactive_button;
	/*
	 * The time of the pointer event being handled (evdev's, in the wrapping
	 * milliseconds Wayland carries), set by zwl_seat_motion and
	 * zwl_seat_button before anything hears the event; the corner's swipe
	 * measures its speed with it (corner.c).
	 */
	uint32_t input_time;
	/*
	 * Where the contact the shell's pointer path carries comes from: the
	 * pointer, except while touch.c passes a finger through the shell as
	 * the pointer's left button (it sets ZWL_CONTACT_TOUCH around each call
	 * and puts ZWL_CONTACT_POINTER back), so the edge gestures know the
	 * finger's contact from the mouse's (corner.c).
	 */
	enum zwl_contact_source shell_source;
	/*
	 * The clipboard (data.c): the wl_data_source set as the selection (NULL
	 * for an empty clipboard), and the number of the client last told it
	 * (the keyboard's client; 0 for none), so a focus change tells the new
	 * one.
	 */
	struct zwl_object *selection;
	uint64_t selection_client;
	/* Whether the selection is zdesktop's own, an item of the clipboard's history (clipboard.c, ws102-p018; selection is NULL then). */
	unsigned selection_offered;
	/*
	 * The primary selection (primary.c, ws035-p100): the source set as it
	 * (NULL for none), and the number of the client last told it (0 for
	 * none), kept like the clipboard's.
	 */
	struct zwl_object *primary;
	uint64_t primary_client;
	/*
	 * The bounds last sent to windows (xdg_toplevel.configure_bounds,
	 * protocol.c): when the space for bodies no longer matches, the windows
	 * hear the new one (0 before any was sent).
	 */
	int32_t bounds_width;
	int32_t bounds_height;
	/*
	 * Drag and drop (data.c, ws035-p084), while dnd_active: the drag's
	 * wl_data_source (NULL for a drag inside its own client), the surface
	 * it started from, its icon surface (NULL for zdesktop's badge), the
	 * surface under the pointer that heard enter and the data device it
	 * heard it on, the offer made for it, and the titlebar told the part
	 * of a breadcrumb the drag is over (NULL for none) with that part.
	 * Each is cleared when its object goes.
	 */
	unsigned dnd_active;
	struct zwl_object *dnd_source;
	struct zwl_object *dnd_origin;
	struct zwl_object *dnd_icon;
	struct zwl_object *dnd_target;
	struct zwl_object *dnd_target_device;
	struct zwl_object *dnd_offer;
	struct zwl_object *dnd_titlebar;
	uint32_t dnd_part_id;
	uint32_t dnd_part_detail;
	/*
	 * The serial of the enter the drag's target heard, and after a drop the
	 * dropped-on client's number and that serial: a context menu answering
	 * it (the "ask" action's choice, ws035-p088) is taken like one
	 * answering a press (menu.c).
	 */
	uint32_t dnd_enter_serial;
	uint64_t dnd_drop_client;
	uint32_t dnd_drop_serial;
	/*
	 * The cursor shape the pointer's client asked for (wp_cursor_shape_v1,
	 * cursor.c, ws035-p080; 0 for zdesktop's arrow), and the images of the
	 * shapes zdesktop draws, by the index cursor.c gives them (NULL until
	 * made).
	 */
	uint32_t cursor_shape;
	struct zwl_import *cursor_images[ZWL_CURSOR_IMAGES];
	/*
	 * The edges of the window frame under the pointer, or of the resize it
	 * started (the glass look's frames, shell.c; ZWL_EDGE_* bits of
	 * toplevel.h, 0 over no frame): while not 0 the cursor is that frame's
	 * resize arrow, over the client's own cursor (cursor.c).
	 */
	uint32_t frame_edges;
	struct zwl_object *resize;
	int32_t resize_pointer_x;
	int32_t resize_pointer_y;
	int32_t resize_width;
	int32_t resize_height;
	/*
	 * The system's input method and the text inputs it serves
	 * (input-method.c, text-input.c, ws095-p004); NULL until
	 * zwl_ime_start makes it.
	 */
	struct zwl_ime *ime;
};

uint64_t zwl_milliseconds(void);
uint64_t zwl_microseconds(void);
void zwl_request_stop(void);
int zwl_greeter_open(struct zwl_server *server);
int zwl_greeter_button(struct zwl_server *server, uint32_t button, uint32_t state);
int zwl_greeter_key(struct zwl_server *server, uint32_t key, uint32_t state);
void zwl_greeter_tick(struct zwl_server *server);
void zwl_handoff_wait(struct zwl_server *server);
int zwl_handoff_logout(struct zwl_server *server);
void zwl_handoff_tick(struct zwl_server *server);
void zwl_handoff_stop(void *data, unsigned reason);
void zwl_handoff_answer(void *data, unsigned request, int error);
void zwl_backend_session_paused(void *data);
void zwl_backend_session_resumed(void *data);
void zwl_backend_input_paused(void *data, const char *path);
void zwl_backend_input_resumed(void *data, const char *path, int descriptor);
void zwl_backend_input_gone(void *data, const char *path);
int zwl_lock(struct zwl_server *server, const char *reason);
void zwl_greeter_answer(struct zwl_server *server, unsigned request, int error);
int zwl_emit(struct zwl_client *client, uint32_t object, uint32_t opcode, const void *payload, size_t size);
int zwl_emit_fd(struct zwl_client *client, uint32_t object, uint32_t opcode, const void *payload, size_t size, int descriptor);
void zwl_packet_free(struct zwl_packet *packet);
int zwl_flush(struct zwl_client *client);
int zwl_read(struct zwl_client *client);
int zwl_dispatch(struct zwl_client *client, uint32_t id, uint32_t opcode, const unsigned char *payload, size_t size);
int zwl_error(struct zwl_client *client, uint32_t object, const char *reason);
int zwl_error_code(struct zwl_client *client, uint32_t object, uint32_t code, const char *reason);
int zwl_take_fd(struct zwl_client *client);
void zwl_delete_id(struct zwl_client *client, uint32_t id);
void zwl_client_destroy(struct zwl_client *client);
struct zwl_object *zwl_find(struct zwl_client *client, uint32_t id);
struct zwl_object *zwl_create(struct zwl_client *client, uint32_t id, enum zwl_kind kind, uint32_t version);
struct zwl_object *zwl_create_server(struct zwl_client *client, enum zwl_kind kind, uint32_t version);
void zwl_object_destroy(struct zwl_object *object);
void zwl_buffer_get(struct zwl_object *buffer);
void zwl_buffer_put(struct zwl_object *buffer);
void zwl_buffer_size(const struct zwl_object *buffer, uint32_t *width, uint32_t *height);
void zwl_callbacks_done(struct zwl_object **callbacks);
void zwl_schedule(struct zwl_server *server);
void zwl_transition_request(struct zwl_server *server, const char *what);
void zwl_frame_done(struct zwl_server *server);
int zwl_compose_open(struct zwl_server *server);
int zwl_compose_output_prepare(struct zwl_server *server);
int zwl_compose_output_open(struct zwl_server *server);
void zwl_compose_output_close(struct zwl_server *server);
int zwl_compose_draw(struct zwl_server *server);
int zwl_compose_complete(struct zwl_server *server);
void zwl_compose_quiesce(struct zwl_server *server);
void zwl_compose_close(struct zwl_server *server);
VkResult zwl_import_adopt(struct zwl_object *buffer, VkImage image, VkDeviceMemory memory, uint32_t width, uint32_t height, VkFormat format);
void zwl_import_destroy(struct zwl_object *buffer);
void zwl_import_set_alpha(struct zwl_object *buffer, uint32_t alpha);
int zwl_shm_upload(struct zwl_server *server);
void zwl_shm_image_destroy(struct zwl_server *server, struct zwl_object *surface);
void zwl_pool_put(struct zwl_pool *pool);
int zwl_shm_request(struct zwl_object *object, uint32_t opcode, const unsigned char *bytes, size_t size);
int zwl_shm_bind(struct zwl_object *shm);
void zwl_cursor_default(struct zwl_server *server);
int zwl_cursor_client_shown(struct zwl_server *server);
int zwl_arrow_create(struct zwl_server *server);
void zwl_arrow_destroy(struct zwl_server *server);
struct zwl_object *zwl_top_window(struct zwl_server *server);
int zwl_glass_still(struct zwl_server *server);
int zwl_glass_body_damage(struct zwl_server *server, struct zwl_object *surface, int32_t *rect);
int zwl_glass_pointer_calm(struct zwl_server *server, int32_t x, int32_t y);
void zwl_damage_pointer(struct zwl_server *server, int32_t old_x, int32_t old_y);
void zwl_damage_commit(struct zwl_server *server, struct zwl_object *surface, struct zwl_object *previous);
void zwl_window_bounds_refresh(struct zwl_server *server);
int zwl_window_send_configure(struct zwl_object *surface);
int zwl_window_enter_fullscreen(struct zwl_object *surface);
int zwl_window_leave_fullscreen(struct zwl_object *surface);
void zwl_window_centre(struct zwl_server *server, struct zwl_object *surface);
int zwl_fence_ready(struct zwl_server *server, struct zwl_object *surface);
int zwl_compose_waiting(struct zwl_server *server);
void zwl_compose_poll(struct zwl_server *server);
int zwl_glass_button(struct zwl_server *server, uint32_t button, uint32_t state);
int zwl_glass_motion(struct zwl_server *server);
void zwl_glass_place(struct zwl_server *server, struct zwl_object *surface, int32_t width, int32_t height, int32_t step);
void zwl_glass_space(struct zwl_server *server, int32_t *width, int32_t *height);
void zwl_glass_fit(struct zwl_server *server, int32_t width, int32_t height, int32_t *x, int32_t *y);
void zwl_glass_tick(struct zwl_server *server);
void zwl_glass_prefetch(struct zwl_server *server);
int zwl_glass_wallpaper(struct zwl_server *server, const char *path);
void zwl_preferences_open(struct zwl_server *server);
void zwl_preferences_tick(struct zwl_server *server, uint64_t now);
void zwl_preferences_close(struct zwl_server *server);
float zwl_home_progress(struct zwl_server *server);
void zwl_home_layer(struct zwl_server *server, float progress, float *x, float *y, float *scale);
int zwl_home_button(struct zwl_server *server, uint32_t button, uint32_t state);
int zwl_home_motion(struct zwl_server *server);
int zwl_home_key(struct zwl_server *server, uint32_t key, uint32_t state);
void zwl_home_tick(struct zwl_server *server);
int zwl_home_axis(struct zwl_server *server, int32_t vertical, int32_t horizontal);
int zwl_home_launched(struct zwl_server *server, int32_t *rect);
void zwl_home_dismiss(struct zwl_server *server, const char *via);
pid_t zwl_spawn(struct zwl_server *server, const char *command);

/* The top-right corner's swipe that brings Notes (corner.c; the drawing is in glass.h). */
int zwl_corner_contact_begin(struct zwl_server *server, enum zwl_contact_source source, int32_t x, int32_t y, uint32_t time);
int zwl_corner_contact_move(struct zwl_server *server, int32_t x, int32_t y, uint32_t time);
int zwl_corner_contact_end(struct zwl_server *server, int32_t x, int32_t y, uint32_t time);
int zwl_corner_button(struct zwl_server *server, uint32_t button, uint32_t state);
int zwl_corner_motion(struct zwl_server *server);
void zwl_corner_tick(struct zwl_server *server);
int zwl_corner_showing(void);

/*
 * The on-screen keyboard (keyboard.c, ws102; the drawing is in glass.h):
 * its bottom corners' swipe, its panel, and the side of the square in each
 * bottom corner where the swipe starts.
 */
#define ZWL_KEYBOARD_ZONE	28
int zwl_keyboard_button(struct zwl_server *server, uint32_t button, uint32_t state);
int zwl_keyboard_motion(struct zwl_server *server);
void zwl_keyboard_tick(struct zwl_server *server);
int zwl_keyboard_showing(void);
int zwl_keyboard_at(int32_t x, int32_t y);
void zwl_keyboard_close(struct zwl_server *server, const char *reason);
void zwl_keyboard_reserved(int32_t *right, int32_t *bottom);
void zwl_keyboard_reserved_now(int32_t *right, int32_t *bottom);
/* A window hung under its parent's title bar (sheet.c, ws090-p014). */
struct zwl_object *zwl_sheet_parent(const struct zwl_object *surface);
struct zwl_object *zwl_sheet_of(const struct zwl_object *parent);
void zwl_sheet_set_parent(struct zwl_object *surface, struct zwl_object *parent);
void zwl_sheet_surface_gone(struct zwl_object *surface);
int zwl_keyboard_touch_down(struct zwl_server *server, uint32_t id, int32_t x, int32_t y, uint32_t time);
int zwl_keyboard_touch_motion(struct zwl_server *server, uint32_t id, int32_t x, int32_t y, uint32_t time);
int zwl_keyboard_touch_up(struct zwl_server *server, uint32_t id, int32_t x, int32_t y, uint32_t time);
void zwl_keyboard_touch_cancel(struct zwl_server *server, uint32_t id);
void zwl_keyboard_inset_notify(struct zwl_server *server, const int32_t *panel);

/* The editing operations and the previous application, for the keyboard's tool face (edit.c, ws102-p017). */
int zwl_edit_action(struct zwl_server *server, unsigned action);
int zwl_edit_state(struct zwl_server *server, uint32_t *enabled);
int zwl_focus_previous(struct zwl_server *server);

/*
 * The clipboard's history (clipboard.c, ws102-p018): the last
 * ZWL_CLIPBOARD_HISTORY selections' text, newest first, in memory only, for
 * the keyboard's history tab; an item is pasted by making it the
 * selection and sending the paste operation.  The lock screen and Log Out
 * empty it.
 */
#define ZWL_CLIPBOARD_HISTORY		10U
#define ZWL_CLIPBOARD_TEXT_MAX		(64U * 1024U)
unsigned zwl_clipboard_history_count(struct zwl_server *server);
const char *zwl_clipboard_history_get(struct zwl_server *server, unsigned index, size_t *length);
int zwl_clipboard_history_paste(struct zwl_server *server, unsigned index);
void zwl_clipboard_history_clear(struct zwl_server *server, const char *reason);

/* The edge gestures over a fullscreen window, whether the input is theirs, and whether one shows something (shell.c). */
int zwl_glass_fullscreen_input(struct zwl_server *server);
int zwl_glass_edge_button(struct zwl_server *server, uint32_t button, uint32_t state);
int zwl_glass_edge_motion(struct zwl_server *server);
int zwl_glass_overlay(struct zwl_server *server);
struct zwl_object *zwl_glass_title_at(struct zwl_server *server, int32_t x, int32_t y);
void zwl_glass_lower(struct zwl_server *server, struct zwl_object *surface, const char *via);
void zwl_glass_mapped(struct zwl_server *server, struct zwl_object *surface);
int zwl_glass_key(struct zwl_server *server, uint32_t key, uint32_t state);

/* The network's icon in the system bar and its menu (network.c, ws035-p013; the drawing is in glass.h). */
void zwl_network_tick(struct zwl_server *server);
int zwl_network_button(struct zwl_server *server, uint32_t button, uint32_t state);
int zwl_network_key(struct zwl_server *server, uint32_t key, uint32_t state);
int zwl_network_motion(struct zwl_server *server);
int zwl_network_is_open(void);
void zwl_volume_tick(struct zwl_server *server);
void zwl_volume_preferences(struct zwl_server *server);
int zwl_volume_button(struct zwl_server *server, uint32_t button, uint32_t state);
int zwl_volume_key(struct zwl_server *server, uint32_t key, uint32_t state);
int zwl_volume_motion(struct zwl_server *server);
int zwl_volume_axis(struct zwl_server *server, int32_t vertical, int32_t horizontal);
int zwl_volume_is_open(void);

/* What a toplevel asks the glass look's shell to do (xdg_toplevel requests, ws035-p076). */
#define ZWL_TOPLEVEL_MOVE		1
#define ZWL_TOPLEVEL_MAXIMIZE		2
#define ZWL_TOPLEVEL_UNMAXIMIZE		3
#define ZWL_TOPLEVEL_MINIMIZE		4
void zwl_glass_toplevel_request(struct zwl_server *server, struct zwl_object *surface, int request);
void zwl_glass_toplevel_move_end(struct zwl_server *server, struct zwl_object *surface);
uint32_t zwl_next_serial(struct zwl_server *server);
int zwl_seat_bind(struct zwl_object *seat);
int zwl_seat_request(struct zwl_object *object, uint32_t opcode, const unsigned char *bytes, size_t size);
void zwl_seat_focus(struct zwl_server *server);
void zwl_seat_surface_gone(struct zwl_object *surface);
void zwl_seat_capabilities(struct zwl_server *server);
void zwl_seat_motion(struct zwl_server *server, uint32_t time);
int zwl_seat_motion_shell(struct zwl_server *server, uint32_t time);
void zwl_seat_motion_deliver(struct zwl_server *server, uint32_t time);
void zwl_seat_button(struct zwl_server *server, uint32_t time, uint32_t button, uint32_t state);
int zwl_seat_button_shell(struct zwl_server *server, uint32_t time, uint32_t button, uint32_t state);
void zwl_seat_button_deliver(struct zwl_server *server, uint32_t time, uint32_t button, uint32_t state);
void zwl_seat_axis(struct zwl_server *server, uint32_t time, int32_t vertical, int32_t horizontal);
void zwl_seat_frame(struct zwl_server *server);
void zwl_seat_key(struct zwl_server *server, uint32_t time, uint32_t key, uint32_t state);
void zwl_seat_key_deliver(struct zwl_server *server, uint32_t time, uint32_t key, uint32_t state);
void zwl_seat_modifiers(struct zwl_server *server);
int zwl_input_attach(struct zwl_server *server, int descriptor, const char *path, unsigned pointer, unsigned keyboard, const struct input_absinfo *x, const struct input_absinfo *y);
void zwl_input_read_devices(struct zwl_server *server, struct zwl_input_device **devices, size_t count);
void zwl_input_close(struct zwl_server *server, struct zwl_input_device *device);
void zwl_input_forget(struct zwl_server *server, struct zwl_input_device *device);
void zwl_input_cleanup(struct zwl_server *server);

#endif
