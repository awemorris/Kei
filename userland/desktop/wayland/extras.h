/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The protocols toolkits use besides the core and xdg-shell (ws035-p080):
 * xdg-decoration (decoration.c), cursor-shape (cursor.c) and viewporter
 * (viewport.c).
 */

#ifndef ZWL_EXTRAS_H
#define ZWL_EXTRAS_H

#include "compose.h"

int zwl_decoration_request(struct zwl_object *object, uint32_t opcode, const unsigned char *bytes, size_t size);
void zwl_decoration_object_gone(struct zwl_object *object);
int zwl_decoration_configure(struct zwl_object *surface, uint32_t serial);
int zwl_decoration_ack(struct zwl_object *surface, uint32_t serial);
void zwl_decoration_commit(struct zwl_object *surface);
int zwl_decoration_server(const struct zwl_object *surface);
int zwl_decoration_native_changed(struct zwl_object *toplevel);
void zwl_decoration_geometry(const struct zwl_object *surface, uint32_t *width, uint32_t *height);

int zwl_cursor_shape_request(struct zwl_object *object, uint32_t opcode, const unsigned char *bytes, size_t size);
void zwl_cursor_shape_object_gone(struct zwl_object *object);
void zwl_cursor_images_destroy(struct zwl_server *server);
const struct zwl_import *zwl_cursor_image(const struct zwl_server *server, int32_t *hotspot_x, int32_t *hotspot_y);
void zwl_cursor_frame(struct zwl_server *server, uint32_t edges);

int zwl_viewport_request(struct zwl_object *object, uint32_t opcode, const unsigned char *bytes, size_t size);
void zwl_viewport_commit(struct zwl_object *surface);
void zwl_viewport_object_gone(struct zwl_object *object);
void zwl_surface_size(const struct zwl_object *surface, uint32_t *width, uint32_t *height);
void zwl_viewport_source(const struct zwl_object *surface, float *uv);

#endif
