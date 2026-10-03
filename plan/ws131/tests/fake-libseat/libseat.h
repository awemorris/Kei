/* The declarations of libseat the FreeBSD seat uses, for host-seat-freebsd.c (ws131-p006); Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib */
struct libseat;
struct libseat_seat_listener {
	void (*enable_seat)(struct libseat *seat, void *userdata);
	void (*disable_seat)(struct libseat *seat, void *userdata);
};
struct libseat *libseat_open_seat(const struct libseat_seat_listener *listener, void *userdata);
int libseat_disable_seat(struct libseat *seat);
int libseat_close_seat(struct libseat *seat);
int libseat_open_device(struct libseat *seat, const char *path, int *fd);
int libseat_close_device(struct libseat *seat, int device_id);
int libseat_get_fd(struct libseat *seat);
int libseat_dispatch(struct libseat *seat, int timeout);
