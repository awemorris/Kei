/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The image and keymap probe (ws115-p008): exercises gdk-pixbuf, libxkbcommon
 * and libepoxy on the target, the way GTK will, and prints one PASS line or
 * the step that failed.
 *
 *   image-probe <image-directory> <compositor-keymap-file>
 *
 * gdk-pixbuf loads a PNG, a JPEG and a TIFF of known sizes.  libxkbcommon
 * builds the default keymap by name from xkeyboard-config, as GTK does when
 * it starts, and parses the keymap text the Keiland compositor sends; in
 * both, the key A gives "a" and, with Shift held, "A".  epoxy then reaches
 * zedBSD's EGL by its library name and asks it for its client extensions.
 */

#include <stdio.h>
#include <string.h>

#include <epoxy/egl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <xkbcommon/xkbcommon.h>

/*
 * The evdev key codes, plus the 8 X11 adds, of the keys the keymap steps press.
 */
#define PROBE_KEY_A (30 + 8)
#define PROBE_KEY_LEFT_SHIFT (42 + 8)

/*
 * The longest image path the probe builds.
 */
#define PROBE_PATH_MAX 512

/*
 * One image the probe loads and the size it has to come out at.
 */
struct probe_image {
	const char *name;
	int width;
	int height;
};

/*
 * The images, one per loader GTK relies on: libpng's and libjpeg-turbo's test
 * pictures and a libtiff test picture.
 */
static const struct probe_image probe_images[] = {
	{ "pngtest.png", 91, 69 },
	{ "testorig.jpg", 227, 149 },
	{ "rgb-3c-8b.tiff", 157, 151 },
};

static const char *probe_run(const char *image_directory, const char *keymap_path);
static gboolean probe_image_load(const char *image_directory, const struct probe_image *image);
static gboolean probe_keymap_names(struct xkb_context *context);
static gboolean probe_keymap_compositor(struct xkb_context *context, const char *keymap_path);
static gboolean probe_keymap_keys(struct xkb_keymap *keymap);
static gboolean probe_egl(void);

/*
 * Runs every probe step and reports the result.
 */
int
main(
	int argc,
	char **argv)
{
	const char *failed_step;

	/* Refuses a call without the image directory and the keymap file. */
	if (argc != 3) {
		fprintf(stderr, "usage: image-probe <image-directory> <compositor-keymap-file>\n");
		return 2;
	}

	/* Runs the steps, which stop at the first one that fails. */
	failed_step = probe_run(argv[1], argv[2]);
	if (failed_step != NULL) {
		printf("image-probe: FAIL %s\n", failed_step);
		return 1;
	}

	/* Reports that every step passed. */
	printf("image-probe: PASS gdk-pixbuf %s png jpeg tiff, xkbcommon names and compositor, epoxy egl\n",
	       GDK_PIXBUF_VERSION);

	/* Succeeded: every step passed. */
	return 0;
}

/* Runs the probe steps in order and names the first that fails. */
static const char *
probe_run(
	const char *image_directory,
	const char *keymap_path)
{
	struct xkb_context *context;
	gboolean passed;
	size_t i;

	/* Loads each image with the loader its format selects. */
	for (i = 0; i < sizeof(probe_images) / sizeof(probe_images[0]); i++) {
		passed = probe_image_load(image_directory, &probe_images[i]);
		if (!passed)
			return probe_images[i].name;
	}

	/* Makes the keymap context with the default data paths. */
	context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (context == NULL)
		return "xkb-context";

	/* Builds the default keymap by name from xkeyboard-config. */
	passed = probe_keymap_names(context);
	if (!passed) {
		xkb_context_unref(context);
		return "xkb-names";
	}

	/* Parses the keymap text the compositor sends. */
	passed = probe_keymap_compositor(context, keymap_path);
	xkb_context_unref(context);
	if (!passed)
		return "xkb-compositor";

	/* Reaches EGL through epoxy. */
	passed = probe_egl();
	if (!passed)
		return "epoxy-egl";

	/* Succeeded: no step failed. */
	return NULL;
}

/* Loads one image and checks the size it came out at. */
static gboolean
probe_image_load(
	const char *image_directory,
	const struct probe_image *image)
{
	char path[PROBE_PATH_MAX];
	GdkPixbuf *pixbuf;
	GError *error;
	int width;
	int height;

	/* Loads the image; gdk-pixbuf picks the loader from the file's contents. */
	snprintf(path, sizeof(path), "%s/%s", image_directory, image->name);
	error = NULL;
	pixbuf = gdk_pixbuf_new_from_file(path, &error);
	if (pixbuf == NULL) {
		printf("image-probe: %s: %s\n", image->name, error->message);
		g_error_free(error);
		return FALSE;
	}

	/* Reads the decoded size and releases the image. */
	width = gdk_pixbuf_get_width(pixbuf);
	height = gdk_pixbuf_get_height(pixbuf);
	g_object_unref(pixbuf);
	printf("image-probe: %s is %dx%d\n", image->name, width, height);

	/* Refuses an image that decoded to another size. */
	if (width != image->width || height != image->height)
		return FALSE;

	/* Succeeded: the image decoded at its size. */
	return TRUE;
}

/* Builds the default keymap by name and checks its letters. */
static gboolean
probe_keymap_names(
	struct xkb_context *context)
{
	struct xkb_keymap *keymap;
	gboolean passed;

	/* Builds the keymap GTK starts with: no names asks for the defaults. */
	keymap = xkb_keymap_new_from_names(context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (keymap == NULL)
		return FALSE;

	/* Presses A without and with Shift. */
	passed = probe_keymap_keys(keymap);
	xkb_keymap_unref(keymap);
	if (!passed)
		return FALSE;

	/* Succeeded: the data built a working keymap. */
	return TRUE;
}

/* Parses the compositor's keymap text and checks its letters. */
static gboolean
probe_keymap_compositor(
	struct xkb_context *context,
	const char *keymap_path)
{
	struct xkb_keymap *keymap;
	gchar *text;
	gsize length;
	GError *error;
	gboolean loaded;
	gboolean passed;

	/* Reads the text the compositor writes into the keymap descriptor. */
	error = NULL;
	loaded = g_file_get_contents(keymap_path, &text, &length, &error);
	if (!loaded) {
		printf("image-probe: %s: %s\n", keymap_path, error->message);
		g_error_free(error);
		return FALSE;
	}

	/* Parses it as GTK's Wayland backend does. */
	keymap = xkb_keymap_new_from_string(context, text, XKB_KEYMAP_FORMAT_TEXT_V1,
					    XKB_KEYMAP_COMPILE_NO_FLAGS);
	g_free(text);
	if (keymap == NULL)
		return FALSE;

	/* Presses A without and with Shift. */
	passed = probe_keymap_keys(keymap);
	xkb_keymap_unref(keymap);
	if (!passed)
		return FALSE;

	/* Succeeded: the compositor's keymap parses and types letters. */
	return TRUE;
}

/* Checks that A gives "a" alone and "A" with Shift held. */
static gboolean
probe_keymap_keys(
	struct xkb_keymap *keymap)
{
	struct xkb_state *state;
	xkb_keysym_t plain;
	xkb_keysym_t shifted;

	/* Makes a state with no modifier held. */
	state = xkb_state_new(keymap);
	if (state == NULL)
		return FALSE;

	/* Reads A, then holds Shift and reads it again. */
	plain = xkb_state_key_get_one_sym(state, PROBE_KEY_A);
	xkb_state_update_key(state, PROBE_KEY_LEFT_SHIFT, XKB_KEY_DOWN);
	shifted = xkb_state_key_get_one_sym(state, PROBE_KEY_A);
	xkb_state_unref(state);

	/* Refuses a keymap that does not give the two letters. */
	if (plain != XKB_KEY_a || shifted != XKB_KEY_A)
		return FALSE;

	/* Succeeded: both letters came out. */
	return TRUE;
}

/* Asks EGL for its client extensions through epoxy. */
static gboolean
probe_egl(void)
{
	const char *extensions;

	/*
	 * The first EGL call makes epoxy open the EGL library by name; with the
	 * wrong name it would print "Couldn't open" and abort the probe.
	 */
	extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
	if (extensions == NULL)
		return FALSE;

	/* Records what EGL offers before any display exists. */
	printf("image-probe: EGL client extensions: %s\n", extensions);

	/* Succeeded: epoxy reached zedBSD's EGL. */
	return TRUE;
}
