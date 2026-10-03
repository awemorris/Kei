/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The interface between the Keiland compositor and its operating system
 * (WS131, plan/ws131/design.md section 3).
 *
 * libkeiland-backend holds every piece of the desktop that differs between
 * zedBSD, Linux and FreeBSD.  This header is the one interface they share:
 * each of userland/desktop/libkeiland-backend-zedbsd, -linux and -freebsd
 * implements it, and only the compositor calls it.  Applications never see
 * it; they reach the system through the compositor's extensions and
 * libkeiland.  Nothing here includes a header of the compositor or of
 * libkeiland, and an implementation reports to the compositor only through
 * the callbacks of struct kl_backend_host.
 *
 * The areas move here one at a time (plan/ws131/design.md section 7): the
 * network first, then the sound, the power, the seat and the session, the
 * input devices, the display and the GPU buffers.  An area an operating
 * system does not offer answers ENOTSUP.
 */

#ifndef KL_BACKEND_H
#define KL_BACKEND_H

#include <stddef.h>
#include <stdint.h>

struct pollfd;
struct kl_backend_input_caps;

/*
 * The revision of this interface.  The backend is a static library built
 * with the compositor from the same tree, so the number is only compared at
 * compile time; it is not an ABI version.
 */
#define KL_BACKEND_INTERFACE	1U

/*
 * The compositor's backend.
 *
 * One exists per compositor process, from kl_backend_open to
 * kl_backend_close.  It keeps what the operating system's areas need
 * between calls of the event loop.
 */
struct kl_backend;

/*
 * What the backend tells the compositor.
 *
 * The compositor fills it and hands it to kl_backend_open; the backend
 * keeps a copy.  Each area adds the callbacks it needs when it moves here,
 * and a callback is only called from kl_backend_poll_done or
 * kl_backend_tick, on the event loop's thread.  A callback never calls back
 * into the backend.  data is passed to every callback unchanged.
 */
struct kl_backend_host {
	void *data;
	void (*session_stop)(void *data, unsigned reason);
	void (*session_answer)(void *data, unsigned request, int error);
	void (*session_paused)(void *data);
	void (*session_resumed)(void *data);
	void (*input_paused)(void *data, const char *path);
	void (*input_resumed)(void *data, const char *path, int descriptor);
	void (*input_gone)(void *data, const char *path);
	int (*input_known)(void *data, const char *path);
	int (*input_found)(void *data, int descriptor, const char *path, const struct kl_backend_input_caps *caps);
};

/*
 * How the compositor is started.
 *
 * flags is 0 so far; the areas that need to know (a windowed run, the
 * descriptors sessiond hands over) add their fields when they move here.
 * greeter_descriptor is the login screen's descriptor to the session
 * manager (zedBSD's sessiond, --auth-fd), or -1 when the compositor is not
 * the login screen; session_descriptor is a session's descriptor to it
 * (--control-fd), or -1 when no session manager started the session.  The
 * session (ws131-p006) and the power (ws131-p005) speak on them; the
 * compositor makes them nonblocking and keeps them from its children.
 */
struct kl_backend_options {
	unsigned flags;
	int greeter_descriptor;
	int session_descriptor;
};

/*
 * Opens the backend.  Returns 0 and the backend through *backend, or an
 * errno value (ENOMEM).
 */
int kl_backend_open(const struct kl_backend_options *options, const struct kl_backend_host *host, struct kl_backend **backend);

/*
 * Closes the backend; NULL is allowed.
 */
void kl_backend_close(struct kl_backend *backend);

/*
 * Counts the descriptors the backend needs in the event loop's next poll.
 */
size_t kl_backend_poll_count(const struct kl_backend *backend);

/*
 * Fills the backend's kl_backend_poll_count descriptors, starting at
 * descriptors.
 */
void kl_backend_poll_fill(struct kl_backend *backend, struct pollfd *descriptors);

/*
 * Handles what poll reported for the descriptors kl_backend_poll_fill
 * filled.
 */
void kl_backend_poll_done(struct kl_backend *backend, const struct pollfd *descriptors);

/*
 * Lets the backend do the work that waits for time rather than for a
 * descriptor (now_ms is the compositor's monotonic clock).
 */
void kl_backend_tick(struct kl_backend *backend, uint64_t now_ms);

/*
 * The network (ws035-p013).
 *
 * The desktop's view of the network and its Wi-Fi switch, for the system
 * bar: whether the machine is connected and through what (a wired
 * interface, or a Wi-Fi network by its SSID), the networks the radio sees,
 * and the requests a user makes from a menu (join a network, disconnect,
 * turn Wi-Fi on or off).  The system's network daemon is behind it; the
 * desktop never speaks the daemon's protocol itself.
 *
 * Nothing here waits.  The state arrives when the daemon reports a change;
 * kl_backend_network_update reads what has arrived and says what changed.  A
 * request is sent at once and its answer arrives through the same update,
 * so a scan or a join that takes seconds does not stop the caller.  One
 * request is outstanding at a time (EBUSY otherwise).
 *
 * Every call that can fail returns 0 or an errno value: ENOENT (the daemon
 * is not running), EACCES or EPERM (the user may not look or act),
 * EBUSY, EINVAL, ENOMEM.
 */
struct kl_backend_network;

/* The longest SSID shown, as printable text with the terminating NUL. */
#define KL_BACKEND_NETWORK_SSID_MAX	33U

/* The longest interface name, with the terminating NUL. */
#define KL_BACKEND_NETWORK_NAME_MAX	16U

/* The most networks a scan keeps. */
#define KL_BACKEND_NETWORK_SCAN_MAX	24U

/* What carries the connection. */
#define KL_BACKEND_NETWORK_NONE		0U
#define KL_BACKEND_NETWORK_WIRED		1U
#define KL_BACKEND_NETWORK_WIFI		2U

/* The Wi-Fi's state. */
#define KL_BACKEND_WIFI_ABSENT		0U	/* no radio */
#define KL_BACKEND_WIFI_OFF		1U
#define KL_BACKEND_WIFI_SEARCHING		2U
#define KL_BACKEND_WIFI_CONNECTING	3U
#define KL_BACKEND_WIFI_CONNECTED		4U
#define KL_BACKEND_WIFI_DISCONNECTED	5U	/* on, and left unconnected by the user */

/* What kl_backend_network_update found (bits). */
#define KL_BACKEND_NETWORK_CHANGED_STATE	1U
#define KL_BACKEND_NETWORK_CHANGED_SCAN	2U
#define KL_BACKEND_NETWORK_CHANGED_DONE	4U

/* The requests. */
#define KL_BACKEND_NETWORK_REQUEST_NONE		0U
#define KL_BACKEND_NETWORK_REQUEST_SCAN		1U
#define KL_BACKEND_NETWORK_REQUEST_JOIN		2U
#define KL_BACKEND_NETWORK_REQUEST_DISCONNECT	3U
#define KL_BACKEND_NETWORK_REQUEST_WIFI_ON	4U
#define KL_BACKEND_NETWORK_REQUEST_WIFI_OFF	5U
#define KL_BACKEND_NETWORK_REQUEST_PROFILES	6U	/* the user's saved networks changed */

/*
 * The network as last reported: connected (an interface is up with an
 * address), through what and which interface, the wired interface that is
 * up with an address (empty when none, even while the Wi-Fi carries the
 * connection), and the Wi-Fi's state with the SSID of the network it is on
 * or joining (empty otherwise).  reachable is 0 while the daemon cannot be
 * reached.
 */
struct kl_backend_network_state {
	unsigned reachable;
	unsigned connected;
	unsigned kind;
	char interface[KL_BACKEND_NETWORK_NAME_MAX];
	char wired[KL_BACKEND_NETWORK_NAME_MAX];
	unsigned wifi;
	char wifi_interface[KL_BACKEND_NETWORK_NAME_MAX];
	char ssid[KL_BACKEND_NETWORK_SSID_MAX];
};

/*
 * One network a scan found: its SSID, its signal in dBm, and whether it
 * asks for a key.  The strongest of the access points of one SSID stands
 * for it.
 */
struct kl_backend_network_ap {
	char ssid[KL_BACKEND_NETWORK_SSID_MAX];
	int rssi;
	unsigned secured;
};

/*
 * Starts watching the network.  Returns NULL with errno set on ENOMEM; a
 * daemon that is not running yet is tried again by the updates.
 */
struct kl_backend_network *kl_backend_network_open(void);

/*
 * Stops watching and drops an outstanding request.
 */
void kl_backend_network_close(struct kl_backend_network *network);

/*
 * Reads what has arrived without waiting, and reconnects to a daemon that
 * went away (at most once a second).  *changed gets the
 * KL_BACKEND_NETWORK_CHANGED_* bits of what changed.
 */
int kl_backend_network_update(struct kl_backend_network *network, unsigned *changed);

/*
 * Copies the network's state as last reported.
 */
void kl_backend_network_get_state(const struct kl_backend_network *network, struct kl_backend_network_state *state);

/*
 * Copies up to capacity networks of the last scan, the strongest first,
 * and returns how many there are.
 */
size_t kl_backend_network_get_scan(const struct kl_backend_network *network, struct kl_backend_network_ap *aps, size_t capacity);

/*
 * Sends a request (KL_BACKEND_NETWORK_REQUEST_*; a join names the SSID, the
 * others take NULL).  A join uses the network's saved profile.
 */
int kl_backend_network_request(struct kl_backend_network *network, unsigned request, const char *ssid);

/*
 * Tells the request outstanding (KL_BACKEND_NETWORK_REQUEST_NONE when none),
 * or, after KL_BACKEND_NETWORK_CHANGED_DONE, the one that finished and its
 * errno value (0 when it succeeded) through *error.
 */
unsigned kl_backend_network_get_request(const struct kl_backend_network *network, int *error);

/*
 * The network's details for Settings (ws089-p003): each
 * interface as the kernel reports it, the DNS servers, and the keys of the
 * Wi-Fi networks the user has saved.  These read the kernel and the files
 * directly and do not wait for the daemon.
 *
 * A new network is joined with its key in three steps: the key is saved in
 * the user's credential store (kl_backend_network_save_key: /etc/wifi.conf for
 * root, the .wifi.conf of the passwd home otherwise), the daemon is told
 * the saved networks changed (KL_BACKEND_NETWORK_REQUEST_PROFILES), and the
 * network is joined (KL_BACKEND_NETWORK_REQUEST_JOIN).  A key is a WPA
 * passphrase of 8 to 63 characters; the daemon joins with the keys of the
 * user who turned the Wi-Fi on.
 */

/* The most interfaces and DNS servers reported, and an IPv4 address's text with its NUL. */
#define KL_BACKEND_NETWORK_LINKS_MAX	16U
#define KL_BACKEND_NETWORK_DNS_MAX		4U
#define KL_BACKEND_NETWORK_ADDRESS_MAX	16U

/* The shortest and the longest key. */
#define KL_BACKEND_NETWORK_KEY_MIN		8U
#define KL_BACKEND_NETWORK_KEY_MAX		63U

/*
 * One interface: its name, whether it is up and has its link, whether it
 * is the loopback, its IPv4 address and netmask (empty when it has none),
 * its hardware address and MTU, and the bytes it has received and sent.
 */
struct kl_backend_network_link {
	char name[KL_BACKEND_NETWORK_NAME_MAX];
	unsigned up;
	unsigned running;
	unsigned loopback;
	char address[KL_BACKEND_NETWORK_ADDRESS_MAX];
	char netmask[KL_BACKEND_NETWORK_ADDRESS_MAX];
	unsigned char hardware[6];
	unsigned mtu;
	uint64_t received_bytes;
	uint64_t sent_bytes;
};

/*
 * Copies up to capacity interfaces and returns how many there are (0 when
 * they cannot be read).
 */
size_t kl_backend_network_get_links(struct kl_backend_network_link *links, size_t capacity);

/*
 * Copies up to capacity DNS servers of /etc/resolv.conf (dotted IPv4) and
 * returns how many were copied.
 */
size_t kl_backend_network_get_dns(char (*servers)[KL_BACKEND_NETWORK_ADDRESS_MAX], size_t capacity);

/*
 * Saves the key of a Wi-Fi network in the user's credential store (joined
 * by itself from then on).  Returns 0 or an errno value (EINVAL for an SSID
 * or a key outside the bounds).
 */
int kl_backend_network_save_key(const char *ssid, const char *key);

/*
 * Copies up to capacity SSIDs the user has saved keys for and returns how
 * many there are (0 when none, or the store cannot be read).
 */
size_t kl_backend_network_get_saved(char (*ssids)[KL_BACKEND_NETWORK_SSID_MAX], size_t capacity);


/*
 * The sound output's volume (ws100-p003, ws131-p004).
 *
 * The device volume the system's sound service applies to everything it
 * plays, 0 to 100 per channel, and whether it is muted, for the system
 * bar: audiod on zedBSD, the ALSA mixer on Linux and the OSS mixer on
 * FreeBSD.  Nothing here waits: kl_backend_audio_update reads what has
 * arrived, and a set or the feedback sound is sent at once.  A service that
 * is not running is not a failure; the updates connect again, at most once
 * a second.
 */
struct kl_backend_audio;

/* What the sound service last reported. */
struct kl_backend_audio_state {
	unsigned reachable;	/* 0 while the service cannot be reached */
	unsigned device;	/* 0 when the service has no sound device */
	unsigned rate;		/* the device's rate, 0 unknown */
	unsigned channels;
	unsigned left;		/* 0..100 */
	unsigned right;		/* 0..100 */
	unsigned muted;		/* 0 or 1 */
};

/* What kl_backend_audio_update found changed. */
#define KL_BACKEND_AUDIO_CHANGED_REACHABLE	1U	/* the service came or went */
#define KL_BACKEND_AUDIO_CHANGED_VOLUME		2U	/* the volume or mute changed */

/*
 * Starts following the volume.  Returns NULL only without memory.
 */
struct kl_backend_audio *kl_backend_audio_open(void);

/*
 * Stops following the volume.
 */
void kl_backend_audio_close(struct kl_backend_audio *audio);

/*
 * The descriptor to poll for the service's reports, or -1 when there is
 * none (an OSS mixer is read by the periodic updates).
 */
int kl_backend_audio_fd(const struct kl_backend_audio *audio);

/*
 * Reads what has arrived without waiting, and connects again when the
 * connection went (at most once a second).  *changed has the
 * KL_BACKEND_AUDIO_CHANGED_* bits of what changed.  Returns 0, or EINVAL.
 */
int kl_backend_audio_update(struct kl_backend_audio *audio, unsigned *changed);

/*
 * Copies what the service last reported.
 */
void kl_backend_audio_get_state(const struct kl_backend_audio *audio, struct kl_backend_audio_state *state);

/*
 * Asks for a volume (0..100 each) and mute.  The new volume comes back
 * through kl_backend_audio_update.  Returns 0, ENOTCONN (not connected),
 * EINVAL (out of range) or the error of sending.
 */
int kl_backend_audio_set_volume(struct kl_backend_audio *audio, unsigned left, unsigned right, unsigned muted);

/*
 * Asks for the short feedback sound at the device volume (a service
 * without it stays silent).  Returns 0, ENOTCONN, or the error of sending.
 */
int kl_backend_audio_feedback(struct kl_backend_audio *audio);

/*
 * Tells whether the sound service runs: 1 when it does, 0 when it does
 * not.  It does not connect and does not wait; a service that runs may
 * still have no sound device (struct kl_backend_audio_state's device).
 */
int kl_backend_audio_available(void);


/*
 * The power (ws131-p005).
 *
 * The machine's power source and the actions a user may take on the
 * machine: power it off, restart it, suspend it.  An action is asked of
 * the system's session manager (zedBSD's sessiond from the login screen;
 * logind on Linux) and the machine then ends or sleeps.  On zedBSD
 * sessiond's answer comes as session_answer(KL_BACKEND_SESSION_POWER); on
 * Linux logind answers the call itself and the action's return value is
 * the answer.  One action is asked at a time.  The power source is not read on any system yet: the state
 * says unknown.
 */

/* The actions (kl_backend_power_action), and their bits in the state's actions. */
#define KL_BACKEND_POWER_POWEROFF	1U
#define KL_BACKEND_POWER_REBOOT		2U
#define KL_BACKEND_POWER_SUSPEND	3U
#define KL_BACKEND_POWER_ACTION_BIT(action)	(1U << (action))

/* Where the power comes from. */
#define KL_BACKEND_POWER_SOURCE_UNKNOWN	0U
#define KL_BACKEND_POWER_SOURCE_AC	1U
#define KL_BACKEND_POWER_SOURCE_BATTERY	2U

/*
 * The power as the backend knows it: the source, the battery's charge in
 * percent (-1 when unknown), whether it charges, and the
 * KL_BACKEND_POWER_ACTION_BIT of each action a user may take now (0 when
 * none: a zedBSD session cannot power the machine off, only the login
 * screen can).
 */
struct kl_backend_power_state {
	unsigned source;
	int percent;
	unsigned charging;
	unsigned actions;
};

/*
 * Copies the power's state.  Returns 0, or EINVAL without a backend.
 */
int kl_backend_power_get_state(const struct kl_backend *backend, struct kl_backend_power_state *state);

/*
 * Asks for an action (KL_BACKEND_POWER_*).  Returns 0 when it was asked,
 * ENOTSUP for an action not in the state's actions, EBUSY when one was
 * asked already, EINVAL, or the error of sending.
 */
int kl_backend_power_action(struct kl_backend *backend, unsigned action);


/*
 * The session (ws131-p006): the session manager that started the
 * compositor, and the hand-over of the display between the login screen
 * and a session.
 *
 * On zedBSD that is sessiond (plan/ws035/login-manager-design.md): the
 * login screen asks it to log a user in on its descriptor, a session asks
 * it to log out or to unlock its lock screen on its own descriptor, and
 * both say when they first take the display and when they have given it
 * back.  Elsewhere no session manager speaks to the compositor yet: every
 * call answers ENOTSUP and kl_backend_session_managed is 0.
 *
 * The answers and the end come through the host's callbacks, from
 * kl_backend_tick: session_answer(request, error) for the request asked
 * last (KL_BACKEND_SESSION_NONE for a line no request asked for), error 0
 * when it was granted, EACCES when refused (a wrong password), EIO when
 * the manager could not do it, EPROTO for a line not understood; and
 * session_stop(reason) when the compositor is to end.  For
 * KL_BACKEND_SESSION_QUIT and KL_BACKEND_SESSION_ENDED the compositor gives
 * the display back (its swapchain and lease) inside the callback, and the
 * backend tells the manager so when the callback returns.
 */

/* The requests a session_answer answers. */
#define KL_BACKEND_SESSION_NONE		0U
#define KL_BACKEND_SESSION_AUTH		1U	/* the login screen's log in */
#define KL_BACKEND_SESSION_UNLOCK	2U	/* a session's lock screen */
#define KL_BACKEND_SESSION_POWER	3U	/* kl_backend_power_action */

/* Why session_stop is called. */
#define KL_BACKEND_SESSION_QUIT		1U	/* the session's Log Out was answered: end */
#define KL_BACKEND_SESSION_ENDED	2U	/* the login screen's manager is done with it (a session is ready, or the manager went) */
#define KL_BACKEND_SESSION_UNANSWERED	3U	/* a Log Out had no answer in time: end anyway */
#define KL_BACKEND_SESSION_LOST		4U	/* the seat's authority failed or went away: end through the ordinary cleanup */

/*
 * Says the compositor is about to take the display for the first time,
 * and waits (at most 20 seconds) for the manager to let it: 0 when it did,
 * ETIMEDOUT when the wait ended without it (the display is taken anyway),
 * ENOTSUP without a session manager, or the error of saying so.
 */
int kl_backend_session_ready(struct kl_backend *backend);

/*
 * Asks the manager to log the session out.  Returns 0 when asked (the
 * answer is session_stop(KL_BACKEND_SESSION_QUIT), or
 * KL_BACKEND_SESSION_UNANSWERED after 30 seconds), ENOTSUP when no manager
 * started the session (the compositor simply ends), or the error of
 * asking.
 */
int kl_backend_session_logout(struct kl_backend *backend);

/*
 * Asks the manager to log user in with password (the login screen).
 * Returns 0 when asked, EBUSY while another request waits for its answer,
 * ENOTSUP, EINVAL, or the error of asking.  Nothing of the password is
 * kept.
 */
int kl_backend_session_authenticate(struct kl_backend *backend, const char *user, const char *password);

/*
 * Asks the manager to unlock the session's lock screen with password.
 * Returns as kl_backend_session_authenticate.
 */
int kl_backend_session_unlock(struct kl_backend *backend, const char *password);

/*
 * Tells whether a session manager started this session and still listens
 * (1), so that it can be locked and logged out through it, or not (0).
 */
int kl_backend_session_managed(const struct kl_backend *backend);


/*
 * The seat (ws131-p006): who may use the display and the input devices.
 *
 * Linux takes them through logind (the session's TakeDevice) or directly as
 * root, FreeBSD through seatd (libseat); zedBSD's compositor uses its own
 * kernel interfaces and no seat (every call answers ENOTSUP, and the
 * compositor does not open it).  On Linux the seat also puts a virtual
 * terminal on standard input in graphics mode while it is held.  kl_backend_seat_open takes the seat and
 * the primary display node before Vulkan opens; the input devices are
 * opened through the seat, one descriptor each, and the seat may take them
 * all away while another session has the display (a virtual terminal
 * switch) and give them back later.
 *
 * The callbacks come from kl_backend_poll_done:
 *   session_paused    stop drawing and close the output now; the seat is
 *                     told the compositor has let go when it returns
 *   session_resumed   the display may be opened again (the next frame)
 *   input_paused      stop reading the input of path; its descriptor stays
 *                     the seat's and is not closed
 *   input_resumed     read the input of path from descriptor from now on
 *                     (the old one is the seat's to close)
 *   input_gone        the input of path is gone; forget it without closing
 *                     its descriptor, which the seat has closed
 * A failed authority calls session_stop(KL_BACKEND_SESSION_LOST).
 *
 * The input devices' scan (keiland-backend-evdev.h, ws131-p007) asks
 * input_known(path) (1 when the compositor already reads the device) and
 * offers each new device with input_found(descriptor, path, caps), which
 * returns 1 when the compositor keeps it (it then owns the descriptor
 * until kl_backend_input_close) and 0 when the backend is to close it.
 */

/*
 * Takes the seat and the primary display node (KEILAND_DRM_DEVICE, or
 * /dev/dri/card0).  Returns 0, ENOTSUP where there is no seat, or an errno
 * value; kl_backend_seat_close is called after a failure too.
 */
int kl_backend_seat_open(struct kl_backend *backend);

/*
 * Returns every device and the seat (partial opens too).
 */
void kl_backend_seat_close(struct kl_backend *backend);

/*
 * The primary display node's descriptor (-1 while it is paused or not
 * taken) and path, for the display's acquisition.
 */
int kl_backend_seat_primary_fd(const struct kl_backend *backend);
const char *kl_backend_seat_primary_path(const struct kl_backend *backend);

/*
 * Tells whether the seat is paused (1: no drawing, no new input device).
 */
int kl_backend_seat_paused(const struct kl_backend *backend);

/*
 * Opens the input device at path through the seat: a nonblocking
 * descriptor, or -1 with errno (EAGAIN while paused).
 */
int kl_backend_seat_device_open(struct kl_backend *backend, const char *path);

/*
 * Returns an input device's descriptor to the seat.
 */
void kl_backend_seat_device_close(struct kl_backend *backend, int descriptor);

/*
 * Tells the seat that reading descriptor failed as revoked (ENODEV).
 * Returns 1 when the seat keeps the device for a later resume (the
 * compositor stops reading it and waits for input_resumed or input_gone),
 * 0 when the compositor closes it as usual.
 */
int kl_backend_seat_device_revoked(struct kl_backend *backend, int descriptor);

#endif
