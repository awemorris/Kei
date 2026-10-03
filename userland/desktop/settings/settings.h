/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The model of Settings (WS089): its pages, the window's layout, the
 * clickable regions of a frame, the history of pages and what the titlebar
 * and the menus show.
 *
 * Nothing here knows about Wayland or Vulkan.  The window's parts
 * (window.h) hand inputs to the interface and show the frames it draws;
 * the host tests drive the same interface and draw its frames into
 * pictures.  The drawing surface is the file manager's canvas
 * (userland/desktop/files/canvas.c, text.c and icons.c), compiled into
 * this program unchanged (plan/ws089/design.md section 4).
 */

#ifndef KEILAND_SETTINGS_H
#define KEILAND_SETTINGS_H

#include "../files/canvas.h"

#include <keiland.h>

#include <stddef.h>
#include <stdint.h>

/* The window's size when the compositor leaves it to the program. */
#define SE_WIDTH		1180
#define SE_HEIGHT		800

/* How many clickable regions one frame records. */
#define SE_HITS			512

/* How many pages the history of pages keeps. */
#define SE_HISTORY		64

/* The bytes of a titlebar's text with its NUL, and of one part of the breadcrumb. */
#define SE_TITLEBAR_TEXT	1024
#define SE_TITLEBAR_PART	65

/* How many parts the breadcrumb has at most: Settings and the page. */
#define SE_CRUMBS		2

/* How many things done with the titlebar wait for the main loop at most. */
#define SE_TITLEBAR_EVENTS	16U

/* How many glass panels a frame has at most: the list of pages and the page. */
#define SE_PANELS		2

/*
 * The colours of the interface, the file manager's (ws071) so that the two
 * windows look alike: quiet slate text, the accent for what is chosen.
 */
#define SE_COLOR_BACKGROUND_TOP		FM_RGB(0xeef2f7)
#define SE_COLOR_BACKGROUND_BOTTOM	FM_RGB(0xe6ebf3)
#define SE_COLOR_PANEL			FM_RGBA(0xffffff, 150)
#define SE_COLOR_PANEL_EDGE		FM_RGBA(0xffffff, 170)
#define SE_COLOR_GLASS_SIDEBAR		FM_RGBA(0xffffff, 40)
#define SE_COLOR_GLASS_PAGE		FM_RGBA(0xffffff, 60)
#define SE_COLOR_CARD			FM_RGBA(0xffffff, 150)
#define SE_COLOR_CARD_EDGE		FM_RGBA(0xffffff, 190)
#define SE_COLOR_TILE			FM_RGBA(0xf4f7fb, 190)
#define SE_COLOR_TEXT			FM_RGB(0x1e2632)
#define SE_COLOR_TEXT_SECONDARY		FM_RGB(0x56606f)
#define SE_COLOR_TEXT_FAINT		FM_RGB(0xa3abb8)
#define SE_COLOR_ICON			FM_RGB(0x46526a)
#define SE_COLOR_ACCENT			FM_RGB(0x2f7cf6)
#define SE_COLOR_SELECTION		FM_RGBA(0x2f7cf6, 40)
#define SE_COLOR_SELECTION_INACTIVE	FM_RGBA(0x7a8699, 38)
#define SE_COLOR_HOVER			FM_RGBA(0x5a6b85, 18)
#define SE_COLOR_SEPARATOR		FM_RGBA(0x8a96aa, 60)
#define SE_COLOR_GOOD			FM_RGB(0x2fb45a)
#define SE_COLOR_BAD			FM_RGB(0xe0533d)

/* The modifier keys held with an input, the program's own bits. */
#define SE_MOD_SHIFT		0x01U
#define SE_MOD_CTRL		0x02U
#define SE_MOD_ALT		0x04U
#define SE_MOD_SUPER		0x08U

/* The pointer buttons, as evdev codes (BTN_LEFT, BTN_RIGHT). */
#define SE_BUTTON_LEFT		0x110U
#define SE_BUTTON_RIGHT		0x111U

/* The evdev codes of the keys the interface acts on. */
#define SE_KEY_ESC		1U
#define SE_KEY_BACKSPACE	14U
#define SE_KEY_TAB		15U
#define SE_KEY_ENTER		28U
#define SE_KEY_LEFTSHIFT	42U
#define SE_KEY_RIGHTSHIFT	54U
#define SE_KEY_SPACE		57U
#define SE_KEY_Q		16U
#define SE_KEY_W		17U
#define SE_KEY_F		33U
#define SE_KEY_HOME		102U
#define SE_KEY_UP		103U
#define SE_KEY_PAGE_UP		104U
#define SE_KEY_LEFT		105U
#define SE_KEY_RIGHT		106U
#define SE_KEY_END		107U
#define SE_KEY_DOWN		108U
#define SE_KEY_PAGE_DOWN	109U

/*
 * The kinds of input the window gives the interface.
 */
enum se_event_type {
	SE_EVENT_MOTION,
	SE_EVENT_BUTTON,
	SE_EVENT_AXIS,
	SE_EVENT_LEAVE,
	SE_EVENT_KEY,
	SE_EVENT_FOCUS,
	SE_EVENT_ACTION
};

/*
 * One input: where the pointer is, which button or key, the modifiers
 * held, and a menu's action.  serial is the compositor's serial of a
 * button press.  touch is 1 for the pointer's moves and presses a finger
 * on the touch screen made (a drag of the finger scrolls a pane, ws089-p012).
 */
struct se_event {
	unsigned type;
	int x;
	int y;
	uint32_t button;
	int pressed;
	int scroll;
	uint32_t key;
	uint32_t modifiers;
	uint32_t serial;
	uint64_t time;
	int focused;
	uint32_t action;
	int touch;
};

/*
 * The pages, in the order the list of pages shows them.  Home is not in
 * the list; the titlebar's Home control opens it.
 */
enum se_page_id {
	SE_PAGE_HOME,
	SE_PAGE_WIFI,
	SE_PAGE_ETHERNET,
	SE_PAGE_BLUETOOTH,
	SE_PAGE_VPN,
	SE_PAGE_NETWORK,
	SE_PAGE_APPEARANCE,
	SE_PAGE_WALLPAPER,
	SE_PAGE_NOTIFICATIONS,
	SE_PAGE_SOUND,
	SE_PAGE_DISPLAY,
	SE_PAGE_STORAGE,
	SE_PAGE_BATTERY,
	SE_PAGE_KEYBOARD,
	SE_PAGE_MOUSE,
	SE_PAGE_TOUCHPAD,
	SE_PAGE_PRINTERS,
	SE_PAGE_SHARING,
	SE_PAGE_USERS,
	SE_PAGE_PRIVACY,
	SE_PAGE_SECURITY,
	SE_PAGE_ACCESSIBILITY,
	SE_PAGE_UPDATES,
	SE_PAGE_ABOUT,
	SE_PAGES
};

/*
 * The groups the list of pages is divided into, a thin line between two.
 */
enum se_group {
	SE_GROUP_NONE,
	SE_GROUP_CONNECTIVITY,
	SE_GROUP_PERSONALIZATION,
	SE_GROUP_DEVICES,
	SE_GROUP_SYSTEM
};

/*
 * The line pictures of the pages (glyphs.c), drawn in a square box.
 */
enum se_glyph {
	SE_GLYPH_GRID,
	SE_GLYPH_WIFI,
	SE_GLYPH_ETHERNET,
	SE_GLYPH_BLUETOOTH,
	SE_GLYPH_SHIELD,
	SE_GLYPH_GLOBE,
	SE_GLYPH_PALETTE,
	SE_GLYPH_PICTURE,
	SE_GLYPH_BELL,
	SE_GLYPH_SPEAKER,
	SE_GLYPH_MONITOR,
	SE_GLYPH_DISK,
	SE_GLYPH_BATTERY,
	SE_GLYPH_KEYBOARD,
	SE_GLYPH_MOUSE,
	SE_GLYPH_TOUCHPAD,
	SE_GLYPH_PRINTER,
	SE_GLYPH_SHARE,
	SE_GLYPH_PEOPLE,
	SE_GLYPH_EYE,
	SE_GLYPH_LOCK,
	SE_GLYPH_PERSON,
	SE_GLYPH_REFRESH,
	SE_GLYPH_INFO,
	SE_GLYPH_CHEVRON
};

struct se_app;

/*
 * One page of Settings: its identity, where the list shows it, its
 * words, and how it is drawn.
 *
 * The table of pages lives in pages.c for the whole run.  word names the
 * page on the command line (settings network).  ready is 0 for a page
 * that shows only its frame and "coming in a later version".  draw lays
 * the page's cards out from a top edge within a column and returns the
 * bottom edge of what it drew; press carries out a click on one of the
 * page's own controls (its hit index); key takes a key press first and
 * returns 1 when it used it (a text field has the keyboard); drag
 * follows a press held on one of the page's controls (a slider) through
 * its moves to its release; any may be NULL.
 */
struct se_page {
	unsigned id;
	unsigned group;
	unsigned glyph;
	const char *name;
	const char *summary;
	const char *word;
	const char *keywords;
	int ready;
	int (*draw)(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
	void (*press)(struct se_app *app, int index);
	int (*key)(struct se_app *app, const struct se_event *event);
	void (*drag)(struct se_app *app, int index, int x, unsigned phase);
};

/*
 * The phases of a drag on a page's control (a slider): the press, each
 * move while the button is held, and the release (wherever it lands).
 */
enum se_drag_phase {
	SE_DRAG_START,
	SE_DRAG_MOVE,
	SE_DRAG_END
};

/*
 * The kinds of clickable region a frame records.
 */
enum se_hit_kind {
	SE_HIT_NONE,
	SE_HIT_PAGE_ROW,
	SE_HIT_SIDEBAR,
	SE_HIT_PAGE,
	SE_HIT_TILE,
	SE_HIT_CONTROL,
	SE_HIT_RESULT
};

/*
 * One clickable region of the last frame: where it is, what it is and
 * which one (a page's ID, or a page's own control).
 */
struct se_hit {
	struct fm_rect rect;
	unsigned kind;
	int index;
};

/*
 * The panes of the last frame: the list of pages and the page, each a
 * card standing on zdesktop's frosted glass.  A hidden list has no size.
 */
struct se_layout {
	struct fm_rect sidebar;
	struct fm_rect page;
};

/* The kind of glass panel the window has: a card floating in the window. */
#define SE_PANEL_CARD		0U

/*
 * One part of the window that stands on zdesktop's frosted glass: its
 * rectangle in the window, its corners' radius and its kind.
 */
struct se_panel {
	struct fm_rect rect;
	int radius;
	unsigned kind;
};

/* How many networks, interfaces, DNS servers and saved networks Settings keeps. */
#define SE_NETWORK_SCAN		KEILAND_NETWORK_SCAN_MAX
#define SE_NETWORK_LINKS	KEILAND_NETWORK_LINKS_MAX
#define SE_NETWORK_DNS		KEILAND_NETWORK_DNS_MAX
#define SE_NETWORK_SAVED	32U

/* How many seconds of the network's activity the graph keeps (one sample a second). */
#define SE_USAGE_SAMPLES	120U

/* The bytes of a message the network pages show, and of a key typed (63 characters and its NUL). */
#define SE_MESSAGE		160U
#define SE_KEY_TEXT		64U

/*
 * The steps of joining a network whose key was just typed: the key is
 * saved, the daemon is told the saved networks changed, and the network is
 * joined.
 */
enum se_join_step {
	SE_JOIN_NONE,
	SE_JOIN_PROFILES,
	SE_JOIN_CONNECT
};

/*
 * A line of text being typed: its characters (ASCII) and how many there
 * are.  A key's text is wiped when the field is closed.
 */
struct se_field {
	char text[SE_KEY_TEXT];
	size_t length;
};

/*
 * What the network pages show, and what they have asked of the daemon
 * (network.c keeps it up to date; the host tests fill it by hand).
 *
 * state, scan, links, dns and saved are the last reports.  request is the
 * daemon's request outstanding (KEILAND_NETWORK_REQUEST_NONE when none);
 * join_step and join_ssid carry a join with a new key through its steps.
 * libkeiland carries one request at a time: a switch, a disconnect or a
 * join asked for while another request (a scan, usually) is out waits in
 * one slot -- pending_request (NONE when empty), the join's step and its
 * network -- and is sent when that one is answered (ws089-p012 C1, as the
 * system bar does since ws005-p019).  A scan is never kept.
 * scan_received tells that a scan's report arrived, so an empty list
 * means no network is in reach rather than none looked for yet.
 * key_ssid names the network whose key is being typed (empty when the key
 * form is closed).  The usage ring holds the bytes a second received and
 * sent, newest at usage_next - 1.
 */
struct se_network {
	int live;
	struct keiland_network *handle;
	struct keiland_network_state state;
	struct keiland_network_ap scan[SE_NETWORK_SCAN];
	size_t scan_count;
	uint64_t scanned_at;
	int scan_received;
	struct keiland_network_link links[SE_NETWORK_LINKS];
	size_t link_count;
	char dns[SE_NETWORK_DNS][KEILAND_NETWORK_ADDRESS_MAX];
	size_t dns_count;
	char saved[SE_NETWORK_SAVED][KEILAND_NETWORK_SSID_MAX];
	size_t saved_count;
	unsigned request;
	unsigned join_step;
	char join_ssid[KEILAND_NETWORK_SSID_MAX];
	unsigned pending_request;
	unsigned pending_step;
	char pending_ssid[KEILAND_NETWORK_SSID_MAX];
	char key_ssid[KEILAND_NETWORK_SSID_MAX];
	struct se_field key;
	int key_shown;
	char message[SE_MESSAGE];
	int message_bad;
	uint64_t polled_at;
	uint64_t sampled_at;
	uint64_t received_total;
	uint64_t sent_total;
	uint32_t received[SE_USAGE_SAMPLES];
	uint32_t sent[SE_USAGE_SAMPLES];
	unsigned usage_count;
	unsigned usage_next;
};

/*
 * What About shows of the machine, read once when the program starts
 * (about.c).  An empty text is a value that could not be read, and the
 * row is not shown.
 */
struct se_about {
	char kernel[160];
	char machine[80];
	char processor[64];
	char host[64];
	char graphics[128];
	char display[64];
	unsigned cores;
};

/*
 * The actions of the menus and the keys, carried out by se_ui_action.
 * Going to a page is SE_ACTION_PAGE_FIRST plus the page's ID.
 */
enum se_action {
	SE_ACTION_NONE,
	SE_ACTION_CLOSE_WINDOW,
	SE_ACTION_QUIT,
	SE_ACTION_BACK,
	SE_ACTION_FORWARD,
	SE_ACTION_HOME,
	SE_ACTION_SHOW_SIDEBAR,
	SE_ACTION_MINIMIZE,
	SE_ACTION_ZOOM,
	SE_ACTION_ABOUT,
	SE_ACTION_FIND,
	SE_ACTION_PAGE_FIRST = 100
};

/*
 * What the interface asks of the window, which the main loop carries out
 * once.
 */
enum se_request {
	SE_REQUEST_NONE,
	SE_REQUEST_CLOSE,
	SE_REQUEST_MINIMIZE,
	SE_REQUEST_ZOOM
};

/*
 * The controls of the window's titlebar (WS070's CONTROLS presentation,
 * drawn by zdesktop): their IDs in the model titlebar.c gives zdesktop.
 */
enum se_control {
	SE_CONTROL_NONE,
	SE_CONTROL_BACK,
	SE_CONTROL_FORWARD,
	SE_CONTROL_HOME,
	SE_CONTROL_PATH,
	SE_CONTROL_SEARCH,
	SE_CONTROL_SIDEBAR
};

/*
 * What the titlebar shows of the window's state: whether the history can
 * go back and forward, the parts of the breadcrumb, the search's query,
 * and whether the list of pages is shown.  focus_serial moves each time
 * the window asks for the search field to have the keyboard (Ctrl+F);
 * titlebar.c gives it the keyboard when the serial differs from the one
 * it last sent.  titlebar.c sends the state to zdesktop when it differs
 * from what the titlebar shows.
 */
struct se_titlebar_state {
	int can_back;
	int can_forward;
	int part_count;
	char parts[SE_CRUMBS][SE_TITLEBAR_PART];
	char query[SE_TITLEBAR_TEXT];
	unsigned focus_serial;
	int sidebar;
};

/*
 * What the titlebar tells the window.
 */
enum se_titlebar_kind {
	SE_TITLEBAR_ACTIVATED,
	SE_TITLEBAR_CHANGED,
	SE_TITLEBAR_DONE
};

/*
 * One thing done with the titlebar: a control chosen (with the
 * breadcrumb's part for the breadcrumb), a text control's text as typed,
 * or its editing ended, with its text.
 */
struct se_titlebar_event {
	unsigned kind;
	uint32_t id;
	uint32_t detail;
	char text[SE_TITLEBAR_TEXT];
};

/*
 * What the menus show of the window's state: the history's steps, the
 * list of pages shown, and the page shown (checked in the Go menu).
 */
struct se_menu_state {
	int can_back;
	int can_forward;
	int sidebar;
	unsigned page;
};

/* The bytes of a search's query with its NUL, and how many results it lists at most. */
#define SE_SEARCH_QUERY		128U
#define SE_SEARCH_RESULTS	48U

/*
 * One thing a search found: a page, or one setting on a page (setting is
 * then its name, NULL for the page itself).
 */
struct se_search_result {
	unsigned page;
	const char *setting;
};

/*
 * The search of the titlebar (search.c, ws089-p008): the query as typed,
 * and what it found.
 *
 * While the query has a word in it (active), the page pane shows the
 * results in place of the page, which stays the history's step; ending the
 * search (Esc, a result or a page chosen) shows the page again.  The
 * results point into the tables of pages and of settings, which live for
 * the whole run.  focus_serial moves when the field is to have the
 * keyboard (see se_titlebar_state).
 */
struct se_search {
	char query[SE_SEARCH_QUERY];
	int active;
	struct se_search_result results[SE_SEARCH_RESULTS];
	unsigned count;
	unsigned focus_serial;

	/*
	 * The result the keys have chosen (ws089-p012): the first of a new list,
	 * moved by Up and Down, and opened by Enter.  reveal asks the next frame
	 * to scroll it into sight after the keys moved it.
	 */
	unsigned chosen;
	int reveal;
};

/* How many pictures the Wallpaper page offers besides the default, and the bytes of a path. */
#define SE_WALLPAPERS		8U
#define SE_PATH			256U

/* How many file systems the Storage page shows. */
#define SE_VOLUMES		6U

/*
 * One picture the Wallpaper page offers: its file, the name shown (the
 * file's name without .ppm), and a small copy for its tile (empty until
 * the page is first shown, or when the file cannot be read).
 */
struct se_wallpaper {
	char path[SE_PATH];
	char name[64];
	struct fm_image thumbnail;
	int read;
};

/*
 * One file system the Storage page shows: where it is mounted and its
 * sizes in bytes.
 */
struct se_volume {
	char path[64];
	uint64_t total;
	uint64_t available;
	uint64_t used;
};

/*
 * The desktop's look and the user's preferences as Settings shows them
 * (look.c, ws089-p004).
 *
 * preferences is the user's file (NULL, with open_error, when there is no
 * home and nothing can be saved); it is read again once a second.  opacity
 * is the windows' opacity shown (a drag moves it before it is written),
 * and so are the pointer's speed (percent), natural scrolling and the
 * keyboards' repeat (keys a second, milliseconds before it starts);
 * wallpaper is the preferences' picture (empty for the default).  The
 * pictures are found and read when the Wallpaper page is first shown; the
 * default's (the session's --wallpaper) is wallpapers[0] when it exists.
 * The sliders' rectangles are the last frame's, for a drag (slider for
 * the opacity, sliders[] for the input pages' by their order).  The volumes are
 * read when the Storage page is shown.
 */
struct se_look {
	struct keiland_preferences *preferences;
	int open_error;
	uint64_t checked_at;
	int opacity;
	int pointer_speed;
	int pointer_natural;
	int repeat_rate;
	int repeat_delay;
	int dragging;
	struct fm_rect slider;
	struct fm_rect sliders[4];
	char wallpaper[SE_PATH];
	struct se_wallpaper wallpapers[SE_WALLPAPERS];
	unsigned wallpaper_count;
	int has_default;
	int scanned;
	struct se_volume volumes[SE_VOLUMES];
	unsigned volume_count;
	char message[SE_MESSAGE];
	int message_bad;
};

/*
 * The sound's volume as the Sound page shows and sets it (ws100-p005,
 * sound.c): the link to audiod (libkeiland's keiland_audio, NULL without
 * memory) and what it last reported, the volume and mute shown (audiod's,
 * or the one being set), a drag in progress, a volume or a feedback sound
 * held back (at most one every 50 and 250 milliseconds while dragging, as
 * the system bar does), and the slider's place in the last frame.
 */
struct se_sound {
	struct keiland_audio *audio;
	struct keiland_audio_state state;
	int value;
	int muted;
	int dragging;
	int send_waiting;
	int feedback_waiting;
	uint64_t sent_at;
	uint64_t feedback_at;
	struct fm_rect slider;
};

/*
 * A finger on the touch screen held on a pane, which a drag of it scrolls
 * (ws089-p012): where it touched, the pane and its scroll then, and whether
 * it has moved far enough to be a scroll rather than a tap.  held is 0 when
 * no finger is down (or the finger is on a slider, which it drags instead).
 */
struct se_touch_scroll {
	int held;
	int scrolling;
	unsigned pane;
	int start_x;
	int start_y;
	int start_scroll;
};

/*
 * Settings in one window: the page shown and its history, the list's and
 * the page's scroll, what the last frame drew, and what the pointer is
 * doing.
 *
 * One lives for the whole run.
 */
struct se_app {
	/* The fonts, the window's size, the time of the input being handled, and whether a new frame is due. */
	struct fm_text *text;
	int width;
	int height;
	uint64_t now;
	int dirty;

	/* Whether the window is glass (its ground clear, the panes on zdesktop's glass), docked (maximized), and has the focus. */
	int glass;
	int docked;
	int focused;

	/* Whether the list of pages is shown. */
	int show_sidebar;

	/* The page shown, and the history of pages: its steps and the one shown. */
	unsigned page;
	unsigned history[SE_HISTORY];
	int history_count;
	int history_index;

	/* How far the page and the list are scrolled, and how tall their content was in the last frame (pixels). */
	int page_scroll;
	int page_extent;
	int sidebar_scroll;
	int sidebar_extent;

	/* Whether the list is to scroll the page shown into sight at the next frame (after the page changed). */
	int reveal;

	/* A finger held on a pane, which a drag of it scrolls (ws089-p012). */
	struct se_touch_scroll touch;

	/* The panes of the last frame and its clickable regions. */
	struct se_layout layout;
	struct se_hit hits[SE_HITS];
	int hit_count;

	/*
	 * The page's controls as the log last listed them (a new frame whose
	 * controls moved, came or went lists them again, so that a test can
	 * find them).
	 */
	struct se_hit logged[SE_HITS];
	int logged_count;

	/* The region under the pointer (lit), and the one a press started on (a click lands on the same one). */
	unsigned hover_kind;
	int hover_index;
	unsigned press_kind;
	int press_index;

	/* The minute of the clock last drawn (About shows how long the machine has run). */
	uint64_t minute;

	/* What the window is asked to do (SE_REQUEST_*), taken by the main loop. */
	unsigned request;

	/* What About shows of the machine. */
	struct se_about about;

	/* The titlebar's search and what it found. */
	struct se_search search;

	/* What the network pages show and have asked of the daemon. */
	struct se_network network;

	/* The desktop's look and the user's preferences. */
	struct se_look look;

	/* The sound's volume (ws100-p005). */
	struct se_sound sound;
};

/* The table of pages (pages.c). */
extern const struct se_page se_pages[SE_PAGES];
const struct se_page *se_page_find(const char *word);

/* The interface (ui.c). */
void se_ui_init(struct se_app *app, struct fm_text *text, unsigned page);
void se_ui_event(struct se_app *app, const struct se_event *event);
void se_ui_action(struct se_app *app, uint32_t action);
void se_ui_tick(struct se_app *app, uint64_t now);
void se_ui_draw(struct se_app *app, struct fm_canvas *canvas);
size_t se_ui_panels(struct se_app *app, struct se_panel *panels, size_t capacity);
void se_ui_hit(struct se_app *app, const struct fm_rect *rect, unsigned kind, int index);
void se_ui_go(struct se_app *app, unsigned page);
void se_ui_titlebar_state(struct se_app *app, struct se_titlebar_state *state);
void se_ui_titlebar(struct se_app *app, const struct se_titlebar_event *event);
void se_ui_menu_state(struct se_app *app, struct se_menu_state *state);
int se_ui_lit(const struct se_app *app, unsigned kind, int index);
void se_log(const char *format, ...);

/* The parts pages are built of (widgets.c). */
int se_page_header(struct se_app *app, struct fm_canvas *canvas, const struct se_page *page, int x, int top, int width);
int se_card_begin(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width, int height, const char *title, const char *subtitle);
int se_card_height(int rows, int titled);
int se_row_value(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width, const char *label, const char *value, int last);
void se_mark_draw(struct fm_canvas *canvas, int x, int y, unsigned pixels, float opacity);
void se_toggle_draw(struct se_app *app, struct fm_canvas *canvas, int x, int y, int on, int enabled, int index);
int se_button_draw(struct se_app *app, struct fm_canvas *canvas, int x, int y, const char *label, int primary, int enabled, int index);
int se_button_width(struct se_app *app, const char *label);
void se_dot_draw(struct fm_canvas *canvas, float cx, float cy, fm_color color);
void se_signal_draw(struct fm_canvas *canvas, float x, float y, int rssi, fm_color color);
void se_bytes_text(uint64_t bytes, char *text, size_t size);

/* The line pictures (glyphs.c). */
void se_glyph_draw(struct fm_canvas *canvas, unsigned glyph, float x, float y, float size, fm_color color);

/* The network pages (page-network.c). */
int se_network_page_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
int se_wifi_page_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
int se_ethernet_page_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
void se_network_press(struct se_app *app, int index);
int se_network_key(struct se_app *app, const struct se_event *event);

/* The network's backend (network.c; the host tests have their own). */
void se_network_open(struct se_app *app);
void se_network_poll(struct se_app *app, uint64_t now);
int se_network_wait(struct se_app *app);
void se_network_close(struct se_app *app);
void se_network_wifi(struct se_app *app, int on);
void se_network_scan(struct se_app *app);
void se_network_join(struct se_app *app, const char *ssid);
void se_network_join_key(struct se_app *app, const char *ssid, const char *key);
void se_network_disconnect(struct se_app *app);

/* The text fields (widgets.c). */
int se_field_key(struct se_field *field, const struct se_event *event);
void se_field_clear(struct se_field *field);

/* The look and the preferences (look.c). */
void se_look_open(struct se_app *app);
void se_look_poll(struct se_app *app, uint64_t now);
void se_look_close(struct se_app *app);
void se_look_set_opacity(struct se_app *app, int percent);
void se_look_set_number(struct se_app *app, const char *key, int value, int fallback);
void se_look_set_wallpaper(struct se_app *app, int index);
void se_look_scan(struct se_app *app);
void se_look_volumes(struct se_app *app);
const char *se_look_wallpaper_name(const struct se_app *app);

/* The look's pages (page-look.c). */
int se_appearance_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
int se_wallpaper_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
int se_display_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
int se_storage_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
void se_look_press(struct se_app *app, int index);
void se_look_drag(struct se_app *app, int index, int x, unsigned phase);

/* The input and sound pages (page-input.c). */
int se_mouse_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
int se_keyboard_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
int se_sound_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
void se_input_press(struct se_app *app, int index);
void se_input_drag(struct se_app *app, int index, int x, unsigned phase);

/* The sound's volume (sound.c, ws100-p005), and its page's controls (hit indices). */
#define SE_SOUND_VOLUME		6
#define SE_SOUND_MUTE		7
void se_sound_open(struct se_app *app);
void se_sound_poll(struct se_app *app, uint64_t now);
int se_sound_wait(const struct se_app *app);
void se_sound_close(struct se_app *app);
void se_sound_press(struct se_app *app, int index);
void se_sound_drag(struct se_app *app, int index, int x, unsigned phase);
int se_sound_available(const struct se_app *app);

/* A slider (widgets.c). */
void se_slider_draw(struct se_app *app, struct fm_canvas *canvas, int x, int y, int width, float fraction, int enabled, int index, struct fm_rect *rect);
float se_slider_fraction(const struct fm_rect *rect, int x);

/* The search (search.c). */
void se_search_set(struct se_app *app, const char *query);
void se_search_end(struct se_app *app);
void se_search_focus(struct se_app *app);
int se_search_open_chosen(struct se_app *app);
void se_search_step(struct se_app *app, int direction);
int se_search_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
void se_search_press(struct se_app *app, int index);

/* The pages' drawing (page-home.c, page-about.c, page-soon.c). */
int se_home_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
int se_about_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);
int se_soon_draw(struct se_app *app, struct fm_canvas *canvas, int x, int top, int width);

/* What About shows of the machine (about.c). */
void se_about_read(struct se_about *about);

#endif
