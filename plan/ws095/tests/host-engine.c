/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * ws095: host tests of the input method's engines (plan/ws095/design.md
 * section 12), built on Linux with the engines' own sources:
 *
 *   sh plan/ws095/tests/host-engine.sh
 *
 * The romaji rules, the dictionary reader (with the fixed dictionary
 * ja-test.dict, and with REmacs's SKK-JISYO.X when it is at hand), the
 * okurigana rules, the splits of example sentences, the user dictionary's
 * round trip and damaged files, and the engine driven key by key are each
 * checked; each check prints ok or FAIL, and the last line counts them.
 *
 *   host-engine convert SYSTEM [SUPPLEMENT] -- READING...
 *
 * converts readings (in hiragana) with the given dictionaries and prints
 * each split with its first candidates, for measuring a dictionary.
 */

#include "ja.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* The evdev code the tests give a letter key; the engines read the character. */
#define TEST_KEY_LETTER		30U

/* The evdev code of the shift-less 0 key. */
#define TEST_KEY_0		11U

static int test_passed;
static int test_failed;
static char test_dir[256];
static char test_dict[1024];
static char test_x[1024];

static void check(int ok, const char *format, ...);
static void test_romaji(void);
static void test_dict_reader(void);
static void test_dict_x(void);
static void test_inflect(void);
static void test_conjugation(void);
static void test_split(void);
static void test_user(void);
static void test_engine_keys(void);
static void test_direct(void);
static void romaji_case(const char *typed, const char *expected);
static void inflect_case(const char *reading, size_t stem_end, char consonant, size_t end, int expected);
static void conjugated_case(const char *reading, size_t stem_end, char consonant, enum ja_conjugation conjugation, size_t end, int expected);
static void conjugation_of(const struct ja_dict *dict, const char *key, size_t index, const char *word, enum ja_conjugation expected);
static void annotated_split_case(const char *system, const char *supplement, const char *reading, const char *expected);
static void split_case(struct ja_core *core, const char *reading, const char *expected);
static size_t set_reading(struct ja_core *core, const char *reading);
static void join_split(const struct ja_core *core, char *out, size_t size);
static void key_char(struct ime_engine *engine, char character, struct ime_output *out);
static void key_code(struct ime_engine *engine, uint32_t code, uint32_t modifiers, struct ime_output *out);
static void type_text(struct ime_engine *engine, const char *text, struct ime_output *out);
static int write_file(const char *name, const char *text, char *path, size_t size);
static int convert_main(int argc, char **argv);

int
main(
	int argc,
	char **argv)
{
	char *made;

	/* The measuring mode. */
	if (argc >= 2 && strcmp(argv[1], "convert") == 0)
		return convert_main(argc, argv);

	/* The fixed dictionary beside this file, and REmacs's when it is there. */
	if (argc >= 2) {
		snprintf(test_dict, sizeof(test_dict), "%s", argv[1]);
	} else {
		snprintf(test_dict, sizeof(test_dict), "plan/ws095/tests/ja-test.dict");
	}

	if (argc >= 3) {
		snprintf(test_x, sizeof(test_x), "%s", argv[2]);
	} else {
		test_x[0] = '\0';
	}

	/* A directory of its own for the files the tests write. */
	snprintf(test_dir, sizeof(test_dir), "/tmp/ws095-host-engine-XXXXXX");
	made = mkdtemp(test_dir);
	if (made == NULL) {
		perror("mkdtemp");
		return 1;
	}

	test_romaji();
	test_dict_reader();
	test_dict_x();
	test_inflect();
	test_conjugation();
	test_split();
	test_user();
	test_engine_keys();
	test_direct();

	printf("host-engine: %d passed, %d failed\n", test_passed, test_failed);
	if (test_failed != 0)
		return 1;
	return 0;
}

static void
check(
	int ok,
	const char *format,
	...)
{
	va_list arguments;

	if (ok) {
		test_passed++;
		printf("ok   ");
	} else {
		test_failed++;
		printf("FAIL ");
	}

	va_start(arguments, format);
	vprintf(format, arguments);
	va_end(arguments);
	printf("\n");
}

/*
 * The romaji rules: whole spellings, ん, っ, the marks, and letters no
 * rule can use.
 */
static void
test_romaji(void)
{
	romaji_case("kanji", "かんじ");
	romaji_case("konnnichiha", "こんにちは");
	romaji_case("konnichiha", "こんいちは");
	romaji_case("kitte", "きって");
	romaji_case("matcha", "まっちゃ");
	romaji_case("kan'i", "かんい");
	romaji_case("shinbun", "しんぶn");
	romaji_case("shinbunn", "しんぶん");
	romaji_case("ca", "か");
	romaji_case("xtu", "っ");
	romaji_case("ltsu", "っ");
	romaji_case("kyakka", "きゃっか");
	romaji_case("thi", "てぃ");
	romaji_case("fa", "ふぁ");
	romaji_case("k", "k");
	romaji_case("q1", "q1");
	romaji_case("ko-hi-", "こーひー");
	romaji_case("a,b.", "あ、b。");
	romaji_case("[a]/", "「あ」・");
	romaji_case("n", "n");
	romaji_case("nn", "ん");
	romaji_case("x", "x");
	romaji_case("123", "123");
}

static void
romaji_case(
	const char *typed,
	const char *expected)
{
	struct ja_core core;
	struct ja_config config;
	struct ime_output *out;
	char user[1024];
	size_t i;
	int error;

	out = malloc(sizeof(*out));
	snprintf(user, sizeof(user), "%s/romaji-user.dict", test_dir);
	config.system_dictionary = "/nonexistent";
	config.supplement_dictionary = NULL;
	config.user_dictionary = user;
	error = ja_core_open(&core, &config);
	if (error != 0 || out == NULL) {
		check(0, "romaji %s: open", typed);
		free(out);
		return;
	}

	for (i = 0; typed[i] != '\0'; i++)
		(void)ja_core_type(&core, typed[i]);

	ime_output_clear(out);
	ja_core_output(&core, out);
	check(strcmp(out->preedit, expected) == 0, "romaji %s -> %s (got %s)", typed, expected, out->preedit);
	ja_core_close(&core);
	free(out);
}

/*
 * The dictionary reader with the fixed dictionary: headwords, malformed
 * lines, annotations and Lisp candidates.
 */
static void
test_dict_reader(void)
{
	struct ja_dict dict;
	const struct ja_dict_entry *entry;
	const char *candidate;
	size_t length;
	size_t position;
	bool more;
	int error;

	error = ja_dict_load(&dict, test_dict, JA_DICT_SIZE_MAX);
	check(error == 0, "dict: the fixed dictionary loads (error %d)", error);
	if (error != 0)
		return;

	check(dict.entry_count == 34, "dict: 34 headwords (got %zu)", dict.entry_count);
	check(dict.malformed_count == 3, "dict: 3 malformed lines skipped (got %zu)", dict.malformed_count);

	entry = ja_dict_find(&dict, "かk", strlen("かk"));
	check(entry != NULL, "dict: かk found");
	if (entry != NULL) {
		position = 0;
		more = ja_dict_next_candidate(entry, &position, &candidate, &length);
		check(more && length == strlen("書") && memcmp(candidate, "書", length) == 0, "dict: かk's first candidate is 書");
	}

	entry = ja_dict_find(&dict, "とうきょう", strlen("とうきょう"));
	check(entry != NULL, "dict: とうきょう found");
	if (entry != NULL) {
		position = 0;
		more = ja_dict_next_candidate(entry, &position, &candidate, &length);
		check(more && length == strlen("東京") && memcmp(candidate, "東京", length) == 0, "dict: the annotation is left out");
	}

	entry = ja_dict_find(&dict, "ひづけ", strlen("ひづけ"));
	check(entry != NULL, "dict: ひづけ found");
	if (entry != NULL) {
		position = 0;
		more = ja_dict_next_candidate(entry, &position, &candidate, &length);
		check(more && length == strlen("日付") && memcmp(candidate, "日付", length) == 0, "dict: a Lisp candidate is skipped");
		more = ja_dict_next_candidate(entry, &position, &candidate, &length);
		check(!more, "dict: no candidate after the last");
	}

	entry = ja_dict_find(&dict, "ないよみ", strlen("ないよみ"));
	check(entry == NULL, "dict: a reading not in it is not found");

	ja_dict_free(&dict);

	error = ja_dict_load(&dict, "/nonexistent/dict", JA_DICT_SIZE_MAX);
	check(error == ENOENT, "dict: a missing file is ENOENT (got %d)", error);

	error = ja_dict_load(&dict, test_dict, 16);
	check(error == EFBIG, "dict: a file over the limit is EFBIG (got %d)", error);
}

/*
 * REmacs's SKK-JISYO.X, when it is at hand: every line reads and is
 * indexed.
 */
static void
test_dict_x(void)
{
	struct ja_dict dict;
	FILE *file;
	char line[8192];
	size_t lines;
	int error;

	if (test_x[0] == '\0') {
		printf("skip dict X: no path given\n");
		return;
	}

	/* Counts the lines that are not comments. */
	file = fopen(test_x, "r");
	if (file == NULL) {
		printf("skip dict X: %s cannot be opened\n", test_x);
		return;
	}

	lines = 0;
	while (fgets(line, sizeof(line), file) != NULL) {
		if (line[0] != ';' && line[0] != '\n')
			lines++;
	}

	fclose(file);

	error = ja_dict_load(&dict, test_x, JA_DICT_SIZE_MAX);
	check(error == 0, "dict X: loads (error %d)", error);
	if (error != 0)
		return;

	check(dict.malformed_count == 0, "dict X: no malformed line (got %zu)", dict.malformed_count);
	check(dict.entry_count == lines, "dict X: %zu headwords, one per line (got %zu)", lines, dict.entry_count);
	ja_dict_free(&dict);
}

/*
 * The okurigana rules.
 */
static void
test_inflect(void)
{
	inflect_case("よんだ", 1, 'm', 3, 1);
	inflect_case("よみます", 1, 'm', 4, 1);
	inflect_case("よまない", 1, 'm', 4, 1);
	inflect_case("よめば", 1, 'm', 3, 1);
	inflect_case("よもう", 1, 'm', 3, 1);
	inflect_case("たべています", 1, 'b', 6, 1);
	inflect_case("たべた", 1, 'b', 3, 1);
	inflect_case("たべられない", 1, 'b', 6, 1);
	inflect_case("みます", 1, 'r', 3, 1);
	inflect_case("みた", 1, 'r', 2, 1);
	inflect_case("かった", 1, 'w', 3, 1);
	inflect_case("かわない", 1, 'w', 4, 1);
	inflect_case("かいた", 1, 'k', 3, 1);
	inflect_case("かきました", 1, 'k', 5, 1);
	inflect_case("かけば", 1, 'k', 3, 1);
	inflect_case("かこう", 1, 'k', 3, 1);
	inflect_case("いった", 1, 'k', 3, 1);
	inflect_case("かった", 1, 'k', 3, 0);
	inflect_case("はなした", 2, 's', 4, 1);
	inflect_case("たかかった", 2, 'k', 5, 1);
	inflect_case("たかい", 2, 'i', 3, 1);
	inflect_case("たかくない", 2, 'k', 5, 1);
	inflect_case("むずかしい", 3, 's', 5, 1);
	inflect_case("あがった", 1, 'g', 4, 1);
	inflect_case("あがります", 1, 'g', 5, 1);
	inflect_case("かんがえます", 3, 'e', 6, 1);
	inflect_case("とった", 1, 'r', 3, 1);
	inflect_case("よ", 1, 'm', 1, 0);
	inflect_case("よまないで", 1, 'm', 5, 1);
	inflect_case("たべないでください", 1, 'b', 9, 1);
}

static void
inflect_case(
	const char *reading,
	size_t stem_end,
	char consonant,
	size_t end,
	int expected)
{
	struct ja_unit units[JA_UNITS_MAX];
	struct ja_text *text;
	bool ends[JA_UNITS_MAX + 1U];
	size_t position;
	size_t count;
	size_t used;
	uint32_t code;

	text = malloc(sizeof(*text));
	count = 0;
	position = 0;
	while (reading[position] != '\0') {
		used = ja_utf8_decode(reading + position, strlen(reading + position), &code);
		memset(&units[count], 0, sizeof(units[count]));
		units[count].code = code;
		units[count].kind = JA_UNIT_KANA;
		count++;
		position += used;
	}

	ja_text_build(text, units, count);
	memset(ends, 0, sizeof(ends));
	ja_inflect_ends(text, stem_end, consonant, false, ends);
	check((ends[end] ? 1 : 0) == expected, "inflect %s (stem %zu, %c) %s at %zu", reading, stem_end, consonant,
	      expected ? "ends" : "does not end", end);
	free(text);
}

/*
 * The conjugations a supplement's annotations name (ws095-p012): the
 * reader gives them, the okurigana rules follow only the one named, and a
 * conversion leaves out a candidate whose conjugation does not fit.
 */
static void
test_conjugation(void)
{
	struct ja_dict dict;
	char supplement[1024];
	char system[1024];
	int error;

	/* Writes a small annotated supplement and a system dictionary with the words it competes with. */
	error = write_file("conjugation-kei.dict",
			   ";; annotated supplement of the host tests\n"
			   "かえr /帰;五段/変え;一段/\n"
			   "いr /要;五段/\n"
			   "いれr /入れ;一段/\n"
			   "かりr /借り;一段/\n"
			   "はなs /話;五段/\n"
			   "たかi /高;形容詞/\n"
			   "ともだち /友達/\n"
			   "とうきょう /東京;地名/\n",
			   supplement,
			   sizeof(supplement));
	check(error == 0, "conjugation: the supplement is written (error %d)", error);
	if (error != 0)
		return;

	error = write_file("conjugation-system.dict",
			   ";; system dictionary of the host tests\n"
			   "かr /借/狩/\n"
			   "なs /成/\n"
			   "とうきょう /東京/\n",
			   system,
			   sizeof(system));
	check(error == 0, "conjugation: the system dictionary is written (error %d)", error);
	if (error != 0)
		return;

	/* The reader gives each candidate's conjugation; an annotation for the reader names none. */
	error = ja_dict_load(&dict, supplement, JA_DICT_SIZE_MAX);
	check(error == 0, "conjugation: the supplement loads (error %d)", error);
	if (error != 0)
		return;

	conjugation_of(&dict, "かえr", 0, "帰", JA_CONJUGATION_GODAN);
	conjugation_of(&dict, "かえr", 1, "変え", JA_CONJUGATION_ICHIDAN);
	conjugation_of(&dict, "たかi", 0, "高", JA_CONJUGATION_ADJECTIVE);
	conjugation_of(&dict, "とうきょう", 0, "東京", JA_CONJUGATION_ANY);
	ja_dict_free(&dict);

	/* The okurigana of each conjugation alone. */
	conjugated_case("かえります", 2, 'r', JA_CONJUGATION_GODAN, 5, 1);
	conjugated_case("かえます", 2, 'r', JA_CONJUGATION_GODAN, 4, 0);
	conjugated_case("かえます", 2, 'r', JA_CONJUGATION_ICHIDAN, 4, 1);
	conjugated_case("かえります", 2, 'r', JA_CONJUGATION_ICHIDAN, 5, 0);
	conjugated_case("かえった", 2, 'r', JA_CONJUGATION_GODAN, 4, 1);
	conjugated_case("かえた", 2, 'r', JA_CONJUGATION_ICHIDAN, 3, 1);
	conjugated_case("かりた", 2, 'r', JA_CONJUGATION_ICHIDAN, 3, 1);
	conjugated_case("かった", 1, 'w', JA_CONJUGATION_GODAN, 3, 1);
	conjugated_case("かった", 1, 'w', JA_CONJUGATION_ICHIDAN, 3, 0);
	conjugated_case("たかかった", 2, 'i', JA_CONJUGATION_ADJECTIVE, 5, 1);
	conjugated_case("たかきます", 2, 'k', JA_CONJUGATION_ADJECTIVE, 5, 0);
	conjugated_case("たかきます", 2, 'k', JA_CONJUGATION_ANY, 5, 1);

	/* Conversions with the supplement before the system dictionary. */
	annotated_split_case(system, supplement, "かえります", "帰ります");
	annotated_split_case(system, supplement, "かえます", "変えます");
	annotated_split_case(system, supplement, "いれた", "入れた");
	annotated_split_case(system, supplement, "いります", "要ります");
	annotated_split_case(system, supplement, "かりた", "借りた");
	annotated_split_case(system, supplement, "とうきょう", "東京");

	/* Of two splits into as many segments, the one of the supplement's words (not 友達とは|成しました). */
	annotated_split_case(system, supplement, "ともだちとはなしました", "友達と|話しました");
}

/*
 * Checks where the okurigana of one conjugation ends after a stem.
 */
static void
conjugated_case(
	const char *reading,
	size_t stem_end,
	char consonant,
	enum ja_conjugation conjugation,
	size_t end,
	int expected)
{
	struct ja_unit units[JA_UNITS_MAX];
	struct ja_text *text;
	bool ends[JA_UNITS_MAX + 1U];
	size_t position;
	size_t count;
	size_t used;
	uint32_t code;
	int ended;

	/* The segmenter's view of the reading, every character a kana. */
	text = malloc(sizeof(*text));
	if (text == NULL) {
		check(0, "conjugated %s: no memory", reading);
		return;
	}

	count = 0;
	position = 0;
	while (reading[position] != '\0') {
		used = ja_utf8_decode(reading + position, strlen(reading + position), &code);
		memset(&units[count], 0, sizeof(units[count]));
		units[count].code = code;
		units[count].kind = JA_UNIT_KANA;
		count++;
		position += used;
	}

	ja_text_build(text, units, count);

	/* Marks the ends of the conjugation named and compares the one asked about. */
	memset(ends, 0, sizeof(ends));
	ja_inflect_conjugated_ends(text, stem_end, consonant, conjugation, ends);
	ended = 0;
	if (ends[end])
		ended = 1;

	check(ended == expected, "conjugated %s (stem %zu, %c, conjugation %d) %s at %zu", reading, stem_end, consonant,
	      (int)conjugation, expected ? "ends" : "does not end", end);
	free(text);
}

/*
 * Checks one candidate of a headword and the conjugation its annotation names.
 */
static void
conjugation_of(
	const struct ja_dict *dict,
	const char *key,
	size_t index,
	const char *word,
	enum ja_conjugation expected)
{
	const struct ja_dict_entry *entry;
	enum ja_conjugation conjugation;
	const char *candidate;
	size_t length;
	size_t position;
	size_t i;
	bool more;
	bool same;

	/* Finds the headword. */
	entry = ja_dict_find(dict, key, strlen(key));
	if (entry == NULL) {
		check(0, "conjugation: %s found", key);
		return;
	}

	/* Walks to the candidate asked about. */
	position = 0;
	more = false;
	for (i = 0; i <= index; i++) {
		more = ja_dict_next_conjugated(entry, &position, &candidate, &length, &conjugation);
		if (!more)
			break;
	}

	if (!more) {
		check(0, "conjugation: %s has a candidate %zu", key, index);
		return;
	}

	/* The word without its annotation, and the conjugation named. */
	same = ja_bytes_equal(candidate, length, word, strlen(word));
	check(same, "conjugation: %s candidate %zu is %s", key, index, word);
	check(conjugation == expected, "conjugation: %s %s names conjugation %d (got %d)", key, word, (int)expected,
	      (int)conjugation);
}

/*
 * Converts a reading with a system dictionary and a supplement and checks
 * the split with its first candidates.
 */
static void
annotated_split_case(
	const char *system,
	const char *supplement,
	const char *reading,
	const char *expected)
{
	struct ja_core core;
	struct ja_config config;
	char user[1024];
	int error;

	/* Opens the engine with both dictionaries and an empty user dictionary of its own. */
	snprintf(user, sizeof(user), "%s/conjugation-user.dict", test_dir);
	(void)unlink(user);
	config.system_dictionary = system;
	config.supplement_dictionary = supplement;
	config.user_dictionary = user;
	error = ja_core_open(&core, &config);
	if (error != 0) {
		check(0, "conjugation: the engine opens (error %d)", error);
		return;
	}

	/* Converts and compares the split. */
	split_case(&core, reading, expected);
	ja_core_close(&core);
}

/*
 * Splits of example sentences with the fixed dictionary.
 */
static void
test_split(void)
{
	struct ja_core core;
	struct ja_config config;
	char user[1024];
	int error;
	size_t i;
	bool found;

	snprintf(user, sizeof(user), "%s/split-user.dict", test_dir);
	config.system_dictionary = test_dict;
	config.supplement_dictionary = NULL;
	config.user_dictionary = user;
	error = ja_core_open(&core, &config);
	check(error == 0 && core.system_error == 0, "split: the engine opens with the fixed dictionary");
	if (error != 0)
		return;

	split_case(&core, "ほんをよんだ", "本を|読んだ");
	split_case(&core, "たべています", "食べています");
	split_case(&core, "きょうはいいてんきです", "今日は|いい|天気です");
	split_case(&core, "べんきょうします", "勉強します");
	split_case(&core, "わたしはにほんごをはなします", "私は|日本語を|話します");
	split_case(&core, "かいしゃにいきます", "会社に|行きます");
	split_case(&core, "みます", "見ます");
	split_case(&core, "たかかった", "高かった");
	split_case(&core, "むずかしい", "難しい");
	split_case(&core, "かんがえています", "考えています");
	split_case(&core, "ものをかった", "物を|変った");
	split_case(&core, "でんしゃにのって", "電車に|乗って");
	split_case(&core, "きょうはいいてんきですね", "今日は|いい|天気ですね");
	split_case(&core, "どこからきましたか", "どこから|来ましたか");
	split_case(&core, "たかくない", "高くない");
	split_case(&core, "わたしはいます", "私は|います");
	split_case(&core, "ほんですね", "本ですね");
	split_case(&core, "こーひー", "コーヒー");
	split_case(&core, "こーひーを", "コーヒーを");
	split_case(&core, "ぱそこんを", "ぱそこんを");

	/* An unknown word is offered in katakana, its particle in hiragana. */
	found = false;
	for (i = 0; i < core.segments[0].candidate_count; i++) {
		if (strcmp(core.segments[0].candidates[i], "パソコンを") == 0)
			found = true;
	}
	check(found, "split: ぱそこんを offers パソコンを");

	ja_core_close(&core);
}

static void
split_case(
	struct ja_core *core,
	const char *reading,
	const char *expected)
{
	char joined[4096];

	ja_core_clear(core);
	(void)set_reading(core, reading);
	(void)ja_core_convert(core);
	join_split(core, joined, sizeof(joined));
	check(strcmp(joined, expected) == 0, "split %s -> %s (got %s)", reading, expected, joined);
}

/*
 * Puts a reading in hiragana straight into a composition.
 */
static size_t
set_reading(
	struct ja_core *core,
	const char *reading)
{
	size_t position;
	size_t used;
	uint32_t code;

	position = 0;
	core->unit_count = 0;
	while (reading[position] != '\0' && core->unit_count < JA_UNITS_MAX) {
		used = ja_utf8_decode(reading + position, strlen(reading + position), &code);
		memset(&core->units[core->unit_count], 0, sizeof(core->units[0]));
		core->units[core->unit_count].code = code;
		core->units[core->unit_count].kind = JA_UNIT_KANA;
		if (!ja_is_hiragana(code) && code != 0x30fcU)
			core->units[core->unit_count].kind = JA_UNIT_LITERAL;
		core->unit_count++;
		position += used;
	}

	return core->unit_count;
}

/*
 * Writes a conversion's first candidates with a bar between segments.
 */
static void
join_split(
	const struct ja_core *core,
	char *out,
	size_t size)
{
	size_t length;
	size_t i;
	const struct ja_segment *segment;

	length = 0;
	out[0] = '\0';
	for (i = 0; i < core->segment_count; i++) {
		segment = &core->segments[i];
		length += (size_t)snprintf(out + length, size - length, "%s%s", i == 0 ? "" : "|",
					   segment->candidate_count != 0 ? segment->candidates[segment->selected] : "?");
		if (length >= size)
			break;
	}
}

/*
 * The user dictionary: learning, saving, reading back, a damaged file and
 * an oversized one.
 */
static void
test_user(void)
{
	struct ja_user user;
	const struct ja_user_entry *entry;
	struct stat status;
	char path[1024];
	char damaged[1024];
	char big[1024];
	FILE *file;
	size_t i;
	int error;

	snprintf(path, sizeof(path), "%s/user.dict", test_dir);
	error = ja_user_open(&user, path);
	check(error == 0, "user: opens with no file (error %d)", error);
	error = ja_user_learn(&user, "わたし", strlen("わたし"), "私");
	check(error == 0, "user: learns 私");
	error = ja_user_learn(&user, "わたし", strlen("わたし"), "渡し");
	check(error == 0, "user: learns 渡し");
	error = ja_user_learn(&user, "わたし", strlen("わたし"), "私");
	check(error == 0, "user: learns 私 again");
	error = ja_user_learn(&user, "きょう", strlen("きょう"), "今日");
	check(error == 0, "user: learns 今日");
	error = ja_user_learn(&user, "a/b", 3, "x");
	check(error == EINVAL, "user: a reading with a slash is refused");
	error = ja_user_save(&user);
	check(error == 0, "user: saves (error %d)", error);
	ja_user_free(&user);

	error = stat(path, &status);
	check(error == 0 && (status.st_mode & 0777) == 0600, "user: the file is the user's alone (mode %o)", status.st_mode & 0777);

	error = ja_user_open(&user, path);
	check(error == 0, "user: reopens");
	entry = ja_user_find(&user, "わたし", strlen("わたし"));
	check(entry != NULL && entry->candidate_count == 2, "user: わたし has two candidates");
	if (entry != NULL && entry->candidate_count == 2) {
		check(strcmp(entry->candidates[0], "私") == 0, "user: the last choice 私 is first");
		check(strcmp(entry->candidates[1], "渡し") == 0, "user: 渡し is second");
	}

	entry = ja_user_find(&user, "きょう", strlen("きょう"));
	check(entry != NULL, "user: きょう is kept");
	ja_user_free(&user);

	/* A damaged file gives its good lines. */
	error = write_file("damaged.dict", ";; header\nよい /良い/\nbroken\nわるい /悪い/\n / /\n", damaged, sizeof(damaged));
	check(error == 0, "user: the damaged file is written");
	error = ja_user_open(&user, damaged);
	check(error == 0 && user.entry_count == 2 && user.malformed_count == 2, "user: a damaged file gives its 2 good lines (%zu, %zu malformed)",
	      user.entry_count, user.malformed_count);
	ja_user_free(&user);

	/* A file over the limit is not read at all. */
	snprintf(big, sizeof(big), "%s/big.dict", test_dir);
	file = fopen(big, "w");
	for (i = 0; file != NULL && i < JA_USER_SIZE_MAX / 16U + 10U; i++)
		fprintf(file, "あいう%06zu /x/\n", i % 1000000U);
	if (file != NULL)
		fclose(file);
	error = ja_user_open(&user, big);
	check(error == 0 && user.entry_count == 0, "user: an oversized file is not read (%zu entries)", user.entry_count);
	ja_user_free(&user);
}

/*
 * The Japanese engine driven key by key, as the Wayland side will.
 */
static void
test_engine_keys(void)
{
	struct ime_engine engine;
	struct ja_config config;
	struct ime_output *out;
	char user[1024];
	int error;
	size_t i;

	out = malloc(sizeof(*out));
	snprintf(user, sizeof(user), "%s/keys-user.dict", test_dir);
	config.system_dictionary = test_dict;
	config.supplement_dictionary = NULL;
	config.user_dictionary = user;
	error = ja_engine_create(&engine, &config);
	check(error == 0, "keys: the engine is made");
	if (error != 0 || out == NULL)
		return;

	check(strcmp(engine.ops->id, "ja") == 0 && strcmp(engine.ops->label, "あ") == 0, "keys: id ja, label あ");

	/* With nothing composed, a space and Enter are the application's. */
	key_code(&engine, IME_KEY_SPACE, 0, out);
	check(out->pass_key && !out->composing, "keys: an idle space passes");
	key_code(&engine, IME_KEY_ENTER, 0, out);
	check(out->pass_key, "keys: an idle Enter passes");

	/* Typing shows kana with the cursor at the end. */
	type_text(&engine, "watasi", out);
	check(strcmp(out->preedit, "わたし") == 0 && out->composing && !out->pass_key, "keys: watasi shows わたし");
	check(out->cursor_begin == (int32_t)strlen("わたし") && out->cursor_end == out->cursor_begin, "keys: the cursor is after the kana");

	/* Space converts; Enter commits. */
	key_code(&engine, IME_KEY_SPACE, 0, out);
	check(strcmp(out->preedit, "私") == 0, "keys: Space converts to 私 (got %s)", out->preedit);
	check(out->cursor_begin == 0 && out->cursor_end == (int32_t)strlen("私"), "keys: the segment in focus is the cursor range");
	key_code(&engine, IME_KEY_SPACE, 0, out);
	check(strcmp(out->preedit, "渡し") == 0 && !out->candidates_shown, "keys: a second Space gives 渡し, no window yet");
	key_code(&engine, IME_KEY_SPACE, 0, out);
	check(strcmp(out->preedit, "渡") == 0 && out->candidates_shown, "keys: a third Space gives 渡 and opens the window");
	check(out->candidate_selected == 2 && strcmp(out->candidates[0], "私") == 0, "keys: the window lists the candidates");
	key_code(&engine, IME_KEY_1 + 1U, 0, out);
	check(strcmp(out->preedit, "渡し") == 0, "keys: 2 chooses the second candidate");
	key_code(&engine, IME_KEY_ENTER, 0, out);
	check(strcmp(out->commit, "渡し") == 0 && out->preedit_length == 0 && !out->composing, "keys: Enter commits 渡し");

	/* The choice is learned: 渡し comes first now. */
	type_text(&engine, "watasi", out);
	key_code(&engine, IME_KEY_SPACE, 0, out);
	check(strcmp(out->preedit, "渡し") == 0, "keys: the learned 渡し comes first (got %s)", out->preedit);
	key_code(&engine, IME_KEY_ESCAPE, 0, out);
	check(strcmp(out->preedit, "わたし") == 0, "keys: Escape goes back to the kana");
	key_code(&engine, IME_KEY_ESCAPE, 0, out);
	check(out->preedit_length == 0 && !out->composing, "keys: a second Escape drops the composition");

	/* A sentence: move among the segments, shorten one, commit all. */
	type_text(&engine, "kyouhaiitenkidesu", out);
	key_code(&engine, IME_KEY_SPACE, 0, out);
	check(strcmp(out->preedit, "今日はいい天気です") == 0, "keys: the sentence converts (got %s)", out->preedit);
	check(out->cursor_begin == 0 && out->cursor_end == (int32_t)strlen("今日は"), "keys: the focus is on 今日は");
	key_code(&engine, IME_KEY_RIGHT, 0, out);
	check(out->cursor_begin == (int32_t)strlen("今日は") && out->cursor_end == (int32_t)strlen("今日はいい"), "keys: Right moves the focus to いい");
	key_code(&engine, IME_KEY_LEFT, 0, out);
	key_code(&engine, IME_KEY_LEFT, IME_MOD_SHIFT, out);
	check(strcmp(out->preedit, "今日はいい天気です") == 0 && out->cursor_end == (int32_t)strlen("今日"),
	      "keys: Shift+Left leaves 今日 and は (got %s, %d)", out->preedit, out->cursor_end);
	key_code(&engine, IME_KEY_RIGHT, IME_MOD_SHIFT, out);
	check(out->cursor_end == (int32_t)strlen("今日は"), "keys: Shift+Right takes the は back");
	key_code(&engine, IME_KEY_ENTER, 0, out);
	check(strcmp(out->commit, "今日はいい天気です") == 0, "keys: Enter commits the sentence");

	/* A character typed while converting commits and starts anew. */
	type_text(&engine, "hon", out);
	key_code(&engine, IME_KEY_SPACE, 0, out);
	key_char(&engine, 'w', out);
	check(strcmp(out->commit, "本") == 0 && strcmp(out->preedit, "w") == 0, "keys: typing commits 本 and starts w");
	key_code(&engine, IME_KEY_BACKSPACE, 0, out);
	check(out->preedit_length == 0 && !out->composing, "keys: Backspace takes the pending w back");

	/* The forms of F6 to F10. */
	type_text(&engine, "pasokon", out);
	key_code(&engine, IME_KEY_F7, 0, out);
	check(strcmp(out->preedit, "パソコン") == 0, "keys: F7 gives パソコン (got %s)", out->preedit);
	key_code(&engine, IME_KEY_F8, 0, out);
	check(strcmp(out->preedit, "ﾊﾟｿｺﾝ") == 0, "keys: F8 gives ﾊﾟｿｺﾝ (got %s)", out->preedit);
	key_code(&engine, IME_KEY_F9, 0, out);
	check(strcmp(out->preedit, "ｐａｓｏｋｏｎ") == 0, "keys: F9 gives ｐａｓｏｋｏｎ (got %s)", out->preedit);
	key_code(&engine, IME_KEY_F10, 0, out);
	check(strcmp(out->preedit, "pasokon") == 0, "keys: F10 gives pasokon (got %s)", out->preedit);
	key_code(&engine, IME_KEY_F6, 0, out);
	check(strcmp(out->preedit, "ぱそこん") == 0, "keys: F6 gives ぱそこん (got %s)", out->preedit);
	key_code(&engine, IME_KEY_ENTER, 0, out);
	check(strcmp(out->commit, "ぱそこん") == 0, "keys: Enter commits the form");

	/* A capital letter keeps the word in letters. */
	type_text(&engine, "Linux", out);
	check(strcmp(out->preedit, "Linux") == 0, "keys: Linux stays in letters (got %s)", out->preedit);
	key_code(&engine, IME_KEY_ENTER, 0, out);
	check(strcmp(out->commit, "Linux") == 0, "keys: Enter commits Linux");
	type_text(&engine, "ka", out);
	check(strcmp(out->preedit, "か") == 0, "keys: kana again after the commit");

	/* A shortcut commits what is composed and passes. */
	key_code(&engine, TEST_KEY_LETTER, IME_MOD_CTRL, out);
	check(strcmp(out->commit, "か") == 0 && out->pass_key, "keys: Ctrl commits か and passes the key");

	/* Digits while composing are half width; with nothing composed they start a composition. */
	type_text(&engine, "3ko", out);
	check(strcmp(out->preedit, "3こ") == 0, "keys: 3ko shows 3こ (got %s)", out->preedit);

	/* Reset with commit (switching languages) and without (deactivation). */
	engine.ops->reset(&engine, true, out);
	check(strcmp(out->commit, "3こ") == 0 && out->preedit_length == 0, "keys: reset commits 3こ");
	type_text(&engine, "abc", out);
	engine.ops->reset(&engine, false, out);
	check(out->commit_length == 0 && out->preedit_length == 0, "keys: reset without commit drops the composition");

	/* A composition takes at most its limit, and ignores the rest. */
	for (i = 0; i < JA_UNITS_MAX + 20U; i++)
		key_char(&engine, 'a', out);
	check(out->preedit_length <= JA_UNITS_MAX * 3U && out->preedit_length >= (JA_UNITS_MAX - JA_RAW_MAX) * 3U,
	      "keys: a long composition stops at its limit (%zu bytes)", out->preedit_length);
	engine.ops->reset(&engine, false, out);

	/* A secret field teaches nothing: 渡 chosen there does not come first later. */
	engine.ops->content_type(&engine, IME_HINT_SENSITIVE_DATA, 0);
	type_text(&engine, "watasi", out);
	key_code(&engine, IME_KEY_SPACE, 0, out);
	key_code(&engine, IME_KEY_SPACE, 0, out);
	key_code(&engine, IME_KEY_SPACE, 0, out);
	check(strcmp(out->preedit, "渡") == 0, "keys: 渡 is chosen in a sensitive field (got %s)", out->preedit);
	key_code(&engine, IME_KEY_ENTER, 0, out);
	engine.ops->content_type(&engine, 0, 0);
	type_text(&engine, "watasi", out);
	key_code(&engine, IME_KEY_SPACE, 0, out);
	check(strcmp(out->preedit, "渡し") == 0, "keys: the choice in the sensitive field was not learned (got %s)", out->preedit);
	engine.ops->reset(&engine, false, out);

	/* The arrows while composing do not reach the application. */
	type_text(&engine, "ka", out);
	key_code(&engine, IME_KEY_LEFT, 0, out);
	check(!out->pass_key && strcmp(out->preedit, "か") == 0, "keys: Left while composing is swallowed");
	engine.ops->reset(&engine, false, out);

	engine.ops->destroy(&engine);
	free(out);
}

/*
 * The direct engine gives every key back.
 */
static void
test_direct(void)
{
	struct ime_engine engine;
	struct ime_output *out;
	int error;

	out = malloc(sizeof(*out));
	error = ime_direct_create(&engine);
	check(error == 0 && strcmp(engine.ops->id, "direct") == 0, "direct: made");
	key_char(&engine, 'a', out);
	check(out->pass_key && out->preedit_length == 0 && out->commit_length == 0, "direct: a passes");
	engine.ops->destroy(&engine);
	free(out);
}

static void
key_char(
	struct ime_engine *engine,
	char character,
	struct ime_output *out)
{
	struct ime_key key;

	key.code = TEST_KEY_LETTER;
	key.character = (unsigned char)character;
	key.modifiers = 0;
	if (character >= 'A' && character <= 'Z')
		key.modifiers = IME_MOD_SHIFT;
	if (character >= '1' && character <= '9')
		key.code = IME_KEY_1 + (uint32_t)(character - '1');
	if (character == '0')
		key.code = TEST_KEY_0;
	if (character == ' ')
		key.code = IME_KEY_SPACE;
	engine->ops->key(engine, &key, out);
}

static void
key_code(
	struct ime_engine *engine,
	uint32_t code,
	uint32_t modifiers,
	struct ime_output *out)
{
	struct ime_key key;

	key.code = code;
	key.character = 0;
	if (code == IME_KEY_SPACE)
		key.character = ' ';
	if (code >= IME_KEY_1 && code <= IME_KEY_9)
		key.character = '1' + (code - IME_KEY_1);
	if (code == TEST_KEY_LETTER)
		key.character = 'a';
	key.modifiers = modifiers;
	engine->ops->key(engine, &key, out);
}

static void
type_text(
	struct ime_engine *engine,
	const char *text,
	struct ime_output *out)
{
	size_t i;

	for (i = 0; text[i] != '\0'; i++)
		key_char(engine, text[i], out);
}

static int
write_file(
	const char *name,
	const char *text,
	char *path,
	size_t size)
{
	FILE *file;

	snprintf(path, size, "%s/%s", test_dir, name);
	file = fopen(path, "w");
	if (file == NULL)
		return errno;
	fputs(text, file);
	fclose(file);
	return 0;
}

/*
 * Converts readings with given dictionaries and prints each split.
 */
static int
convert_main(
	int argc,
	char **argv)
{
	struct ja_core core;
	struct ja_config config;
	char joined[4096];
	char user[256];
	int first;
	int i;
	int error;

	if (argc < 5) {
		fprintf(stderr, "usage: host-engine convert SYSTEM [SUPPLEMENT] -- READING...\n");
		return 2;
	}

	config.system_dictionary = argv[2];
	config.supplement_dictionary = NULL;
	first = 4;
	if (strcmp(argv[3], "--") != 0) {
		config.supplement_dictionary = argv[3];
		first = 5;
	}

	snprintf(user, sizeof(user), "/tmp/ws095-convert-user-%ld.dict", (long)getpid());
	config.user_dictionary = user;
	error = ja_core_open(&core, &config);
	if (error != 0) {
		fprintf(stderr, "host-engine: cannot open the engine (%d)\n", error);
		return 1;
	}

	if (core.system_error != 0)
		fprintf(stderr, "host-engine: the system dictionary cannot be read (%d)\n", core.system_error);

	for (i = first; i < argc; i++) {
		ja_core_clear(&core);
		(void)set_reading(&core, argv[i]);
		(void)ja_core_convert(&core);
		join_split(&core, joined, sizeof(joined));
		printf("%s\t%s\n", argv[i], joined);
	}

	ja_core_close(&core);
	unlink(user);
	return 0;
}
