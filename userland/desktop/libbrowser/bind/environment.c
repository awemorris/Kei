/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The window's environment (ws074-p077): navigator, screen, performance
 * and location, the Image constructor, and the window's other plain
 * properties (devicePixelRatio, the outer size, top and the like) that
 * page scripts read before they do anything else.
 *
 * navigator, screen, performance and location are one object each per
 * window, made from the prototypes of their interfaces.  What never
 * changes (the navigator's names and flags, the screen's depth) is a
 * data property of the prototype, so an own-property test on the object
 * fails as it does in other browsers; what changes (the screen's size,
 * the clock, the location) is an accessor.  The location is read from the
 * host (the page's URL); setting it, which navigates, is not in this pass.
 * navigator.sendBeacon accepts a beacon without sending it.
 */

#include "bind/internal.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* The processor's name in navigator.platform, as the compiler targets it. */
#if defined(__x86_64__)
#define ENVIRONMENT_MACHINE	"x86_64"
#elif defined(__aarch64__)
#define ENVIRONMENT_MACHINE	"aarch64"
#elif defined(__i386__)
#define ENVIRONMENT_MACHINE	"i686"
#else
#define ENVIRONMENT_MACHINE	"unknown"
#endif

/* The colour depth of the screen, in bits per pixel (the surfaces are 32-bit, with 24 bits of colour). */
#define ENVIRONMENT_COLOR_DEPTH	24

/*
 * A string property of an interface's prototype that does not change for
 * the life of the program.
 */
struct environment_string {
	const char *name;
	const char *value;
};

/*
 * A number property of an interface's prototype that does not change for
 * the life of the program.
 */
struct environment_number {
	const char *name;
	double value;
};

/*
 * A Boolean property of an interface's prototype that does not change for
 * the life of the program.
 */
struct environment_flag {
	const char *name;
	int value;
};

/*
 * One accessor whose getter reads one part of something (the part is the
 * getter function's data): its name and the part.
 */
struct environment_part {
	const char *name;
	int part;
};

/* A pending Fetch promise, rooted until its host request completes. */
struct environment_fetch_pending {
	struct bind_window *window;
	struct vm_cell *promise;
	struct environment_fetch_pending *next;
};

/* One MutationObserver and its current target and pending records. */
struct environment_mutation_observer {
	struct vm_cell *observer;
	struct vm_cell *callback;
	struct vm_cell *target;
	struct vm_cell *records;
	int child_list;
	int subtree;
	struct environment_mutation_observer *next;
};

static int environment_install_navigator(struct bind_window *window);
static int environment_install_screen(struct bind_window *window);
static int environment_install_performance(struct bind_window *window);
static int environment_install_location(struct bind_window *window);
static int environment_install_image(struct bind_window *window);
static int environment_install_xhr(struct bind_window *window);
static int environment_install_observers(struct bind_window *window);
static int environment_install_window(struct bind_window *window);
static int environment_instance(struct bind_window *window, int interface, const char *global, struct vm_object **object);
static int environment_strings(struct vm_realm *realm, struct vm_object *object, const struct environment_string *table);
static int environment_numbers(struct vm_realm *realm, struct vm_object *object, const struct environment_number *table);
static int environment_flags(struct vm_realm *realm, struct vm_object *object, const struct environment_flag *table);
static int environment_part_accessor(struct vm_realm *realm, struct vm_object *object, const char *name, vm_native getter, int part);
static int environment_empty_array(struct vm_realm *realm, vm_value *value);
static int environment_navigator_app_version(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_navigator_user_agent(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_navigator_languages(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_navigator_list(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_navigator_java_enabled(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_navigator_send_beacon(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_screen_size(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_performance_now(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_performance_time_origin(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_performance_entries(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_performance_nothing(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_location_part(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_location_to_string(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_image_construct(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_outer_size(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_atob(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_btoa(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_fetch(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static void environment_fetch_done(void *context, int error, int response_status, const unsigned char *bytes, size_t length, const char *url);
static int environment_response_text(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_response_json(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_headers_get(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_xhr_construct(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_xhr_open(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_xhr_send(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_xhr_abort(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_xhr_header(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_xhr_headers(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_xhr_nothing(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_xhr_put(struct vm_realm *realm, vm_value object, const char *name, vm_value value);
static int environment_xhr_get(struct vm_realm *realm, vm_value object, const char *name, vm_value *value);
static int environment_xhr_fire(struct vm_realm *realm, vm_value object, const char *type);
static int environment_response(struct vm_realm *realm, const struct wb_buffer *bytes, const struct wb_buffer *url, int response_status, vm_value *result);
static int environment_settle_promise(struct vm_realm *realm, vm_value value, int reject, vm_value *result);
static int environment_observer_construct(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_observer_observe(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_observer_disconnect(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_observer_records(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int environment_observer_entry(struct bind_window *window, vm_value target, int type, vm_value *entry);
static int environment_observer_rect(struct vm_realm *realm, double x, double y, double width, double height, vm_value *result);
static int environment_observer_size(struct vm_realm *realm, double width, double height, vm_value *result);
static int environment_mutation_add(struct bind_window *window, struct vm_object *observer, vm_value callback);
static struct environment_mutation_observer *environment_mutation_find(struct bind_window *window, vm_value observer);
static void environment_mutation_clear(struct bind_window *window, struct environment_mutation_observer *observer);
static int environment_mutation_options(struct vm_realm *realm, vm_value options, int *child_list, int *subtree);
static int environment_mutation_record(struct bind_window *window, struct environment_mutation_observer *observer, struct dom_node *parent, struct dom_node *added, struct dom_node *removed);
static int environment_node_list(struct bind_window *window, struct dom_node *node, vm_value *result);
static int environment_base64_value(uint16_t unit);

/*
 * The screen's sizes (the part is 0 for the width and 1 for the height).
 * The table is constant for the life of the program.
 */
static const struct environment_part environment_screen_sizes[] = {
	{ "width", 0 },
	{ "height", 1 },
	{ "availWidth", 0 },
	{ "availHeight", 1 },
	{ NULL, 0 }
};

/*
 * The parts of the location, each a BIND_LOCATION_* the host reads.  The
 * table is constant for the life of the program.
 */
static const struct environment_part environment_location_parts[] = {
	{ "href", BIND_LOCATION_HREF },
	{ "origin", BIND_LOCATION_ORIGIN },
	{ "protocol", BIND_LOCATION_PROTOCOL },
	{ "host", BIND_LOCATION_HOST },
	{ "hostname", BIND_LOCATION_HOSTNAME },
	{ "port", BIND_LOCATION_PORT },
	{ "pathname", BIND_LOCATION_PATHNAME },
	{ "search", BIND_LOCATION_SEARCH },
	{ "hash", BIND_LOCATION_HASH },
	{ NULL, 0 }
};

/*
 * The navigator's names that never change.  The table is constant for the
 * life of the program.
 */
static const struct environment_string environment_navigator_strings[] = {
	{ "appCodeName", "Mozilla" },
	{ "appName", "Netscape" },
	{ "platform", "Kei " ENVIRONMENT_MACHINE },
	{ "product", "Gecko" },
	{ "productSub", "20030107" },
	{ "vendor", "" },
	{ "vendorSub", "" },
	{ "language", "en-US" },
	{ NULL, NULL }
};

/*
 * The navigator's flags: cookies are kept, the network is taken to be
 * there, and nothing drives the browser.  The table is constant for the
 * life of the program.
 */
static const struct environment_flag environment_navigator_flags[] = {
	{ "cookieEnabled", 1 },
	{ "onLine", 1 },
	{ "webdriver", 0 },
	{ "pdfViewerEnabled", 0 },
	{ NULL, 0 }
};

/*
 * The navigator's numbers: one processor for the page's scripts, and no
 * touch points.  The table is constant for the life of the program.
 */
static const struct environment_number environment_navigator_numbers[] = {
	{ "hardwareConcurrency", 1.0 },
	{ "maxTouchPoints", 0.0 },
	{ NULL, 0.0 }
};

/*
 * The screen's numbers that never change.  The table is constant for the
 * life of the program.
 */
static const struct environment_number environment_screen_numbers[] = {
	{ "colorDepth", ENVIRONMENT_COLOR_DEPTH },
	{ "pixelDepth", ENVIRONMENT_COLOR_DEPTH },
	{ "availLeft", 0.0 },
	{ "availTop", 0.0 },
	{ NULL, 0.0 }
};

/*
 * The names of the moments of performance.timing, which are all the
 * moment the window was made in this pass (the loader does not report its
 * moments).  The table is constant for the life of the program.
 */
static const char *const environment_timing_names[] = {
	"navigationStart", "unloadEventStart", "unloadEventEnd", "redirectStart", "redirectEnd", "fetchStart",
	"domainLookupStart", "domainLookupEnd", "connectStart", "connectEnd", "secureConnectionStart", "requestStart",
	"responseStart", "responseEnd", "domLoading", "domInteractive", "domContentLoadedEventStart",
	"domContentLoadedEventEnd", "domComplete", "loadEventStart", "loadEventEnd", NULL
};

/*
 * The operations of Navigator.  The table is constant for the life of the
 * program.
 */
static const struct bind_operation environment_navigator_operations[] = {
	{ "javaEnabled", 0, environment_navigator_java_enabled },
	{ "sendBeacon", 1, environment_navigator_send_beacon },
	{ NULL, 0, NULL }
};

/*
 * The attributes of Navigator that depend on the host.  The table is
 * constant for the life of the program.
 */
static const struct bind_attribute environment_navigator_attributes[] = {
	{ "userAgent", environment_navigator_user_agent, NULL },
	{ "appVersion", environment_navigator_app_version, NULL },
	{ "languages", environment_navigator_languages, NULL },
	{ "plugins", environment_navigator_list, NULL },
	{ "mimeTypes", environment_navigator_list, NULL },
	{ NULL, NULL, NULL }
};

/*
 * The Navigator interface (the names and flags are added to its
 * prototype by environment_install_navigator).
 */
const struct bind_interface bind_navigator_interface = {
	"Navigator", BIND_NO_PARENT, 0, NULL, environment_navigator_attributes, environment_navigator_operations, NULL
};

/*
 * The Screen interface (its sizes and depths are added to its prototype
 * by environment_install_screen).
 */
const struct bind_interface bind_screen_interface = {
	"Screen", BIND_NO_PARENT, 0, NULL, NULL, NULL, NULL
};

/*
 * The attributes of Performance.  The table is constant for the life of
 * the program.
 */
static const struct bind_attribute environment_performance_attributes[] = {
	{ "timeOrigin", environment_performance_time_origin, NULL },
	{ NULL, NULL, NULL }
};

/*
 * The operations of Performance: the clock, the entries (there are none
 * in this pass) and the methods that record them (which record nothing).
 * The table is constant for the life of the program.
 */
static const struct bind_operation environment_performance_operations[] = {
	{ "now", 0, environment_performance_now },
	{ "getEntries", 0, environment_performance_entries },
	{ "getEntriesByType", 1, environment_performance_entries },
	{ "getEntriesByName", 1, environment_performance_entries },
	{ "mark", 1, environment_performance_nothing },
	{ "measure", 1, environment_performance_nothing },
	{ "clearMarks", 0, environment_performance_nothing },
	{ "clearMeasures", 0, environment_performance_nothing },
	{ "clearResourceTimings", 0, environment_performance_nothing },
	{ "setResourceTimingBufferSize", 1, environment_performance_nothing },
	{ NULL, 0, NULL }
};

/*
 * The Performance interface.
 */
const struct bind_interface bind_performance_interface = {
	"Performance", BIND_NO_PARENT, 0, NULL, environment_performance_attributes, environment_performance_operations, NULL
};

/*
 * The operations of Location (the parts are accessors added by
 * environment_install_location).  The table is constant for the life of
 * the program.
 */
static const struct bind_operation environment_location_operations[] = {
	{ "toString", 0, environment_location_to_string },
	{ NULL, 0, NULL }
};

/*
 * The Location interface.
 */
const struct bind_interface bind_location_interface = {
	"Location", BIND_NO_PARENT, 0, NULL, NULL, environment_location_operations, NULL
};

/*
 * Makes the window's environment: navigator, screen, performance,
 * location, the Image constructor and the window's plain properties.
 */
int
bind_environment_install(
	struct bind_window *window)
{
	int error;

	/* The moment the window was made, which performance.timeOrigin reports. */
	window->time_origin = bind_epoch_milliseconds();

	/* navigator. */
	error = environment_install_navigator(window);
	if (error != 0)
		return error;

	/* screen. */
	error = environment_install_screen(window);
	if (error != 0)
		return error;

	/* performance. */
	error = environment_install_performance(window);
	if (error != 0)
		return error;

	/* location, which the document reports too. */
	error = environment_install_location(window);
	if (error != 0)
		return error;

	/* Image. */
	error = environment_install_image(window);
	if (error != 0)
		return error;

	/* XMLHttpRequest, used by page components that predate fetch. */
	error = environment_install_xhr(window);
	if (error != 0)
		return error;

	/* The observation APIs used to reveal and resize page components. */
	error = environment_install_observers(window);
	if (error != 0)
		return error;

	/* The window's own plain properties. */
	error = environment_install_window(window);
	if (error != 0)
		return error;

	/* Succeeded: the environment is on the global object. */
	return 0;
}

/* Makes the three observation constructors used by responsive pages. */
static int
environment_install_observers(
	struct bind_window *window)
{
	static const char *const names[] = { "MutationObserver", "IntersectionObserver", "ResizeObserver" };
	struct vm_realm *realm;
	struct vm_function *constructor;
	size_t index;
	int error;

	/* Each constructor records its observer kind in its native function. */
	realm = window->realm;
	for (index = 0; index < sizeof(names) / sizeof(names[0]); index++) {
		error = js_builtin_function(realm, names[index], 1, bind_illegal_constructor,
		    environment_observer_construct, &constructor);
		if (error != 0)
			return error;
		constructor->data = vm_value_int32((int32_t)index);
		error = js_builtin_value(realm, realm->global, names[index], vm_value_cell(constructor), JS_BUILTIN_METHOD);
		if (error != 0)
			return error;
	}

	/* Succeeded: every observer constructor is global. */
	return 0;
}

/*
 * Reads the real-time clock in milliseconds since 1970 (a whole number;
 * 0 when there is no clock).
 */
double
bind_epoch_milliseconds(void)
{
	struct timespec now;
	int status;

	/* The system's clock. */
	status = clock_gettime(CLOCK_REALTIME, &now);
	if (status != 0)
		return 0.0;

	/* Succeeded: the whole milliseconds. */
	return (double)now.tv_sec * 1000.0 + (double)(now.tv_nsec / 1000000L);
}

/*
 * Makes a string of one part of the document's location (a
 * BIND_LOCATION_*), as the host reads it from the page's URL; the empty
 * string when the host has no location.
 */
int
bind_location_part(
	struct bind_window *window,
	int part,
	vm_value *value)
{
	struct wb_buffer text;
	struct vm_string *string;
	int error;

	/* The host writes the part. */
	wb_buffer_init(&text);
	error = 0;
	if (window->host.location != NULL)
		error = window->host.location(window->host.context, part, &text);
	if (error != 0) {
		wb_buffer_release(&text);
		return error;
	}

	/* The string, from the UTF-8 text. */
	string = vm_string_from_utf8(window->realm->heap, wb_buffer_string(&text), text.length);
	wb_buffer_release(&text);
	if (string == NULL)
		return ENOMEM;

	/* Succeeded: the part's string. */
	*value = vm_value_cell(string);
	return 0;
}

/* Makes navigator with its names, flags and numbers. */
static int
environment_install_navigator(
	struct bind_window *window)
{
	struct vm_realm *realm;
	struct vm_object *prototype;
	struct vm_object *navigator;
	int error;

	/* The prototype carries what never changes. */
	realm = window->realm;
	prototype = window->prototypes[BIND_NAVIGATOR];
	error = environment_strings(realm, prototype, environment_navigator_strings);
	if (error != 0)
		return error;
	error = environment_flags(realm, prototype, environment_navigator_flags);
	if (error != 0)
		return error;
	error = environment_numbers(realm, prototype, environment_navigator_numbers);
	if (error != 0)
		return error;

	/* doNotTrack is null: the user has not said. */
	error = js_builtin_value(realm, prototype, "doNotTrack", VM_VALUE_NULL, VM_PROPERTY_ENUMERABLE | VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		return error;

	/* The window's navigator. */
	error = environment_instance(window, BIND_NAVIGATOR, "navigator", &navigator);
	if (error != 0)
		return error;

	/* Succeeded: navigator is on the global object. */
	return 0;
}

/* Makes screen with its sizes and depths. */
static int
environment_install_screen(
	struct bind_window *window)
{
	const struct environment_part *size;
	struct vm_realm *realm;
	struct vm_object *prototype;
	struct vm_object *screen;
	int error;

	/* The depths never change. */
	realm = window->realm;
	prototype = window->prototypes[BIND_SCREEN];
	error = environment_numbers(realm, prototype, environment_screen_numbers);
	if (error != 0)
		return error;

	/* The sizes follow the window's viewport, which is the whole screen the page can have. */
	for (size = environment_screen_sizes; size->name != NULL; size++) {
		error = environment_part_accessor(realm, prototype, size->name, environment_screen_size, size->part);
		if (error != 0)
			return error;
	}

	/* The window's screen. */
	error = environment_instance(window, BIND_SCREEN, "screen", &screen);
	if (error != 0)
		return error;

	/* Succeeded: screen is on the global object. */
	return 0;
}

/* Makes performance with its timing and navigation records. */
static int
environment_install_performance(
	struct bind_window *window)
{
	struct vm_realm *realm;
	struct vm_object *performance;
	struct vm_object *timing;
	struct vm_object *navigation;
	vm_value origin;
	size_t index;
	int error;

	/* The window's performance. */
	realm = window->realm;
	error = environment_instance(window, BIND_PERFORMANCE, "performance", &performance);
	if (error != 0)
		return error;

	/* timing: every moment is the time origin in this pass. */
	timing = vm_object_create(realm->heap, realm->object_prototype);
	if (timing == NULL)
		return ENOMEM;
	origin = vm_value_number(window->time_origin);
	for (index = 0; environment_timing_names[index] != NULL; index++) {
		error = js_builtin_value(realm, timing, environment_timing_names[index], origin, VM_PROPERTY_DEFAULT);
		if (error != 0)
			return error;
	}

	/* The record on performance. */
	error = js_builtin_value(realm, performance, "timing", vm_value_cell(timing), VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* navigation: an ordinary navigation (type 0) without redirects. */
	navigation = vm_object_create(realm->heap, realm->object_prototype);
	if (navigation == NULL)
		return ENOMEM;
	error = js_builtin_value(realm, navigation, "type", vm_value_int32(0), VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;
	error = js_builtin_value(realm, navigation, "redirectCount", vm_value_int32(0), VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;
	error = js_builtin_value(realm, performance, "navigation", vm_value_cell(navigation), VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* Succeeded: performance is on the global object. */
	return 0;
}

/* Makes location, whose parts are read from the host. */
static int
environment_install_location(
	struct bind_window *window)
{
	const struct environment_part *part;
	struct vm_realm *realm;
	struct vm_object *prototype;
	int error;

	/* Each part is an accessor that asks the host. */
	realm = window->realm;
	prototype = window->prototypes[BIND_LOCATION];
	for (part = environment_location_parts; part->name != NULL; part++) {
		error = environment_part_accessor(realm, prototype, part->name, environment_location_part, part->part);
		if (error != 0)
			return error;
	}

	/* The window's location, which document.location reports as well. */
	error = environment_instance(window, BIND_LOCATION, "location", &window->location);
	if (error != 0)
		return error;

	/* Succeeded: location is on the global object. */
	return 0;
}

/* Makes the Image constructor, whose objects are img elements. */
static int
environment_install_image(
	struct bind_window *window)
{
	struct vm_realm *realm;
	struct vm_function *image;
	vm_value prototype;
	int error;

	/* The constructor, which throws when called without new. */
	realm = window->realm;
	error = js_builtin_function(realm, "Image", 0, bind_illegal_constructor, environment_image_construct, &image);
	if (error != 0)
		return error;

	/* Its prototype is HTMLImageElement's (which keeps its own constructor). */
	prototype = vm_value_cell(window->prototypes[BIND_HTML_IMAGE_ELEMENT]);
	error = js_builtin_value(realm, &image->object, "prototype", prototype, 0);
	if (error != 0)
		return error;

	/* The global. */
	error = js_builtin_value(realm, realm->global, "Image", vm_value_cell(image), JS_BUILTIN_METHOD);
	if (error != 0)
		return error;

	/* Succeeded: Image is on the global object. */
	return 0;
}

/* Makes the XMLHttpRequest constructor and its small response interface. */
static int
environment_install_xhr(
	struct bind_window *window)
{
	static const char *const names[] = { "UNSENT", "OPENED", "HEADERS_RECEIVED", "LOADING", "DONE" };
	struct vm_realm *realm;
	struct vm_function *constructor;
	struct vm_object *prototype;
	vm_value value;
	size_t index;
	int status;

	/* The prototype holds the request methods. */
	realm = window->realm;
	prototype = vm_object_create(realm->heap, realm->object_prototype);
	if (prototype == NULL)
		return ENOMEM;
	status = js_builtin_method(realm, prototype, "open", 2, environment_xhr_open);
	if (status == 0)
		status = js_builtin_method(realm, prototype, "send", 1, environment_xhr_send);
	if (status == 0)
		status = js_builtin_method(realm, prototype, "abort", 0, environment_xhr_abort);
	if (status == 0)
		status = js_builtin_method(realm, prototype, "setRequestHeader", 2, environment_xhr_nothing);
	if (status == 0)
		status = js_builtin_method(realm, prototype, "overrideMimeType", 1, environment_xhr_nothing);
	if (status == 0)
		status = js_builtin_method(realm, prototype, "getResponseHeader", 1, environment_xhr_header);
	if (status == 0)
		status = js_builtin_method(realm, prototype, "getAllResponseHeaders", 0, environment_xhr_headers);
	if (status != 0)
		return status;

	/* The constructor keeps its prototype in native data. */
	status = js_builtin_function(realm, "XMLHttpRequest", 0, bind_illegal_constructor, environment_xhr_construct, &constructor);
	if (status != 0)
		return status;
	constructor->data = vm_value_cell(prototype);
	value = vm_value_cell(prototype);
	status = js_builtin_value(realm, &constructor->object, "prototype", value, 0);
	if (status == 0)
		status = js_builtin_value(realm, prototype, "constructor", vm_value_cell(constructor), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, realm->global, "XMLHttpRequest", vm_value_cell(constructor), JS_BUILTIN_METHOD);
	if (status != 0)
		return status;

	/* The ready-state constants are available on both the constructor and instances. */
	for (index = 0; index < 5U; index++) {
		status = js_builtin_value(realm, &constructor->object, names[index], vm_value_int32((int32_t)index), 0);
		if (status == 0)
			status = js_builtin_value(realm, prototype, names[index], vm_value_int32((int32_t)index), 0);
		if (status != 0)
			return status;
	}

	/* Succeeded: the constructor is installed. */
	return 0;
}

/* Makes a request object in the UNSENT state. */
static int
environment_xhr_construct(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	static const char *const null_properties[] = {
		"onreadystatechange", "onload", "onerror", "onloadend", "onabort", "ontimeout", "onprogress", NULL
	};
	struct vm_function *callee;
	struct vm_object *prototype;
	struct vm_object *request;
	struct vm_object *upload;
	vm_value empty;
	size_t index;
	int status;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The native constructor's data is XMLHttpRequest.prototype. */
	callee = js_builtin_callee(realm);
	prototype = (struct vm_object *)vm_value_as_cell(callee->data);
	request = vm_object_create(realm->heap, prototype);
	if (request == NULL)
		return ENOMEM;
	upload = vm_object_create(realm->heap, realm->object_prototype);
	if (upload == NULL)
		return ENOMEM;
	status = bind_string(realm, "", &empty);
	if (status != 0)
		return status;

	/* The response state and commonly tested request options. */
	status = js_builtin_value(realm, request, "readyState", vm_value_int32(0), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, request, "status", vm_value_int32(0), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, request, "statusText", empty, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, request, "responseText", empty, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, request, "response", empty, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, request, "responseURL", empty, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, request, "responseType", empty, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, request, "timeout", vm_value_int32(0), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, request, "withCredentials", VM_VALUE_FALSE, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, request, "upload", vm_value_cell(upload), VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;

	/* Event handler properties begin as null. */
	for (index = 0; null_properties[index] != NULL; index++) {
		status = js_builtin_value(realm, request, null_properties[index], VM_VALUE_NULL, VM_PROPERTY_DEFAULT);
		if (status != 0)
			return status;
	}

	/* Succeeded: the new request. */
	*result = vm_value_cell(request);
	return 0;
}

/* Records the method and URL, moving a request to OPENED. */
static int
environment_xhr_open(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_string *method;
	struct vm_string *url;
	int is_object;
	int status;

	/* The method belongs to a request object. */
	is_object = vm_value_is_object(this_value);
	if (!is_object)
		return bind_throw_illegal(realm);
	status = bind_to_string(realm, js_argument(args, count, 0), &method);
	if (status == 0)
		status = bind_to_string(realm, js_argument(args, count, 1), &url);
	if (status == 0)
		status = environment_xhr_put(realm, this_value, "__zedbsdMethod", vm_value_cell(method));
	if (status == 0)
		status = environment_xhr_put(realm, this_value, "__zedbsdURL", vm_value_cell(url));
	if (status == 0)
		status = environment_xhr_put(realm, this_value, "readyState", vm_value_int32(1));
	if (status == 0)
		status = environment_xhr_fire(realm, this_value, "readystatechange");
	if (status != 0)
		return status;
	*result = VM_VALUE_UNDEFINED;
	return 0;
}

/* Performs one request and dispatches its completion handlers. */
static int
environment_xhr_send(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct vm_string *url;
	struct vm_string *text;
	struct vm_string *location;
	struct wb_buffer href;
	struct wb_buffer bytes;
	struct wb_buffer final_url;
	vm_value value;
	int fetched;
	int is_object;
	int is_string;
	int status;
	const char *event_type;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	is_object = vm_value_is_object(this_value);
	if (!is_object)
		return bind_throw_illegal(realm);
	status = environment_xhr_get(realm, this_value, "__zedbsdURL", &value);
	if (status != 0)
		return status;
	is_string = vm_value_is_string(value);
	if (!is_string)
		return vm_throw_type_error(realm, "XMLHttpRequest.open must be called before send.");
	url = (struct vm_string *)vm_value_as_cell(value);

	/* The host fetcher resolves the URL against the page. */
	wb_buffer_init(&href);
	wb_buffer_init(&bytes);
	wb_buffer_init(&final_url);
	status = vm_string_to_utf8(url, &href);
	window = bind_window_of(realm);
	if (status == 0 && window->host.fetch_sync == NULL)
		status = ENOSYS;
	if (status == 0)
		status = window->host.fetch_sync(window->host.context, wb_buffer_string(&href), &bytes, &final_url);
	fetched = status == 0;

	/* A successful response exposes its body and final URL. */
	if (fetched) {
		text = vm_string_from_utf8(realm->heap, (const char *)bytes.data, bytes.length);
		location = vm_string_from_utf8(realm->heap, wb_buffer_string(&final_url), final_url.length);
		if (text == NULL || location == NULL)
			status = ENOMEM;
		if (status == 0)
			status = environment_xhr_put(realm, this_value, "status", vm_value_int32(200));
		if (status == 0)
			status = environment_xhr_put(realm, this_value, "responseText", vm_value_cell(text));
		if (status == 0)
			status = environment_xhr_put(realm, this_value, "response", vm_value_cell(text));
		if (status == 0)
			status = environment_xhr_put(realm, this_value, "responseURL", vm_value_cell(location));
	} else {
		/* Network failure is reported by the error event, not thrown from send. */
		status = environment_xhr_put(realm, this_value, "status", vm_value_int32(0));
	}

	/* Temporary network buffers are done. */
	wb_buffer_release(&href);
	wb_buffer_release(&bytes);
	wb_buffer_release(&final_url);
	if (status != 0)
		return status;

	/* DONE, then load or error, and loadend. */
	status = environment_xhr_put(realm, this_value, "readyState", vm_value_int32(4));
	if (status == 0)
		status = environment_xhr_fire(realm, this_value, "readystatechange");
	event_type = "error";
	if (fetched)
		event_type = "load";
	if (status == 0)
		status = environment_xhr_fire(realm, this_value, event_type);
	if (status == 0)
		status = environment_xhr_fire(realm, this_value, "loadend");
	if (status != 0)
		return status;
	*result = VM_VALUE_UNDEFINED;
	return 0;
}

/* Resets a request to UNSENT and reports abort. */
static int
environment_xhr_abort(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	status = environment_xhr_put(realm, this_value, "readyState", vm_value_int32(0));
	if (status == 0)
		status = environment_xhr_fire(realm, this_value, "abort");
	if (status != 0)
		return status;
	*result = VM_VALUE_UNDEFINED;
	return 0;
}

/* No response headers are retained in this first pass. */
static int
environment_xhr_header(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(realm);
	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	*result = VM_VALUE_NULL;
	return 0;
}

/* The empty response-header block. */
static int
environment_xhr_headers(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	return bind_string(realm, "", result);
}

/* Accepts setRequestHeader and overrideMimeType. */
static int
environment_xhr_nothing(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(realm);
	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	*result = VM_VALUE_UNDEFINED;
	return 0;
}

/* Writes one named request property. */
static int
environment_xhr_put(
	struct vm_realm *realm,
	vm_value object,
	const char *name,
	vm_value value)
{
	vm_value key;

	/* The atom is the property's key. */
	key = vm_key_from_ascii(realm->heap, name);
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	return vm_put(realm, object, key, value);
}

/* Reads one named request property. */
static int
environment_xhr_get(
	struct vm_realm *realm,
	vm_value object,
	const char *name,
	vm_value *value)
{
	vm_value key;

	/* The atom is the property's key. */
	key = vm_key_from_ascii(realm->heap, name);
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	return vm_get(realm, object, key, value);
}

/* Calls an XMLHttpRequest handler property when it is callable. */
static int
environment_xhr_fire(
	struct vm_realm *realm,
	vm_value object,
	const char *type)
{
	struct bind_window *window;
	struct vm_object *event;
	vm_value callback;
	vm_value event_value;
	vm_value ignored;
	int callable;
	int status;
	char name[40];

	/* Finds onTYPE. */
	snprintf(name, sizeof(name), "on%s", type);
	status = environment_xhr_get(realm, object, name, &callback);
	if (status != 0)
		return status;
	callable = vm_value_is_callable(callback);
	if (!callable)
		return 0;

	/* A small event gives handlers their type and target. */
	event = vm_object_create(realm->heap, realm->object_prototype);
	if (event == NULL)
		return ENOMEM;
	status = bind_string(realm, type, &event_value);
	if (status == 0)
		status = js_builtin_value(realm, event, "type", event_value, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, event, "target", object, VM_PROPERTY_DEFAULT);
	if (status == 0) {
		event_value = vm_value_cell(event);
		status = vm_call(realm, callback, object, &event_value, 1, &ignored);
	}

	/* Event-handler exceptions are reported without making send throw. */
	if (status == VM_THROWN) {
		window = bind_window_of(realm);
		bind_report_exception(window, realm->exception);
		realm->exception = VM_VALUE_UNDEFINED;
		status = 0;
	}

	/* Reports the call or allocation outcome. */
	return status;
}

/*
 * Defines the window's plain properties: the pixel ratio, the outer size
 * and position, and the window's relatives (a page without frames is its
 * own top and parent).
 */
static int
environment_install_window(
	struct bind_window *window)
{
	struct vm_realm *realm;
	struct vm_object *global;
	vm_value self;
	int error;

	/* One device pixel is one CSS pixel. */
	realm = window->realm;
	global = realm->global;
	error = js_builtin_value(realm, global, "devicePixelRatio", vm_value_int32(1), VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* Binary string conversion used by resource loaders. */
	error = js_builtin_method(realm, global, "atob", 1, environment_atob);
	if (error == 0)
		error = js_builtin_method(realm, global, "btoa", 1, environment_btoa);
	if (error == 0)
		error = js_builtin_method(realm, global, "fetch", 1, environment_fetch);
	if (error != 0)
		return error;

	/* The outer size is the viewport's (the window's frame is not counted). */
	error = environment_part_accessor(realm, global, "outerWidth", environment_outer_size, 0);
	if (error != 0)
		return error;
	error = environment_part_accessor(realm, global, "outerHeight", environment_outer_size, 1);
	if (error != 0)
		return error;

	/* The window's place on the screen is its corner. */
	error = js_builtin_value(realm, global, "screenX", vm_value_int32(0), VM_PROPERTY_DEFAULT);
	if (error == 0)
		error = js_builtin_value(realm, global, "screenY", vm_value_int32(0), VM_PROPERTY_DEFAULT);
	if (error == 0)
		error = js_builtin_value(realm, global, "screenLeft", vm_value_int32(0), VM_PROPERTY_DEFAULT);
	if (error == 0)
		error = js_builtin_value(realm, global, "screenTop", vm_value_int32(0), VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* A page without frames is its own top, parent and frames, and nothing opened it. */
	self = vm_value_cell(global);
	error = js_builtin_value(realm, global, "top", self, VM_PROPERTY_ENUMERABLE);
	if (error == 0)
		error = js_builtin_value(realm, global, "parent", self, VM_PROPERTY_DEFAULT);
	if (error == 0)
		error = js_builtin_value(realm, global, "frames", self, VM_PROPERTY_DEFAULT);
	if (error == 0)
		error = js_builtin_value(realm, global, "opener", VM_VALUE_NULL, VM_PROPERTY_DEFAULT);
	if (error == 0)
		error = js_builtin_value(realm, global, "length", vm_value_int32(0), VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* The window has no name and is open. */
	error = bind_string(realm, "", &self);
	if (error == 0)
		error = js_builtin_value(realm, global, "name", self, VM_PROPERTY_DEFAULT);
	if (error == 0)
		error = js_builtin_value(realm, global, "closed", VM_VALUE_FALSE, VM_PROPERTY_DEFAULT);
	if (error != 0)
		return error;

	/* Succeeded: the window's properties are defined. */
	return 0;
}

/*
 * Makes the window's object of an interface (from its prototype) and puts
 * it on the global object under a name that cannot be replaced.
 */
static int
environment_instance(
	struct bind_window *window,
	int interface,
	const char *global,
	struct vm_object **object)
{
	struct vm_realm *realm;
	int error;

	/* The object. */
	realm = window->realm;
	*object = vm_object_create(realm->heap, window->prototypes[interface]);
	if (*object == NULL)
		return ENOMEM;

	/* The global property, enumerable and fixed as the window's own objects are. */
	error = js_builtin_value(realm, realm->global, global, vm_value_cell(*object), VM_PROPERTY_ENUMERABLE | VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		return error;

	/* Succeeded: the object is on the global object. */
	return 0;
}

/* Defines each string of a table on an object. */
static int
environment_strings(
	struct vm_realm *realm,
	struct vm_object *object,
	const struct environment_string *table)
{
	vm_value value;
	int error;

	/* Each entry, a string that cannot be assigned. */
	for (; table->name != NULL; table++) {
		error = bind_string(realm, table->value, &value);
		if (error != 0)
			return error;
		error = js_builtin_value(realm, object, table->name, value, VM_PROPERTY_ENUMERABLE | VM_PROPERTY_CONFIGURABLE);
		if (error != 0)
			return error;
	}

	/* Succeeded: the strings are defined. */
	return 0;
}

/* Defines each number of a table on an object. */
static int
environment_numbers(
	struct vm_realm *realm,
	struct vm_object *object,
	const struct environment_number *table)
{
	int error;

	/* Each entry, a number that cannot be assigned. */
	for (; table->name != NULL; table++) {
		error = js_builtin_value(realm, object, table->name, vm_value_number(table->value), VM_PROPERTY_ENUMERABLE | VM_PROPERTY_CONFIGURABLE);
		if (error != 0)
			return error;
	}

	/* Succeeded: the numbers are defined. */
	return 0;
}

/* Defines each flag of a table on an object. */
static int
environment_flags(
	struct vm_realm *realm,
	struct vm_object *object,
	const struct environment_flag *table)
{
	vm_value value;
	int error;

	/* Each entry, a Boolean that cannot be assigned. */
	for (; table->name != NULL; table++) {
		value = VM_VALUE_FALSE;
		if (table->value)
			value = VM_VALUE_TRUE;
		error = js_builtin_value(realm, object, table->name, value, VM_PROPERTY_ENUMERABLE | VM_PROPERTY_CONFIGURABLE);
		if (error != 0)
			return error;
	}

	/* Succeeded: the flags are defined. */
	return 0;
}

/*
 * Defines a read-only accessor whose getter reads one part of something;
 * the getter finds the part in its function's data.
 */
static int
environment_part_accessor(
	struct vm_realm *realm,
	struct vm_object *object,
	const char *name,
	vm_native getter,
	int part)
{
	struct vm_function *function;
	struct vm_accessor *accessor;
	char getter_name[80];
	int error;

	/* The getter, named "get NAME", which knows its part. */
	snprintf(getter_name, sizeof(getter_name), "get %s", name);
	error = js_builtin_function(realm, getter_name, 0, getter, NULL, &function);
	if (error != 0)
		return error;
	function->data = vm_value_int32(part);

	/* The accessor, without a setter. */
	accessor = vm_accessor_create(realm->heap, vm_value_cell(function), VM_VALUE_UNDEFINED);
	if (accessor == NULL)
		return ENOMEM;
	error = js_builtin_value(realm, object, name, vm_value_cell(accessor), VM_PROPERTY_ACCESSOR | VM_PROPERTY_ENUMERABLE | VM_PROPERTY_CONFIGURABLE);
	if (error != 0)
		return error;

	/* Succeeded: the accessor is defined. */
	return 0;
}

/* Makes a new empty array (a list with nothing in it). */
static int
environment_empty_array(
	struct vm_realm *realm,
	vm_value *value)
{
	struct vm_object *array;
	int error;

	/* The array of the realm. */
	error = bind_array_create(realm, &array);
	if (error != 0)
		return error;

	/* Succeeded: the array. */
	*value = vm_value_cell(array);
	return 0;
}

/* Reports navigator.userAgent: the User-Agent the host sends. */
static int
environment_navigator_user_agent(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	const char *agent;
	int error;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* A host that names no agent leaves it empty. */
	window = bind_window_of(realm);
	agent = window->host.user_agent;
	if (agent == NULL)
		agent = "";

	/* The string. */
	error = bind_string(realm, agent, result);
	if (error != 0)
		return error;

	/* Succeeded: the agent is reported. */
	return 0;
}

/* Reports navigator.appVersion: the User-Agent after its first slash. */
static int
environment_navigator_app_version(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	const char *agent;
	const char *slash;
	int error;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The agent, from after the product's name. */
	window = bind_window_of(realm);
	agent = window->host.user_agent;
	if (agent == NULL)
		agent = "";
	slash = strchr(agent, '/');
	if (slash != NULL)
		agent = slash + 1;

	/* The string. */
	error = bind_string(realm, agent, result);
	if (error != 0)
		return error;

	/* Succeeded: the version is reported. */
	return 0;
}

/* Reports navigator.languages: the one language, in a new array. */
static int
environment_navigator_languages(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	vm_value language;
	int error;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The language navigator.language names. */
	error = bind_string(realm, "en-US", &language);
	if (error != 0)
		return error;

	/* The array of it. */
	error = js_builtin_array(realm, &language, 1, result);
	if (error != 0)
		return error;

	/* Succeeded: the languages are reported. */
	return 0;
}

/* Reports navigator.plugins and navigator.mimeTypes: there are none. */
static int
environment_navigator_list(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int error;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* An empty list. */
	error = environment_empty_array(realm, result);
	if (error != 0)
		return error;

	/* Succeeded: the list is reported. */
	return 0;
}

/* Reports navigator.javaEnabled(): there is no Java. */
static int
environment_navigator_java_enabled(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(realm);
	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Succeeded: false. */
	*result = VM_VALUE_FALSE;
	return 0;
}

/*
 * Takes navigator.sendBeacon(url, data): the beacon is accepted (true)
 * and not sent, since the page's measurements are for the site alone.
 */
static int
environment_navigator_send_beacon(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_string *url;
	int error;

	UNUSED_PARAMETER(this_value);

	/* The URL is converted as the method's argument is, which may throw. */
	error = bind_to_string(realm, js_argument(args, count, 0), &url);
	if (error != 0)
		return error;

	/* Succeeded: the beacon is taken. */
	*result = VM_VALUE_TRUE;
	return 0;
}

/* Reports one of the screen's sizes (the function's data: 0 for a width, 1 for a height). */
static int
environment_screen_size(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct vm_function *callee;
	int part;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Which size. */
	callee = js_builtin_callee(realm);
	part = vm_value_as_int32(callee->data);
	window = bind_window_of(realm);

	/* The height. */
	if (part == 1) {
		*result = vm_value_int32(window->viewport_height);
		return 0;
	}

	/* Succeeded: the width. */
	*result = vm_value_int32(window->viewport_width);
	return 0;
}

/* Reports performance.now(): the page's clock in milliseconds. */
static int
environment_performance_now(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The time the page gave the window. */
	window = bind_window_of(realm);
	*result = vm_value_number(window->now);

	/* Succeeded: the time is reported. */
	return 0;
}

/* Reports performance.timeOrigin: when the window was made, in milliseconds since 1970. */
static int
environment_performance_time_origin(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The moment kept when the window was made. */
	window = bind_window_of(realm);
	*result = vm_value_number(window->time_origin);

	/* Succeeded: the origin is reported. */
	return 0;
}

/* Reports the performance entries asked for: none are recorded in this pass. */
static int
environment_performance_entries(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int error;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* An empty list. */
	error = environment_empty_array(realm, result);
	if (error != 0)
		return error;

	/* Succeeded: the list is reported. */
	return 0;
}

/* Takes a performance method that records something: nothing is recorded in this pass. */
static int
environment_performance_nothing(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(realm);
	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Succeeded: undefined. */
	*result = VM_VALUE_UNDEFINED;
	return 0;
}

/* Reports one part of the location (the function's data is a BIND_LOCATION_*). */
static int
environment_location_part(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_function *callee;
	int part;
	int error;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Which part. */
	callee = js_builtin_callee(realm);
	part = vm_value_as_int32(callee->data);

	/* The part's text from the host. */
	error = bind_location_part(bind_window_of(realm), part, result);
	if (error != 0)
		return error;

	/* Succeeded: the part is reported. */
	return 0;
}

/* Reports location.toString(): the whole URL. */
static int
environment_location_to_string(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int error;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* The href. */
	error = bind_location_part(bind_window_of(realm), BIND_LOCATION_HREF, result);
	if (error != 0)
		return error;

	/* Succeeded: the URL is reported. */
	return 0;
}

/* Makes new Image(width, height): an img element with the sizes given as its attributes. */
static int
environment_image_construct(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct dom_element *element;
	struct vm_string *name;
	struct vm_string *value;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The element, in the HTML namespace of the window's document. */
	window = bind_window_of(realm);
	name = vm_atom_from_ascii(realm->heap, "img");
	if (name == NULL)
		return ENOMEM;
	element = dom_element_create(window->document, DOM_NS_HTML, name, NULL);
	if (element == NULL)
		return ENOMEM;

	/* The width, when given. */
	if (count > 0) {
		status = bind_to_string(realm, args[0], &value);
		if (status != 0)
			return status;
		name = vm_atom_from_ascii(realm->heap, "width");
		if (name == NULL)
			return ENOMEM;
		status = dom_element_set_attribute(element, name, value);
		if (status != 0)
			return status;
	}

	/* The height, when given. */
	if (count > 1) {
		status = bind_to_string(realm, args[1], &value);
		if (status != 0)
			return status;
		name = vm_atom_from_ascii(realm->heap, "height");
		if (name == NULL)
			return ENOMEM;
		status = dom_element_set_attribute(element, name, value);
		if (status != 0)
			return status;
	}

	/* Its object. */
	status = bind_wrap(window, &element->node, result);
	if (status != 0)
		return status;

	/* Succeeded: the image element is made. */
	return 0;
}

/* Decodes a Base64 string into a string whose code units are bytes (atob). */
static int
environment_atob(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_string *input;
	struct vm_string *decoded;
	struct wb_buffer clean;
	struct wb_buffer bytes;
	uint32_t bits;
	uint16_t unit;
	size_t index;
	int value;
	int held;
	int space;
	int status;

	UNUSED_PARAMETER(this_value);

	/* Converts the argument and removes the ASCII whitespace Base64 permits. */
	status = bind_to_string(realm, js_argument(args, count, 0), &input);
	if (status != 0)
		return status;
	wb_buffer_init(&clean);
	wb_buffer_init(&bytes);
	for (index = 0; index < input->length && status == 0; index++) {
		unit = vm_string_at(input, index);
		space = unit == 0x09U || unit == 0x0aU || unit == 0x0cU || unit == 0x0dU || unit == 0x20U;
		if (!space && unit <= 0x7fU)
			status = wb_buffer_append_byte(&clean, (unsigned char)unit);
		if (!space && unit > 0x7fU)
			status = EINVAL;
	}

	/* One or two padding characters at a quartet's end are optional. */
	if (status == 0 && clean.length % 4U == 0U && clean.length != 0 && clean.data[clean.length - 1U] == '=') {
		clean.length--;
		if (clean.length != 0 && clean.data[clean.length - 1U] == '=')
			clean.length--;
	}

	/* A lone character after whole groups cannot make a byte. */
	if (status == 0 && clean.length % 4U == 1U)
		status = EINVAL;

	/* Six bits per character make each output byte. */
	bits = 0;
	held = 0;
	for (index = 0; index < clean.length && status == 0; index++) {
		value = environment_base64_value(clean.data[index]);
		if (value < 0) {
			status = EINVAL;
			continue;
		}

		/* The six bits join those held from the preceding character. */
		bits = (bits << 6) | (uint32_t)value;
		held += 6;
		if (held >= 8) {
			held -= 8;
			status = wb_buffer_append_byte(&bytes, (unsigned char)(bits >> held));
		}
	}

	/* Invalid input throws the DOM exception browsers use. */
	if (status == EINVAL) {
		wb_buffer_release(&clean);
		wb_buffer_release(&bytes);
		return bind_throw_dom(realm, "InvalidCharacterError", "The string is not correctly encoded.");
	}

	/* Other failures are allocation failures. */
	if (status != 0) {
		wb_buffer_release(&clean);
		wb_buffer_release(&bytes);
		return status;
	}

	/* A binary string preserves every decoded byte. */
	decoded = vm_string_from_latin1(realm->heap, bytes.data, bytes.length);
	wb_buffer_release(&clean);
	wb_buffer_release(&bytes);
	if (decoded == NULL)
		return ENOMEM;
	*result = vm_value_cell(decoded);
	return 0;
}

/* Encodes a binary string as Base64 (btoa). */
static int
environment_btoa(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	struct vm_string *input;
	struct vm_string *encoded;
	struct wb_buffer text;
	uint32_t group;
	uint16_t units[3];
	uint16_t unit;
	unsigned char third;
	unsigned char fourth;
	size_t index;
	size_t left;
	size_t take;
	int status;

	UNUSED_PARAMETER(this_value);

	/* Every input code unit must fit one byte. */
	status = bind_to_string(realm, js_argument(args, count, 0), &input);
	if (status != 0)
		return status;
	for (index = 0; index < input->length; index++) {
		unit = vm_string_at(input, index);
		if (unit > 0xffU)
			return bind_throw_dom(realm, "InvalidCharacterError", "The string contains a character outside of the Latin1 range.");
	}

	/* Encodes three bytes into four alphabet characters. */
	wb_buffer_init(&text);
	status = 0;
	for (index = 0; index < input->length && status == 0; index += take) {
		left = input->length - index;
		take = 3U;
		if (left < take)
			take = left;
		units[0] = vm_string_at(input, index);
		units[1] = 0;
		units[2] = 0;
		if (take > 1U)
			units[1] = vm_string_at(input, index + 1U);
		if (take > 2U)
			units[2] = vm_string_at(input, index + 2U);
		group = ((uint32_t)units[0] << 16) | ((uint32_t)units[1] << 8) | units[2];
		third = '=';
		fourth = '=';
		if (take > 1U)
			third = (unsigned char)alphabet[(group >> 6) & 63U];
		if (take > 2U)
			fourth = (unsigned char)alphabet[group & 63U];
		status = wb_buffer_append_byte(&text, (unsigned char)alphabet[(group >> 18) & 63U]);
		if (status == 0)
			status = wb_buffer_append_byte(&text, (unsigned char)alphabet[(group >> 12) & 63U]);
		if (status == 0)
			status = wb_buffer_append_byte(&text, third);
		if (status == 0)
			status = wb_buffer_append_byte(&text, fourth);
	}

	/* An allocation failure stops the encoding. */
	if (status != 0) {
		wb_buffer_release(&text);
		return status;
	}

	/* The encoding is ASCII. */
	encoded = vm_string_from_latin1(realm->heap, text.data, text.length);
	wb_buffer_release(&text);
	if (encoded == NULL)
		return ENOMEM;
	*result = vm_value_cell(encoded);
	return 0;
}

/* Fetches one GET resource and reports a promise of its Response. */
static int
environment_fetch(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct bind_window *window;
	struct environment_fetch_pending *pending;
	struct vm_string *input;
	struct wb_buffer href;
	vm_value promise;
	int status;

	UNUSED_PARAMETER(this_value);

	/* Converts the request's URL to the host's UTF-8 form. */
	status = bind_to_string(realm, js_argument(args, count, 0), &input);
	if (status != 0)
		return status;
	wb_buffer_init(&href);
	status = vm_string_to_utf8(input, &href);

	/* A pending promise remains alive while the host owns its completion. */
	window = bind_window_of(realm);
	pending = NULL;
	if (status == 0) {
		pending = calloc(1, sizeof(*pending));
		if (pending == NULL)
			status = ENOMEM;
	}

	/* The Promise becomes the pending record's heap root. */
	if (status == 0)
		status = vm_promise_create(realm, NULL, &promise);
	if (status == 0) {
		pending->window = window;
		pending->promise = vm_value_as_cell(promise);
		status = vm_heap_add_root(realm->heap, &pending->promise);
	}

	/* The window owns the record before the host can complete synchronously. */
	if (status == 0) {
		pending->next = window->fetches;
		window->fetches = pending;
		*result = promise;
		if (window->host.fetch == NULL) {
			status = ENOSYS;
		} else {
			status = window->host.fetch(window->host.context, wb_buffer_string(&href),
			    environment_fetch_done, pending);
		}

		/* A request that could not start becomes a rejected Promise. */
		if (status != 0)
			environment_fetch_done(pending, status, 0, NULL, 0, NULL);
		status = 0;
	}

	/* The host copied the URL. */
	if (status != 0 && pending != NULL)
		free(pending);
	wb_buffer_release(&href);
	return status;
}

/* Settles one Fetch promise when the page's loader completes. */
static void
environment_fetch_done(
	void *context,
	int error,
	int response_status,
	const unsigned char *bytes,
	size_t length,
	const char *url)
{
	struct environment_fetch_pending *pending;
	struct environment_fetch_pending **link;
	struct vm_realm *realm;
	struct wb_buffer body;
	struct wb_buffer location;
	vm_value value;
	int status;

	/* The promise and temporary copies used to make the Response. */
	pending = context;
	realm = pending->window->realm;
	wb_buffer_init(&body);
	wb_buffer_init(&location);
	status = error;
	if (status == 0)
		status = wb_buffer_append(&body, bytes, length);
	if (status == 0 && url != NULL)
		status = wb_buffer_append_string(&location, url);
	if (status == 0)
		status = environment_response(realm, &body, &location, response_status, &value);
	if (status == 0)
		status = vm_promise_resolve(realm, vm_value_cell(pending->promise), value);
	if (status != 0 && status != ENOMEM) {
		status = vm_error_create(realm, VM_ERROR_TYPE, "Failed to fetch", &value);
		if (status == 0)
			status = vm_promise_reject(realm, vm_value_cell(pending->promise), value);
	}

	/* Unlink it after settlement; the host performs the task's microtask checkpoint. */
	link = &pending->window->fetches;
	while (*link != NULL && *link != pending)
		link = &(*link)->next;
	if (*link == pending)
		*link = pending->next;
	vm_heap_remove_root(realm->heap, &pending->promise);
	wb_buffer_release(&body);
	wb_buffer_release(&location);
	free(pending);
}

/* Drops unfinished Fetch roots after their host requests were cancelled. */
void
bind_environment_release(
	struct bind_window *window)
{
	struct environment_mutation_observer *observer_next;
	struct environment_mutation_observer *observer;
	struct environment_fetch_pending *next;
	struct environment_fetch_pending *pending;

	/* Each remaining host request has already been cancelled by the page. */
	pending = window->fetches;
	while (pending != NULL) {
		next = pending->next;
		vm_heap_remove_root(window->realm->heap, &pending->promise);
		free(pending);
		pending = next;
	}

	/* The list is empty. */
	window->fetches = NULL;

	/* Mutation observers retain their objects, callbacks, targets and pending records. */
	observer = window->mutation_observers;
	while (observer != NULL) {
		observer_next = observer->next;
		environment_mutation_clear(window, observer);
		vm_heap_remove_root(window->realm->heap, &observer->callback);
		vm_heap_remove_root(window->realm->heap, &observer->observer);
		free(observer);
		observer = observer_next;
	}

	/* The window no longer owns an observer. */
	window->mutation_observers = NULL;
}

/* Makes the small Response object used by fetch. */
static int
environment_response(
	struct vm_realm *realm,
	const struct wb_buffer *bytes,
	const struct wb_buffer *url,
	int response_status,
	vm_value *result)
{
	struct vm_function *method;
	struct vm_object *headers;
	struct vm_object *response;
	struct vm_string *body;
	struct vm_string *empty;
	struct vm_string *location;
	vm_value ok;
	int status;

	/* The body is decoded as UTF-8 for text() and json(). */
	body = vm_string_from_utf8(realm->heap, (const char *)bytes->data, bytes->length);
	if (body == NULL)
		return ENOMEM;
	location = vm_string_from_utf8(realm->heap, wb_buffer_string(url), url->length);
	if (location == NULL)
		return ENOMEM;
	empty = vm_string_from_latin1(realm->heap, (const unsigned char *)"", 0);
	if (empty == NULL)
		return ENOMEM;

	/* Headers is empty in this first pass, with a get() that reports null. */
	headers = vm_object_create(realm->heap, realm->object_prototype);
	if (headers == NULL)
		return ENOMEM;
	status = js_builtin_method(realm, headers, "get", 1, environment_headers_get);
	if (status != 0)
		return status;

	/* The Response's status and URL. */
	response = vm_object_create(realm->heap, realm->object_prototype);
	if (response == NULL)
		return ENOMEM;
	ok = VM_VALUE_FALSE;
	if (response_status >= 200 && response_status <= 299)
		ok = VM_VALUE_TRUE;
	status = js_builtin_value(realm, response, "ok", ok, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, response, "status", vm_value_int32(response_status), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, response, "statusText", vm_value_cell(empty), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, response, "url", vm_value_cell(location), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, response, "redirected", VM_VALUE_FALSE, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, response, "headers", vm_value_cell(headers), VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;

	/* text() and json() retain the body string in their function data. */
	status = js_builtin_function(realm, "text", 0, environment_response_text, NULL, &method);
	if (status == 0) {
		method->data = vm_value_cell(body);
		status = js_builtin_value(realm, response, "text", vm_value_cell(method), JS_BUILTIN_METHOD);
	}

	/* json() uses the same retained string. */
	if (status == 0) {
		status = js_builtin_function(realm, "json", 0, environment_response_json, NULL, &method);
		if (status == 0) {
			method->data = vm_value_cell(body);
			status = js_builtin_value(realm, response, "json", vm_value_cell(method), JS_BUILTIN_METHOD);
		}
	}

	/* A failed method definition stops the response. */
	if (status != 0)
		return status;

	/* Succeeded: the response object owns everything through its properties and methods. */
	*result = vm_value_cell(response);
	return 0;
}

/* Returns a fulfilled promise of a Response's body string. */
static int
environment_response_text(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_function *callee;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	callee = js_builtin_callee(realm);
	return environment_settle_promise(realm, callee->data, 0, result);
}

/* Parses a Response's body and returns a promise of the JSON value. */
static int
environment_response_json(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_function *callee;
	vm_value json;
	vm_value key;
	vm_value parse;
	vm_value value;
	int rejects;
	int status;

	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Calls the realm's JSON.parse with the retained body. */
	callee = js_builtin_callee(realm);
	key = vm_key_from_ascii(realm->heap, "JSON");
	if (key == VM_VALUE_EMPTY)
		return ENOMEM;
	status = vm_get(realm, vm_value_cell(realm->global), key, &json);
	if (status == 0) {
		key = vm_key_from_ascii(realm->heap, "parse");
		if (key == VM_VALUE_EMPTY)
			return ENOMEM;
		status = vm_get(realm, json, key, &parse);
	}

	/* Parses, turning a thrown SyntaxError into the promise's rejection. */
	if (status == 0)
		status = vm_call(realm, parse, json, &callee->data, 1, &value);
	rejects = 0;
	if (status == VM_THROWN) {
		value = realm->exception;
		realm->exception = VM_VALUE_UNDEFINED;
		rejects = 1;
		status = 0;
	}

	/* Other engine failures stop the call. */
	if (status != 0)
		return status;

	/* The promise follows the parse outcome. */
	return environment_settle_promise(realm, value, rejects, result);
}

/* An empty Headers object has no value for a name. */
static int
environment_headers_get(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	UNUSED_PARAMETER(realm);
	UNUSED_PARAMETER(this_value);
	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	*result = VM_VALUE_NULL;
	return 0;
}

/* Makes and fulfills or rejects a native Promise. */
static int
environment_settle_promise(
	struct vm_realm *realm,
	vm_value value,
	int reject,
	vm_value *result)
{
	vm_value promise;
	int status;

	/* The promise uses the realm's Promise prototype. */
	status = vm_promise_create(realm, NULL, &promise);
	if (status != 0)
		return status;
	if (reject) {
		status = vm_promise_reject(realm, promise, value);
	} else {
		status = vm_promise_resolve(realm, promise, value);
	}

	/* Succeeded: the settled promise is returned. */
	if (status != 0)
		return status;
	*result = promise;
	return 0;
}

/* Makes an observer whose callback is kept by its observe method. */
static int
environment_observer_construct(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct vm_function *callee;
	struct vm_function *method;
	struct vm_object *observer;
	vm_value data_values[2];
	vm_value data;
	vm_value callback;
	int callable;
	int type;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The first argument is the callable notified by observe. */
	callback = js_argument(args, count, 0);
	callable = vm_value_is_callable(callback);
	if (!callable)
		return vm_throw_type_error(realm, "The observer callback is not a function.");
	callee = js_builtin_callee(realm);
	type = vm_value_as_int32(callee->data);
	data_values[0] = callback;
	data_values[1] = vm_value_int32(type);
	status = js_builtin_array(realm, data_values, 2, &data);
	if (status != 0)
		return status;

	/* The observer's methods; observe keeps the callback and kind in its function data. */
	observer = vm_object_create(realm->heap, realm->object_prototype);
	if (observer == NULL)
		return ENOMEM;
	status = js_builtin_function(realm, "observe", 1, environment_observer_observe, NULL, &method);
	if (status == 0) {
		method->data = data;
		status = js_builtin_value(realm, observer, "observe", vm_value_cell(method), JS_BUILTIN_METHOD);
	}

	/* The other methods do not need per-observer state. */
	if (status == 0)
		status = js_builtin_method(realm, observer, "unobserve", 1, environment_observer_disconnect);
	if (status == 0)
		status = js_builtin_method(realm, observer, "disconnect", 0, environment_observer_disconnect);
	if (status == 0)
		status = js_builtin_method(realm, observer, "takeRecords", 0, environment_observer_records);
	if (status == 0 && type == 0)
		status = environment_mutation_add(bind_window_of(realm), observer, callback);
	if (status != 0)
		return status;

	/* Succeeded: new reports the observer object. */
	*result = vm_value_cell(observer);
	return 0;
}

/* Begins observing one target and queues its initial layout observation. */
static int
environment_observer_observe(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct environment_mutation_observer *observer;
	struct bind_window *window;
	struct dom_node *node;
	struct vm_function *callee;
	struct vm_object *data;
	vm_value entry;
	vm_value entries;
	vm_value callback;
	vm_value options;
	vm_value target;
	int child_list;
	int subtree;
	int type;
	int status;

	UNUSED_PARAMETER(this_value);
	*result = VM_VALUE_UNDEFINED;

	/* Mutation delivery waits for the DOM mutation queue; layout observers report their initial state. */
	callee = js_builtin_callee(realm);
	data = (struct vm_object *)vm_value_as_cell(callee->data);
	callback = data->elements[0];
	type = vm_value_as_int32(data->elements[1]);
	if (type == 0) {
		/* Mutation observers retain a Node and the requested child-list scope. */
		child_list = 0;
		subtree = 0;
		target = js_argument(args, count, 0);
		node = bind_node_of(target);
		if (node == NULL)
			return vm_throw_type_error(realm, "The observation target is not a Node.");
		options = js_argument(args, count, 1);
		status = environment_mutation_options(realm, options, &child_list, &subtree);
		if (status != 0)
			return status;
		observer = environment_mutation_find(bind_window_of(realm), this_value);
		if (observer == NULL)
			return bind_throw_illegal(realm);

		/* A second observe call replaces this minimal observer's prior target. */
		environment_mutation_clear(bind_window_of(realm), observer);
		observer->target = vm_value_as_cell(target);
		status = vm_heap_add_root(realm->heap, &observer->target);
		if (status != 0) {
			observer->target = NULL;
			return status;
		}

		/* The active registration uses the requested scope. */
		observer->child_list = child_list;
		observer->subtree = subtree;
		return 0;
	}

	/* A layout observation is delivered as one microtask record. */
	target = js_argument(args, count, 0);
	window = bind_window_of(realm);
	status = environment_observer_entry(window, target, type, &entry);
	if (status == 0)
		status = js_builtin_array(realm, &entry, 1, &entries);
	if (status == 0)
		status = vm_enqueue_job(realm, callback, entries);
	return status;
}

/* Stops a MutationObserver; the layout observers have no retained target. */
static int
environment_observer_disconnect(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct environment_mutation_observer *observer;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	*result = VM_VALUE_UNDEFINED;
	observer = environment_mutation_find(bind_window_of(realm), this_value);
	if (observer != NULL)
		environment_mutation_clear(bind_window_of(realm), observer);
	return 0;
}

/* Takes a MutationObserver's pending records, or an empty list. */
static int
environment_observer_records(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct environment_mutation_observer *observer;
	struct vm_cell *records;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);
	observer = environment_mutation_find(bind_window_of(realm), this_value);
	if (observer != NULL && observer->records != NULL) {
		records = observer->records;
		vm_heap_remove_root(realm->heap, &observer->records);
		observer->records = NULL;
		*result = vm_value_cell(records);
		return 0;
	}

	/* An observer without pending records returns an empty sequence. */
	return js_builtin_array(realm, NULL, 0, result);
}

/* Retains one newly constructed MutationObserver and its callback. */
static int
environment_mutation_add(
	struct bind_window *window,
	struct vm_object *observer,
	vm_value callback)
{
	struct environment_mutation_observer *made;
	struct environment_mutation_observer **link;
	int status;

	/* The registered observer remains alive even when the script drops its last reference. */
	made = calloc(1, sizeof(*made));
	if (made == NULL)
		return ENOMEM;
	made->observer = &observer->cell;
	made->callback = vm_value_as_cell(callback);
	status = vm_heap_add_root(window->realm->heap, &made->observer);
	if (status == 0)
		status = vm_heap_add_root(window->realm->heap, &made->callback);
	if (status != 0) {
		if (made->observer != NULL)
			vm_heap_remove_root(window->realm->heap, &made->observer);
		free(made);
		return status;
	}

	/* Registration order is callback delivery order. */
	link = &window->mutation_observers;
	while (*link != NULL)
		link = &(*link)->next;
	*link = made;
	return 0;
}

/* Finds the state belonging to a MutationObserver object. */
static struct environment_mutation_observer *
environment_mutation_find(
	struct bind_window *window,
	vm_value observer)
{
	struct environment_mutation_observer *entry;
	struct vm_cell *cell;
	int is_object;

	/* Only the exact object made by this constructor has state. */
	is_object = vm_value_is_object(observer);
	if (!is_object)
		return NULL;
	cell = vm_value_as_cell(observer);
	for (entry = window->mutation_observers; entry != NULL; entry = entry->next) {
		if (entry->observer == cell)
			return entry;
	}

	/* No registered observer owns this object. */
	return NULL;
}

/* Drops a MutationObserver's target and any records not delivered yet. */
static void
environment_mutation_clear(
	struct bind_window *window,
	struct environment_mutation_observer *observer)
{
	if (observer->target != NULL) {
		vm_heap_remove_root(window->realm->heap, &observer->target);
		observer->target = NULL;
	}

	/* Pending records are discarded by disconnect and a repeated observe. */
	if (observer->records != NULL) {
		vm_heap_remove_root(window->realm->heap, &observer->records);
		observer->records = NULL;
	}

	/* A later observe call supplies the options again. */
	observer->child_list = 0;
	observer->subtree = 0;
}

/* Reads the MutationObserver options implemented by this pass. */
static int
environment_mutation_options(
	struct vm_realm *realm,
	vm_value options,
	int *child_list,
	int *subtree)
{
	vm_value value;
	int attributes = 0;
	int character_data = 0;
	int is_object;
	int present;
	int status;

	/* An options dictionary must enable at least one mutation kind. */
	is_object = vm_value_is_object(options);
	if (!is_object)
		return vm_throw_type_error(realm, "MutationObserver options are required.");
	status = bind_get_option(realm, options, "childList", &present, &value);
	if (status != 0)
		return status;
	*child_list = present && vm_to_boolean(value);
	status = bind_get_option(realm, options, "subtree", &present, &value);
	if (status != 0)
		return status;
	*subtree = present && vm_to_boolean(value);
	status = bind_get_option(realm, options, "attributes", &present, &value);
	if (status != 0)
		return status;
	attributes = present && vm_to_boolean(value);
	status = bind_get_option(realm, options, "characterData", &present, &value);
	if (status != 0)
		return status;
	character_data = present && vm_to_boolean(value);
	if (!*child_list && !attributes && !character_data)
		return vm_throw_type_error(realm, "MutationObserver has no enabled mutation type.");
	return 0;
}

/* Records one child-list change for every observer whose target contains the parent. */
int
bind_environment_child_mutation(
	struct bind_window *window,
	struct dom_node *parent,
	struct dom_node *added,
	struct dom_node *removed)
{
	struct environment_mutation_observer *observer;
	struct dom_node *target;
	int matches;
	int status;

	/* Each observer gets a separate record for the parent in its scope. */
	for (observer = window->mutation_observers; observer != NULL; observer = observer->next) {
		if (!observer->child_list || observer->target == NULL)
			continue;
		target = bind_node_of(vm_value_cell(observer->target));
		matches = target == parent;
		if (!matches && observer->subtree)
			matches = dom_is_inclusive_ancestor(target, parent);
		if (!matches)
			continue;
		status = environment_mutation_record(window, observer, parent, added, removed);
		if (status != 0)
			return status;
	}

	/* Every matching observer now owns its record. */
	return 0;
}

/* Queues callbacks for MutationObservers that have accumulated records. */
int
bind_environment_checkpoint(
	struct bind_window *window,
	int *queued)
{
	struct environment_mutation_observer *observer;
	struct vm_cell *records;
	int status;

	/* Each pending array becomes one callback job. */
	*queued = 0;
	for (observer = window->mutation_observers; observer != NULL; observer = observer->next) {
		if (observer->records == NULL)
			continue;
		records = observer->records;
		vm_heap_remove_root(window->realm->heap, &observer->records);
		observer->records = NULL;
		status = vm_enqueue_job(window->realm, vm_value_cell(observer->callback), vm_value_cell(records));
		if (status != 0)
			return status;
		*queued = 1;
	}

	/* All pending arrays are now jobs. */
	return 0;
}

/* Appends one childList MutationRecord to an observer's pending array. */
static int
environment_mutation_record(
	struct bind_window *window,
	struct environment_mutation_observer *observer,
	struct dom_node *parent,
	struct dom_node *added,
	struct dom_node *removed)
{
	struct vm_realm *realm;
	struct vm_object *array;
	struct vm_object *record;
	vm_value added_nodes;
	vm_value removed_nodes;
	vm_value target;
	vm_value type;
	vm_value records;
	int status;

	/* The first change starts the rooted array for this checkpoint. */
	realm = window->realm;
	if (observer->records == NULL) {
		status = js_builtin_array(realm, NULL, 0, &records);
		if (status != 0)
			return status;
		observer->records = vm_value_as_cell(records);
		status = vm_heap_add_root(realm->heap, &observer->records);
		if (status != 0) {
			observer->records = NULL;
			return status;
		}
	}

	/* The record has the changed parent and NodeList-shaped added and removed sequences. */
	status = bind_wrap(window, parent, &target);
	if (status == 0)
		status = environment_node_list(window, added, &added_nodes);
	if (status == 0)
		status = environment_node_list(window, removed, &removed_nodes);
	if (status == 0)
		status = bind_string(realm, "childList", &type);
	if (status != 0)
		return status;
	record = vm_object_create(realm->heap, realm->object_prototype);
	if (record == NULL)
		return ENOMEM;
	status = js_builtin_value(realm, record, "type", type, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, record, "target", target, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, record, "addedNodes", added_nodes, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, record, "removedNodes", removed_nodes, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, record, "previousSibling", VM_VALUE_NULL, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, record, "nextSibling", VM_VALUE_NULL, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, record, "attributeName", VM_VALUE_NULL, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, record, "attributeNamespace", VM_VALUE_NULL, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, record, "oldValue", VM_VALUE_NULL, VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;

	/* The pending record is kept by the rooted array. */
	array = (struct vm_object *)observer->records;
	return vm_object_define(realm->heap, array, vm_value_int32((int32_t)array->length),
	    vm_value_cell(record), VM_PROPERTY_DEFAULT);
}

/* Makes a zero- or one-item array whose constructor name is NodeList. */
static int
environment_node_list(
	struct bind_window *window,
	struct dom_node *node,
	vm_value *result)
{
	struct vm_realm *realm;
	struct vm_object *constructor;
	struct vm_object *list;
	vm_value item;
	vm_value name;
	int status;

	/* A MutationRecord uses a NodeList even when it contains no node. */
	realm = window->realm;
	if (node == NULL) {
		status = js_builtin_array(realm, NULL, 0, result);
	} else {
		status = bind_wrap(window, node, &item);
		if (status == 0)
			status = js_builtin_array(realm, &item, 1, result);
	}

	/* An allocation failure leaves no usable list. */
	if (status != 0)
		return status;
	list = (struct vm_object *)vm_value_as_cell(*result);
	constructor = vm_object_create(realm->heap, realm->object_prototype);
	if (constructor == NULL)
		return ENOMEM;
	status = bind_string(realm, "NodeList", &name);
	if (status == 0)
		status = js_builtin_value(realm, constructor, "name", name, VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, list, "constructor", vm_value_cell(constructor), VM_PROPERTY_DEFAULT);
	return status;
}

/* Makes the initial IntersectionObserver or ResizeObserver entry for a target. */
static int
environment_observer_entry(
	struct bind_window *window,
	vm_value target,
	int type,
	vm_value *entry)
{
	struct vm_realm *realm;
	struct vm_object *record;
	struct dom_node *node;
	struct bind_box box;
	vm_value rect;
	vm_value root;
	vm_value sizes;
	double scroll_x;
	double scroll_y;
	double x;
	double y;
	int visible;
	int found;
	int status;

	/* Observers only accept elements. */
	realm = window->realm;
	node = bind_node_of(target);
	if (node == NULL || node->type != DOM_ELEMENT)
		return vm_throw_type_error(realm, "The observation target is not an Element.");

	/* Its border box in viewport coordinates. */
	memset(&box, 0, sizeof(box));
	found = 0;
	if (window->host.node_box != NULL)
		found = window->host.node_box(window->host.context, node, &box);
	scroll_x = 0.0;
	scroll_y = 0.0;
	if (window->host.scroll != NULL)
		window->host.scroll(window->host.context, &scroll_x, &scroll_y);
	x = box.x - scroll_x;
	y = box.y - scroll_y;
	status = environment_observer_rect(realm, x, y, box.width, box.height, &rect);
	if (status != 0)
		return status;

	/* Every record names the observed element. */
	record = vm_object_create(realm->heap, realm->object_prototype);
	if (record == NULL)
		return ENOMEM;
	status = js_builtin_value(realm, record, "target", target, VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;

	/* Intersection records include the viewport root and the visible state. */
	if (type == 1) {
		visible = found && box.width > 0.0 && box.height > 0.0 && x < window->viewport_width && y < window->viewport_height &&
		    x + box.width > 0.0 && y + box.height > 0.0;
		status = environment_observer_rect(realm, 0.0, 0.0, window->viewport_width, window->viewport_height, &root);
		if (status == 0)
			status = js_builtin_value(realm, record, "time", vm_value_number(window->now), VM_PROPERTY_DEFAULT);
		if (status == 0)
			status = js_builtin_value(realm, record, "rootBounds", root, VM_PROPERTY_DEFAULT);
		if (status == 0)
			status = js_builtin_value(realm, record, "boundingClientRect", rect, VM_PROPERTY_DEFAULT);
		if (status == 0)
			status = js_builtin_value(realm, record, "intersectionRect", rect, VM_PROPERTY_DEFAULT);
		if (status == 0)
			status = js_builtin_value(realm, record, "isIntersecting", vm_value_boolean(visible), VM_PROPERTY_DEFAULT);
		if (status == 0)
			status = js_builtin_value(realm, record, "intersectionRatio", vm_value_int32(visible), VM_PROPERTY_DEFAULT);
	}

	/* Resize records include the content rectangle and all three size arrays. */
	if (type == 2) {
		status = js_builtin_value(realm, record, "contentRect", rect, VM_PROPERTY_DEFAULT);
		if (status == 0)
			status = environment_observer_size(realm, box.width, box.height, &sizes);
		if (status == 0)
			status = js_builtin_value(realm, record, "contentBoxSize", sizes, VM_PROPERTY_DEFAULT);
		if (status == 0)
			status = js_builtin_value(realm, record, "borderBoxSize", sizes, VM_PROPERTY_DEFAULT);
		if (status == 0)
			status = js_builtin_value(realm, record, "devicePixelContentBoxSize", sizes, VM_PROPERTY_DEFAULT);
	}

	/* An allocation or property failure stops delivery. */
	if (status != 0)
		return status;

	/* Succeeded: the microtask owns the record through its array. */
	*entry = vm_value_cell(record);
	return 0;
}

/* Makes a DOMRect-shaped plain object. */
static int
environment_observer_rect(
	struct vm_realm *realm,
	double x,
	double y,
	double width,
	double height,
	vm_value *result)
{
	struct vm_object *rect;
	int status;

	/* Each DOMRect field is an ordinary number. */
	rect = vm_object_create(realm->heap, realm->object_prototype);
	if (rect == NULL)
		return ENOMEM;
	status = js_builtin_value(realm, rect, "x", vm_value_number(x), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, rect, "y", vm_value_number(y), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, rect, "width", vm_value_number(width), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, rect, "height", vm_value_number(height), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, rect, "top", vm_value_number(y), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, rect, "right", vm_value_number(x + width), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, rect, "bottom", vm_value_number(y + height), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, rect, "left", vm_value_number(x), VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;
	*result = vm_value_cell(rect);
	return 0;
}

/* Makes the one-item ResizeObserverSize sequence. */
static int
environment_observer_size(
	struct vm_realm *realm,
	double width,
	double height,
	vm_value *result)
{
	struct vm_object *size;
	vm_value item;
	int status;

	/* One size record is enough for a single CSS box. */
	size = vm_object_create(realm->heap, realm->object_prototype);
	if (size == NULL)
		return ENOMEM;
	status = js_builtin_value(realm, size, "inlineSize", vm_value_number(width), VM_PROPERTY_DEFAULT);
	if (status == 0)
		status = js_builtin_value(realm, size, "blockSize", vm_value_number(height), VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;
	item = vm_value_cell(size);
	return js_builtin_array(realm, &item, 1, result);
}

/* Reports a Base64 alphabet character's six-bit value, or -1. */
static int
environment_base64_value(
	uint16_t unit)
{
	if (unit >= 'A' && unit <= 'Z')
		return unit - 'A';
	if (unit >= 'a' && unit <= 'z')
		return unit - 'a' + 26;
	if (unit >= '0' && unit <= '9')
		return unit - '0' + 52;
	if (unit == '+')
		return 62;
	if (unit == '/')
		return 63;
	return -1;
}

/* Reports outerWidth or outerHeight (the function's data: 0 for the width, 1 for the height). */
static int
environment_outer_size(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	int error;

	/* The window's outer size is the screen's in this pass. */
	error = environment_screen_size(realm, this_value, args, count, result);
	if (error != 0)
		return error;

	/* Succeeded: the size is reported. */
	return 0;
}
