/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * ws134-p002: host tests of the System Monitor's parts without a window
 * (userland/desktop/monitor/source.c, history.c, rules.c, format.c):
 * the simulation is a function of its seed, a recording plays as written,
 * the history keeps the newest values, the rules grade the state with real
 * fields only and fall one level at a time, and values are formatted.
 *
 *	plan/ws134/tests/host/run.sh
 */

#include "monitor.h"

#include <stdio.h>
#include <string.h>

/* The number of failed checks. */
static int failures;

static void check(int condition, const char *what);
static void test_sim(void);
static void test_replay(const char *path);
static void test_history(void);
static void test_rules(void);
static void test_format(void);

/* Runs every test; the first argument is the replay file. */
int
main(
	int argc,
	char **argv)
{
	/* Each part. */
	test_sim();
	if (argc > 1)
		test_replay(argv[1]);
	test_history();
	test_rules();
	test_format();

	/* The verdict. */
	if (failures != 0) {
		printf("monitor-host: FAIL (%d)\n", failures);
		return 1;
	}

	printf("monitor-host: PASS\n");
	return 0;
}

/* Counts and prints a failed check. */
static void
check(
	int condition,
	const char *what)
{
	/* A passed check is quiet. */
	if (condition)
		return;

	printf("FAIL: %s\n", what);
	failures++;
}

/* The simulation: the same seed gives the same frames, values stay in range, and everything is simulated. */
static void
test_sim(void)
{
	struct sm_source first;
	struct sm_source second;
	struct sm_frame a;
	struct sm_frame b;
	uint64_t now;
	int same;
	int in_range;
	int took;
	unsigned core;

	/* Two simulations of one seed. */
	(void)sm_source_open_sim(&first, 7, 1000, 8, 1, 0);
	(void)sm_source_open_sim(&second, 7, 1000, 8, 1, 0);
	same = 1;
	in_range = 1;
	for (now = 0; now < 600000U; now += 1000U) {
		took = sm_source_take(&first, now, &a);
		took &= sm_source_take(&second, now, &b);
		if (!took || a.time_ms != now)
			same = 0;
		if (memcmp(&a, &b, sizeof(a)) != 0)
			same = 0;
		if (a.cpu < 0.0 || a.cpu > 1.0 || a.memory_used > first.info.memory_total || a.disk_latency_ms <= 0.0)
			in_range = 0;
		for (core = 0; core < first.info.cpu_count; core++) {
			if (a.cpu_core[core] < 0.0 || a.cpu_core[core] > 1.0)
				in_range = 0;
		}
		if (a.simulated != a.valid)
			in_range = 0;
	}

	check(same, "sim: one seed gives one sequence");
	check(in_range, "sim: values in range and all simulated");

	/* A frame is not due before its time. */
	(void)sm_source_open_sim(&first, 7, 1000, 8, 1, 0);
	(void)sm_source_take(&first, 0, &a);
	check(sm_source_take(&first, 999, &a) == 0, "sim: no frame before the period");
	check(sm_source_take(&first, 1000, &a) == 1, "sim: a frame at the period");
}

/* A recording: the info line, the frames at their times, and the fields that were given. */
static void
test_replay(
	const char *path)
{
	struct sm_source source;
	struct sm_frame frame;
	int error;
	int count;

	/* The file opens and describes the machine. */
	error = sm_source_open_replay(&source, path, 1000);
	check(error == 0, "replay: opens");
	if (error != 0)
		return;
	check(source.info.cpu_count == 4U && source.info.gpu_count == 1U, "replay: the machine");
	check(strcmp(source.info.gpu_name[0], "Venus") == 0, "replay: the GPU's name");

	/* Its three frames come at their times. */
	count = 0;
	check(sm_source_take(&source, 0, &frame) == 1 && frame.time_ms == 0U, "replay: frame 0");
	count++;
	check(sm_source_take(&source, 500, &frame) == 0, "replay: none before its time");
	check(sm_source_take(&source, 1000, &frame) == 1, "replay: frame 1");
	count++;
	check(sm_source_take(&source, 2000, &frame) == 1, "replay: frame 2");
	count++;
	check(count == 3 && frame.cpu > 0.369 && frame.cpu < 0.371, "replay: the last frame's CPU");
	check(frame.cpu_core[1] > 0.779 && frame.cpu_core[1] < 0.781, "replay: a core's share");
	check((frame.valid & SM_HAVE_GPU_TEMPERATURE) != 0U && frame.simulated == 0U, "replay: valid and real fields");
	check(sm_source_take(&source, 9000, &frame) == 0, "replay: nothing after the end");
	sm_source_close(&source);
}

/* The history keeps the newest values in order and drops the oldest. */
static void
test_history(void)
{
	static struct sm_history history;
	struct sm_info info;
	struct sm_frame frame;
	float values[8];
	unsigned index;
	unsigned count;

	/* 4000 frames whose CPU is their number. */
	sm_history_init(&history);
	memset(&info, 0, sizeof(info));
	memset(&frame, 0, sizeof(frame));
	for (index = 0; index < 4000U; index++) {
		frame.cpu = (double)index;
		sm_history_add(&history, &info, &frame);
	}

	/* The newest eight, oldest first, and the peak of the hour. */
	count = sm_history_read(&history, SM_SERIES_CPU, 8, values);
	check(count == 8U && values[0] == 3992.0f && values[7] == 3999.0f, "history: newest in order");
	check(sm_history_peak(&history, SM_SERIES_CPU, SM_HISTORY_MAX) == 3999.0f, "history: peak");
	check(history.series[SM_SERIES_CPU].count == SM_HISTORY_MAX, "history: an hour kept");
}

/* The rules: real fields only, the times to rise, and the fall one level at a time. */
static void
test_rules(void)
{
	struct sm_rules rules;
	struct sm_info info;
	struct sm_frame frame;
	enum sm_level level;
	uint64_t time;

	/* A busy CPU, but simulated: nothing happens. */
	memset(&info, 0, sizeof(info));
	info.memory_total = 1000;
	memset(&frame, 0, sizeof(frame));
	frame.valid = SM_HAVE_CPU;
	frame.simulated = SM_HAVE_CPU;
	frame.cpu = 0.99;
	sm_rules_init(&rules);
	level = SM_LEVEL_NORMAL;
	for (time = 0; time <= 200000U; time += 1000U) {
		frame.time_ms = time;
		level = sm_rules_update(&rules, &info, &frame, 0);
	}

	check(level == SM_LEVEL_NORMAL, "rules: a simulated field grades nothing");

	/* From the simulation itself it does. */
	sm_rules_init(&rules);
	for (time = 0; time <= 11000U; time += 1000U) {
		frame.time_ms = time;
		level = sm_rules_update(&rules, &info, &frame, 1);
	}

	check(level == SM_LEVEL_ELEVATED && rules.rule_levels[SM_RULE_CPU] == SM_LEVEL_ELEVATED, "rules: the simulation's own fields grade");

	/* The same CPU really busy: Elevated at 10 s, Warning at 60 s, Critical at 120 s. */
	frame.simulated = 0;
	frame.cpu = 0.97;
	sm_rules_init(&rules);
	for (time = 0; time <= 9000U; time += 1000U) {
		frame.time_ms = time;
		level = sm_rules_update(&rules, &info, &frame, 0);
	}

	check(level == SM_LEVEL_NORMAL, "rules: not before 10 s");
	for (; time <= 10000U; time += 1000U) {
		frame.time_ms = time;
		level = sm_rules_update(&rules, &info, &frame, 0);
	}

	check(level == SM_LEVEL_ELEVATED, "rules: Elevated at 10 s");
	for (; time <= 60000U; time += 1000U) {
		frame.time_ms = time;
		level = sm_rules_update(&rules, &info, &frame, 0);
	}

	check(level == SM_LEVEL_WARNING, "rules: Warning at 60 s");
	for (; time <= 120000U; time += 1000U) {
		frame.time_ms = time;
		level = sm_rules_update(&rules, &info, &frame, 0);
	}

	check(level == SM_LEVEL_CRITICAL, "rules: Critical at 120 s");
	check(strncmp(rules.summary, "CPU 97", 6) == 0, "rules: the summary names the cause");

	/* Calm again: one level every 5 seconds. */
	frame.cpu = 0.2;
	frame.time_ms = time;
	level = sm_rules_update(&rules, &info, &frame, 0);
	check(level == SM_LEVEL_CRITICAL, "rules: no fall at once");
	frame.time_ms = time + 5000U;
	level = sm_rules_update(&rules, &info, &frame, 0);
	check(level == SM_LEVEL_WARNING, "rules: one level after 5 s");
	frame.time_ms = time + 15000U;
	(void)sm_rules_update(&rules, &info, &frame, 0);
	frame.time_ms = time + 20000U;
	level = sm_rules_update(&rules, &info, &frame, 0);
	check(level == SM_LEVEL_NORMAL, "rules: Normal after three steps");
	check(strcmp(rules.summary, "Stable") == 0, "rules: the summary is Stable");
}

/* The values' text. */
static void
test_format(void)
{
	char text[64];

	/* Percents, sizes, rates, pairs and uptimes. */
	(void)sm_format_percent(text, sizeof(text), 0.374);
	check(strcmp(text, "37%") == 0, "format: percent");
	(void)sm_format_bytes(text, sizeof(text), 2254857830ULL);
	check(strcmp(text, "2.1 GiB") == 0, "format: bytes");
	(void)sm_format_rate(text, sizeof(text), 525000.0, 1);
	check(strcmp(text, "4.2 Mb/s") == 0, "format: bits");
	(void)sm_format_rate(text, sizeof(text), 12000000.0, 0);
	check(strcmp(text, "12 MB/s") == 0, "format: bytes a second");
	(void)sm_format_pair(text, sizeof(text), 2254857830ULL, 8589934592ULL);
	check(strcmp(text, "2.1 / 8 GiB") == 0, "format: pair");
	(void)sm_format_uptime(text, sizeof(text), 8040);
	check(strcmp(text, "2:14") == 0, "format: uptime");
}
