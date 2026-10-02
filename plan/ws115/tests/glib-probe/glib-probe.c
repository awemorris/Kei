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

/*
 * The memory settings backend is declared only for callers that ask for the
 * backend interface, and the request has to come before the GIO headers.
 */
#define G_SETTINGS_ENABLE_BACKEND

#include <stdio.h>
#include <string.h>

#include <gio/gio.h>
#include <gio/gsettingsbackend.h>
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
static void probe_object_finalize(GObject *object);
static const char *probe_run(const char *schema_directory, const char *scratch_path);
static gboolean probe_signal(void);
static void probe_main_loop(void);
static gboolean probe_file(const char *scratch_path);
static gboolean probe_regex(void);
static gboolean probe_settings(const char *schema_directory);
static gboolean probe_thread(void);
static void probe_on_ping(ProbeObject *object, gint value, gpointer data);
static gboolean probe_on_timeout(gpointer data);
static gpointer probe_thread_main(gpointer data);
static void probe_report_error(GError *error);

G_DEFINE_TYPE(ProbeObject, probe_object, G_TYPE_OBJECT)

/*
 * Runs every probe step and reports the result.
 */
int
main(
	int argc,
	char **argv)
{
	const char *failed_step;

	/* Refuses a call without the schema directory and the scratch file. */
	if (argc != 3) {
		fprintf(stderr, "usage: glib-probe <schema-directory> <scratch-file>\n");
		return 2;
	}

	/* Runs the steps, which stop at the first one that fails. */
	failed_step = probe_run(argv[1], argv[2]);
	if (failed_step != NULL) {
		printf("glib-probe: FAIL %s\n", failed_step);
		return 1;
	}

	/* Reports that every step passed, with the GLib release that ran them. */
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
	GObjectClass *object_class;

	/* Chains destruction up to GObject through the probe's finalizer. */
	object_class = G_OBJECT_CLASS(klass);
	object_class->finalize = probe_object_finalize;

	/* The signal the probe emits; a null marshaller selects the generic one. */
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

/* Releases a probe object through its parent class. */
static void
probe_object_finalize(
	GObject *object)
{
	GObjectClass *parent_class;

	/* Lets GObject release what the base instance holds. */
	parent_class = G_OBJECT_CLASS(probe_object_parent_class);
	parent_class->finalize(object);
}

/* Runs the probe steps in order and names the first that fails. */
static const char *
probe_run(
	const char *schema_directory,
	const char *scratch_path)
{
	gboolean passed;

	/* Delivers an int through the libffi-based generic marshaller. */
	passed = probe_signal();
	if (!passed)
		return "gobject-signal";

	/* Runs a main loop until a timeout source ends it. */
	probe_main_loop();

	/* Writes the scratch file through GIO and reads the same bytes back. */
	passed = probe_file(scratch_path);
	if (!passed)
		return "gio-file";

	/* Matches a regular expression through PCRE2. */
	passed = probe_regex();
	if (!passed)
		return "regex";

	/* Reads and writes the probe schema's key through the memory backend. */
	passed = probe_settings(schema_directory);
	if (!passed)
		return "settings";

	/* Runs a worker thread and joins it for its result. */
	passed = probe_thread();
	if (!passed)
		return "thread";

	/* Succeeded: no step failed. */
	return NULL;
}

/* Emits the probe object's signal and checks the value its handler saw. */
static gboolean
probe_signal(void)
{
	ProbeObject *object;

	/* Emits 42 to a connected handler and drops the object again. */
	object = g_object_new(probe_object_get_type(), NULL);
	g_signal_connect(object, "ping", G_CALLBACK(probe_on_ping), NULL);
	g_signal_emit(object, probe_ping_signal, 0, 42);
	g_object_unref(object);

	/* Refuses a handler that did not receive the emitted value. */
	if (probe_received != 42)
		return FALSE;

	/* Succeeded: the generic marshaller delivered the int. */
	return TRUE;
}

/* Runs a main loop that a 50 ms timeout quits. */
static void
probe_main_loop(void)
{
	GMainLoop *loop;

	/* Spins the default context until the timeout fires. */
	loop = g_main_loop_new(NULL, FALSE);
	g_timeout_add(50, probe_on_timeout, loop);
	g_main_loop_run(loop);
	g_main_loop_unref(loop);
}

/* Replaces the scratch file's contents through GIO and reads them back. */
static gboolean
probe_file(
	const char *scratch_path)
{
	GFile *file;
	GError *error;
	gchar *contents;
	gsize length;
	gboolean written;
	gboolean loaded;
	int compared;

	/* Writes the probe's line over whatever the scratch file held. */
	error = NULL;
	file = g_file_new_for_path(scratch_path);
	written = g_file_replace_contents(file, "zedBSD GIO\n", 11, NULL, FALSE,
					  G_FILE_CREATE_NONE, NULL, NULL, &error);
	if (!written) {
		probe_report_error(error);
		g_object_unref(file);
		return FALSE;
	}

	/* Loads the file back whole. */
	loaded = g_file_load_contents(file, NULL, &contents, &length, NULL, &error);
	g_object_unref(file);
	if (!loaded) {
		probe_report_error(error);
		return FALSE;
	}

	/* Refuses contents of another length than the line written. */
	if (length != 11) {
		g_free(contents);
		return FALSE;
	}

	/* Compares the bytes read back with the bytes written. */
	compared = memcmp(contents, "zedBSD GIO\n", 11);
	g_free(contents);
	if (compared != 0)
		return FALSE;

	/* Succeeded: GIO wrote and read the same bytes. */
	return TRUE;
}

/* Matches a pattern with one group and checks what the group captured. */
static gboolean
probe_regex(void)
{
	GRegex *regex;
	GMatchInfo *match;
	GError *error;
	gchar *captured;
	gboolean matched;
	int compared;

	/* Compiles the pattern through PCRE2. */
	error = NULL;
	regex = g_regex_new("z(ed)BSD", 0, 0, &error);
	if (regex == NULL) {
		probe_report_error(error);
		return FALSE;
	}

	/* Matches the subject and takes the first group out of the match. */
	matched = g_regex_match(regex, "on zedBSD", 0, &match);
	captured = g_match_info_fetch(match, 1);
	g_match_info_free(match);
	g_regex_unref(regex);

	/* Refuses a subject the pattern did not match. */
	if (!matched || captured == NULL) {
		g_free(captured);
		return FALSE;
	}

	/* The first group is the part in parentheses. */
	compared = strcmp(captured, "ed");
	g_free(captured);
	if (compared != 0)
		return FALSE;

	/* Succeeded: PCRE2 matched and captured the group. */
	return TRUE;
}

/* Reads the schema default and a written value through a memory backend. */
static gboolean
probe_settings(
	const char *schema_directory)
{
	GSettingsSchemaSource *source;
	GSettingsSchema *schema;
	GSettingsBackend *backend;
	GSettings *settings;
	GError *error;
	gint count;

	/* Opens the compiled schemas in the probe's own directory. */
	error = NULL;
	source = g_settings_schema_source_new_from_directory(schema_directory, NULL, TRUE,
							     &error);
	if (source == NULL) {
		probe_report_error(error);
		return FALSE;
	}

	/* Finds the probe schema among them. */
	schema = g_settings_schema_source_lookup(source, "org.zedbsd.GlibProbe", FALSE);
	g_settings_schema_source_unref(source);
	if (schema == NULL)
		return FALSE;

	/* Binds the schema to a backend that keeps values in memory only. */
	backend = g_memory_settings_backend_new();
	settings = g_settings_new_full(schema, backend, NULL);
	g_settings_schema_unref(schema);
	g_object_unref(backend);

	/* Refuses a key that does not start at the schema's default of 7. */
	count = g_settings_get_int(settings, "count");
	if (count != 7) {
		g_object_unref(settings);
		return FALSE;
	}

	/* Writes 9 and reads the key again. */
	g_settings_set_int(settings, "count", 9);
	count = g_settings_get_int(settings, "count");
	g_object_unref(settings);
	if (count != 9)
		return FALSE;

	/* Succeeded: the default and the written value both came back. */
	return TRUE;
}

/* Starts a worker thread and checks the value it hands back. */
static gboolean
probe_thread(void)
{
	GThread *thread;
	gpointer joined;

	/* Runs the worker and waits for it to end. */
	thread = g_thread_new("glib-probe", probe_thread_main, NULL);
	joined = g_thread_join(thread);

	/* Refuses a join that did not return the worker's value. */
	if (GPOINTER_TO_INT(joined) != 5)
		return FALSE;

	/* Refuses a worker whose store the joining thread cannot see. */
	if (probe_thread_result != 5)
		return FALSE;

	/* Succeeded: the worker ran and its result reached this thread. */
	return TRUE;
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

	/* The value the signal step compares. */
	probe_received = value;
}

/* Ends the main loop the timeout was added to. */
static gboolean
probe_on_timeout(
	gpointer data)
{
	/* Stops the loop; returning REMOVE drops the timeout source. */
	g_main_loop_quit(data);
	return G_SOURCE_REMOVE;
}

/* Stores and returns the worker thread's result. */
static gpointer
probe_thread_main(
	gpointer data)
{
	(void)data;

	/* The value the joining thread compares, both stored and returned. */
	probe_thread_result = 5;
	return GINT_TO_POINTER(5);
}

/* Prints why a GLib call failed and releases the error. */
static void
probe_report_error(
	GError *error)
{
	/* A failure that set no error leaves nothing to print. */
	if (error == NULL)
		return;

	/* The message the step's FAIL line is read with. */
	printf("glib-probe: error: %s\n", error->message);
	g_error_free(error);
}
