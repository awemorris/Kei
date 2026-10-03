/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * ws118-p005: host test of the boot parameters i915.start= and i915.debug=.
 *
 * Links src/kern/boot.c and checks that each parameter takes its two words,
 * refuses another word, an empty value and a repeat, is absent when not
 * given, and leaves the other parameters as they were.  Built and run by
 * host-boot-i915-test.sh.
 */

#include <kern/boot.h>

#include <uapi/errno.h>

#include <stdio.h>
#include <string.h>

/* The size of the line buffer a test parses. */
#define TEST_LINE_SIZE	256

/*
 * The checks run and the checks failed.
 *
 * Every check counts itself; the tally is printed at the end and decides the
 * exit status.
 */
static unsigned checks;
static unsigned failures;

static void check(int ok, const char *what);
static int parse(struct kern_boot_parameters *parameters, const char *text);
static int value_is(const struct kern_boot_parameters *parameters, enum kern_boot_parameter_key key, const char *word);
static void test_words(void);
static void test_refusals(void);

/*
 * Runs the checks and reports the tally.
 */
int
main(void)
{
	/* Runs the accepted words, then the refused ones. */
	test_words();
	test_refusals();

	printf("host-boot-i915-test: %u checks, %u failures\n", checks, failures);

	/* Reports a failed check. */
	if (failures != 0U)
		return 1;

	/* Succeeded: every check passed. */
	return 0;
}

/* Counts one check and names it when it failed. */
static void
check(
	int ok,
	const char *what)
{
	/* Every check counts. */
	checks++;

	/* A failed check is named. */
	if (!ok) {
		failures++;
		printf("FAIL: %s\n", what);
	}
}

/* Parses a copy of the text and reports the parser's answer. */
static int
parse(
	struct kern_boot_parameters *parameters,
	const char *text)
{
	char line[TEST_LINE_SIZE];
	int error;

	/* Copies the text, which the parser terminates in place. */
	memset(line, 0, sizeof(line));
	strncpy(line, text, sizeof(line) - 1U);

	/* Parses the copy. */
	error = kern_boot_parameters_parse(parameters, line, sizeof(line));
	if (error != 0)
		return error;

	/* Succeeded: the record holds the line. */
	return 0;
}

/* Reports whether a parameter was given as the word. */
static int
value_is(
	const struct kern_boot_parameters *parameters,
	enum kern_boot_parameter_key key,
	const char *word)
{
	const char *value;
	int different;

	/* A parameter that was not given is not the word. */
	value = kern_boot_parameters_value(parameters, key);
	if (value == NULL)
		return 0;

	/* Compares the given word. */
	different = strcmp(value, word);
	if (different != 0)
		return 0;

	/* Succeeded: the parameter is the word. */
	return 1;
}

/* Checks the words each parameter takes and the absent case. */
static void
test_words(void)
{
	struct kern_boot_parameters parameters;
	const char *value;
	int error;
	int same;

	/* The variant D boot line: every word is kept. */
	error = parse(&parameters, "rootpart=PARTLABEL=zedBSD-root display=edp login=graphical i915.start=manual i915.debug=display");
	check(error == 0, "parse the variant D line");
	same = value_is(&parameters, KERN_BOOT_PARAMETER_I915_START, "manual");
	check(same, "i915.start is manual");
	same = value_is(&parameters, KERN_BOOT_PARAMETER_I915_DEBUG, "display");
	check(same, "i915.debug is display");
	same = value_is(&parameters, KERN_BOOT_PARAMETER_DISPLAY, "edp");
	check(same, "display stays edp");
	same = value_is(&parameters, KERN_BOOT_PARAMETER_LOGIN, "graphical");
	check(same, "login stays graphical");
	check(kern_boot_parameters_unknown_count(&parameters) == 0U, "no unknown parameter in the variant D line");

	/* The default words. */
	error = parse(&parameters, "i915.start=auto i915.debug=off");
	check(error == 0, "parse i915.start=auto i915.debug=off");
	same = value_is(&parameters, KERN_BOOT_PARAMETER_I915_START, "auto");
	check(same, "i915.start is auto");
	same = value_is(&parameters, KERN_BOOT_PARAMETER_I915_DEBUG, "off");
	check(same, "i915.debug is off");

	/* A line without either parameter leaves both absent. */
	error = parse(&parameters, "kmsg=quiet login=graphical display=edp");
	check(error == 0, "parse a line without the i915 parameters");
	value = kern_boot_parameters_value(&parameters, KERN_BOOT_PARAMETER_I915_START);
	check(value == NULL, "no i915.start= is absent");
	value = kern_boot_parameters_value(&parameters, KERN_BOOT_PARAMETER_I915_DEBUG);
	check(value == NULL, "no i915.debug= is absent");
}

/* Checks that another word, an empty value and a repeat are refused. */
static void
test_refusals(void)
{
	struct kern_boot_parameters parameters;
	int error;

	/* Another word. */
	error = parse(&parameters, "i915.start=later");
	check(error == EINVAL, "i915.start=later is refused");
	error = parse(&parameters, "i915.debug=all");
	check(error == EINVAL, "i915.debug=all is refused");
	error = parse(&parameters, "i915.start=Manual");
	check(error == EINVAL, "i915.start=Manual is refused (words are exact)");
	error = parse(&parameters, "i915.debug=displays");
	check(error == EINVAL, "i915.debug=displays is refused (no prefix match)");

	/* An empty value. */
	error = parse(&parameters, "i915.start=");
	check(error == EINVAL, "an empty i915.start= is refused");

	/* A repeat. */
	error = parse(&parameters, "i915.start=manual i915.start=manual");
	check(error == EEXIST, "a repeated i915.start= is refused");
	error = parse(&parameters, "i915.debug=display i915.debug=off");
	check(error == EEXIST, "a repeated i915.debug= is refused");
}
