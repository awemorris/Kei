/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The GLib probe (ws115-p005): exercises the parts of GLib the GTK port
 * stands on, on the target, and prints one PASS line or the step that failed.
 *
 *   glib-probe <schema-directory> <scratch-file>
 */

#include <stdio.h>
#include <string.h>

#include <gio/gio.h>
#include <glib-object.h>

/*
 * The probe's object type: an object with one signal carrying an int.
 *
 * The signal is created without a marshaller, so GObject calls the handler
 * through its generic marshaller, which is built on libffi.
 */
typedef struct {
	GObject parent;
} ProbeObject;

/*
 * The class of the probe's object type.
 */
typedef struct {
	GObjectClass parent_class;
} ProbeObjectClass;

/*
 * The value the signal handler last received; -1 until it runs.
 */
static gint probe_received = -1;

/*
 * The id of the probe object's "ping" signal, set when the class is made.
 */
static guint probe_ping_signal;

/*
 * The value the worker thread hands back; 0 until it runs.
 */
static gint probe_thread_result;

GType probe_object_get_type(void);
static void probe_object_class_init(ProbeObjectClass *klass);
static void probe_object_init(ProbeObject *object);
static void probe_on_ping(ProbeObject *object, gint value, gpointer data);
static gboolean probe_on_timeout(gpointer data);
static gpointer probe_thread_main(gpointer data);
static int probe_fail(const char *step);

G_DEFINE_TYPE(ProbeObject, probe_object, G_TYPE_OBJECT)

/*
 * Runs every probe step and reports the result.
 */
int
main(
	int argc,
	char **argv)
{
	ProbeObject *object;
	GMainLoop *loop;
	GFile *file;
	GError *error;
	gchar *contents;
	gsize length;
	gboolean written;
	gboolean loaded;
	GRegex *regex;
	GMatchInfo *match;
	gchar *captured;
	gboolean matched;
	GSettingsSchemaSource *source;
	GSettingsSchema *schema;
	GSettingsBackend *backend;
	GSettings *settings;
	gint count;
	GThread *thread;
	gint joined;
	int same;
	int compared;

	/* Refuses a call without the schema directory and the scratch file. */
	if (argc != 3) {
		fprintf(stderr, "usage: glib-probe <schema-directory> <scratch-file>\n");
		return 2;
	}

	/* Emits the signal; the generic marshaller has to deliver the int. */
	object = g_object_new(probe_object_get_type(), NULL);
	g_signal_connect(object, "ping", G_CALLBACK(probe_on_ping), NULL);
	g_signal_emit(object, probe_ping_signal, 0, 42);
	g_object_unref(object);
	if (probe_received != 42)
		return probe_fail("gobject-signal");

	/* Runs a main loop until a timeout ends it. */
	loop = g_main_loop_new(NULL, FALSE);
	g_timeout_add(50, probe_on_timeout, loop);
	g_main_loop_run(loop);
	g_main_loop_unref(loop);

	/* Writes the scratch file through GIO and reads it back. */
	error = NULL;
	file = g_file_new_for_path(argv[2]);
	written = g_file_replace_contents(file, "zedBSD GIO\n", 11, NULL, FALSE,
					  G_FILE_CREATE_NONE, NULL, NULL, &error);
	if (!written)
		return probe_fail("gio-write");
	loaded = g_file_load_contents(file, NULL, &contents, &length, NULL, &error);
	g_object_unref(file);
	if (!loaded)
		return probe_fail("gio-read");
	same = 0;
	if (length == 11) {
		/* The bytes read back are the bytes written. */
		compared = memcmp(contents, "zedBSD GIO\n", 11);
		if (compared == 0)
			same = 1;
	}
	g_free(contents);
	if (!same)
		return probe_fail("gio-contents");

	/* Matches a regular expression through PCRE2 and checks the capture. */
	regex = g_regex_new("z(ed)BSD", 0, 0, &error);
	if (regex == NULL)
		return probe_fail("regex-compile");
	matched = g_regex_match(regex, "on zedBSD", 0, &match);
	captured = g_match_info_fetch(match, 1);
	g_match_info_free(match);
	g_regex_unref(regex);
	same = 0;
	if (matched && captured != NULL) {
		/* The first group is the part in parentheses. */
		compared = strcmp(captured, "ed");
		if (compared == 0)
			same = 1;
	}
	g_free(captured);
	if (!same)
		return probe_fail("regex-match");

	/* Opens the probe schema from its directory. */
	source = g_settings_schema_source_new_from_directory(argv[1], NULL, TRUE, &error);
	if (source == NULL)
		return probe_fail("settings-source");
	schema = g_settings_schema_source_lookup(source, "org.zedbsd.GlibProbe", FALSE);
	g_settings_schema_source_unref(source);
	if (schema == NULL)
		return probe_fail("settings-schema");

	/* Reads the default and a written value through a memory backend. */
	backend = g_memory_settings_backend_new();
	settings = g_settings_new_full(schema, backend, NULL);
	g_settings_schema_unref(schema);
	g_object_unref(backend);
	count = g_settings_get_int(settings, "count");
	if (count != 7)
		return probe_fail("settings-default");
	g_settings_set_int(settings, "count", 9);
	count = g_settings_get_int(settings, "count");
	g_object_unref(settings);
	if (count != 9)
		return probe_fail("settings-write");

	/* Runs a worker thread and takes its result. */
	thread = g_thread_new("glib-probe", probe_thread_main, NULL);
	joined = GPOINTER_TO_INT(g_thread_join(thread));
	if (joined != 5 || probe_thread_result != 5)
		return probe_fail("thread");

	/* Reports that every step passed. */
	printf("glib-probe: PASS glib %u.%u.%u\n", glib_major_version, glib_minor_version,
	       glib_micro_version);

	/* Succeeded: every step passed. */
	return 0;
}

/* Creates the "ping" signal, which carries one int and has no marshaller. */
static void
probe_object_class_init(
	ProbeObjectClass *klass)
{
	probe_ping_signal = g_signal_new("ping", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
					 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_INT);
}

/* Initializes a probe object, which has no state. */
static void
probe_object_init(
	ProbeObject *object)
{
	(void)object;
}

/* Records the value the signal delivered. */
static void
probe_on_ping(
	ProbeObject *object,
	gint value,
	gpointer data)
{
	(void)object;
	(void)data;

	/* The value the step compares. */
	probe_received = value;
}

/* Ends the main loop the timeout was added to. */
static gboolean
probe_on_timeout(
	gpointer data)
{
	/* Stops the loop; the source is removed by returning FALSE. */
	g_main_loop_quit(data);
	return G_SOURCE_REMOVE;
}

/* Stores and returns the worker thread's result. */
static gpointer
probe_thread_main(
	gpointer data)
{
	(void)data;

	/* The value the joining thread compares. */
	probe_thread_result = 5;
	return GINT_TO_POINTER(5);
}

/* Reports a failed step and gives the exit status for it. */
static int
probe_fail(
	const char *step)
{
	/* The step the caller reads instead of PASS. */
	printf("glib-probe: FAIL %s\n", step);
	return 1;
}
