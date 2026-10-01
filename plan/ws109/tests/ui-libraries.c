/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Uses installed native UI, PDF and actual service libraries through public headers.
 */
#include <keiland.h>
#include <keiui.h>
#include <pdf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int probe_canvas(void);
static int probe_pdf(struct pdf_writer *writer, const char *path);
static int probe_services(void);

/*
 * Checks real native service observations, CPU pixels and a private PDF round trip.
 */
int
main(
	void)
{
	struct pdf_writer *writer;
	char directory[] = "/tmp/ws109-ui-XXXXXX";
	char path[128];
	char *owned;
	unsigned version;
	int error;

	/* Requires the installed desktop library's public version contract. */
	version = keiland_version();
	if (version != KEILAND_VERSION)
		return 1;

	/* Requires the installed UI library's public version contract. */
	version = kui_version();
	if (version != KUI_VERSION)
		return 1;

	/* Observes the actual native services through the linked libkeiland. */
	error = probe_services();
	if (error != 0)
		return 1;

	/* Exercises shared UI rasterization without substituting an unavailable display. */
	error = probe_canvas();
	if (error != 0)
		return 1;

	/* Owns the PDF pathname exclusively for both writing and reading. */
	owned = mkdtemp(directory);
	if (owned == NULL)
		return 1;
	(void)snprintf(path, sizeof(path), "%s/document.pdf", directory);

	/* Acquires a real production writer before creating any document. */
	error = pdf_writer_create(&writer);
	if (error != 0) {
		(void)rmdir(directory);
		return 1;
	}

	/* Releases the writer and private file even if the round-trip assertion fails. */
	error = probe_pdf(writer, path);
	pdf_writer_destroy(writer);
	(void)unlink(path);
	(void)rmdir(directory);
	if (error != 0) {
		(void)fprintf(stderr, "native PDF library probe failed errno=%d\n", error);
		return 1;
	}

	/* Succeeded: installed public headers and real native libraries work together. */
	(void)printf("PASS native public libkeiland/OSS/network/libkeiui pixels/libpdf round trip\n");
	return 0;
}

/* Verifies every pixel of one bounded public canvas operation. */
static int
probe_canvas(
	void)
{
	struct kui_canvas canvas;
	struct kui_rect rectangle = {2, 1, 3, 2};
	uint32_t pixels[64];
	uint32_t expected;
	int x;
	int y;
	int error;

	/* Initializes a caller-owned surface through the installed shared UI library. */
	error = kui_canvas_init(&canvas, pixels, 8, 8, 8);
	if (error != 0)
		return error;

	/* Draws a bounded opaque rectangle over a transparent cleared surface. */
	kui_canvas_clear(&canvas);
	kui_canvas_fill(&canvas, &rectangle, 0xff010203U);
	kui_canvas_release(&canvas);

	/* Checks the complete surface so out-of-rectangle writes cannot pass. */
	for (y = 0; y < 8; y++) {
		/* Compares the public geometric contract independently of the raster algorithm. */
		for (x = 0; x < 8; x++) {
			/* Only the chosen rectangle's interior is opaque. */
			expected = 0;
			if (x >= 2 &&
			    x < 5 &&
			    y >= 1 &&
			    y < 3)
				expected = 0xff010203U;

			/* Any incorrect pixel invalidates the linked library's drawing contract. */
			if (pixels[y * 8 + x] != expected)
				return EPROTO;
		}
	}

	/* Succeeded: all64 actual CPU-rendered pixels match the public rectangle. */
	return 0;
}

/* Writes and reads one real document while retaining all owner cleanup in the caller. */
static int
probe_pdf(
	struct pdf_writer *writer,
	const char *path)
{
	struct pdf_document *document;
	struct pdf_page_box box;
	unsigned char written[32];
	unsigned char read_back[32];
	size_t count;
	int same;
	int error;

	/* Begins one page with dimensions that the independent reader must recover. */
	error = pdf_writer_begin_page(writer, 100, 80);
	if (error != 0)
		return error;

	/* Finishes the page before asking for its content digest. */
	error = pdf_writer_end_page(writer);
	if (error != 0)
		return error;

	/* Captures the writer's digest through the actual private digest dependency. */
	error = pdf_writer_get_page_content_hash(writer, 0, written);
	if (error != 0)
		return error;

	/* Saves the native file using the installed PDF writer. */
	error = pdf_writer_save(writer, path);
	if (error != 0)
		return error;

	/* Reopens the actual serialized bytes through the installed PDF reader. */
	error = pdf_document_open(path, &document);
	if (error != 0)
		return error;

	/* Retrieves dimensions without borrowing writer internals. */
	count = pdf_document_page_count(document);
	error = pdf_document_page_box(document, 0, &box);
	if (error != 0) {
		pdf_document_close(document);
		return error;
	}

	/* Computes the independently decoded page's content digest. */
	error = pdf_document_page_content_hash(document, 0, read_back);
	pdf_document_close(document);
	if (error != 0)
		return error;

	/* The serialized page count and dimensions must retain the writer's public inputs. */
	if (count != 1 ||
	    box.width != 100 ||
	    box.height != 80)
		return EPROTO;

	/* Independent writer and reader digest paths must agree on the same page bytes. */
	same = memcmp(written, read_back, sizeof(written));
	if (same != 0)
		return EPROTO;

	/* Succeeded: the native serialized page and its digest survived the public round trip. */
	return 0;
}

/* Observes real native OSS and addressed wired connectivity without changing either. */
static int
probe_services(
	void)
{
	struct keiland_audio *audio;
	struct keiland_audio_state sound;
	struct keiland_network *network;
	struct keiland_network_state state;
	unsigned changed;
	int error;

	/* Reads the actual OSS control through the installed libkeiland implementation. */
	audio = keiland_audio_open();
	if (audio == NULL)
		return ENOMEM;
	error = keiland_audio_update(audio, &changed);
	keiland_audio_get_state(audio, &sound);
	keiland_audio_close(audio);
	if (error != 0)
		return error;

	/* A successful placeholder cannot supply the required actual native output control. */
	if (sound.reachable != 1 || sound.device != 1)
		return EPROTO;

	/* Reads native addressed interfaces independently of an absent supplicant. */
	network = keiland_network_open();
	if (network == NULL)
		return ENOMEM;
	error = keiland_network_update(network, &changed);
	keiland_network_get_state(network, &state);
	keiland_network_close(network);
	if (error != 0)
		return error;

	/* The owned fixture's real wired connection must remain visible without WiFi. */
	if (state.connected != 1 || state.kind != KEILAND_NETWORK_WIRED)
		return EPROTO;

	/* Succeeded: native service observations came through the installed public library. */
	(void)printf("native OSS %u/%u muted=%u; wired %s\n", sound.left, sound.right, sound.muted, state.interface);
	return 0;
}
