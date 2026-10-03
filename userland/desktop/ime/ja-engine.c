/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The Japanese engine's state and the operations on it (plan/ws095/design.md
 * section 7).
 *
 * A composition is a row of characters (kana and literals) with the
 * letters typed that do not yet make a kana.  Converting splits it into
 * segments, each with its candidates; the segment in focus can be moved,
 * made longer or shorter, and given another candidate or another form.
 * Committing gives the text and teaches the user dictionary the choices.
 *
 * These operations know no keys: ja-keys.c decides which key does which,
 * so that another way of converting can bind the same operations
 * differently.  The ime_engine functions at the end tie the two together.
 */

#include "ja.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* How many candidates one page of the candidate window shows. */
#define ENGINE_PAGE_SIZE	9U

/* How many times the next candidate is asked for before the window opens. */
#define ENGINE_PRESSES_WINDOW	2U

static bool engine_append_units(struct ja_core *core, const struct ja_romaji_result *result);
static void engine_commit_conversion(struct ja_core *core, struct ime_output *out);
static void engine_commit_composition(struct ja_core *core, struct ime_output *out);
static void engine_output_conversion(const struct ja_core *core, struct ime_output *out);
static void engine_flush_romaji(struct ja_core *core);
static void engine_fill_segment(struct ja_core *core, const struct ja_text *text, size_t index, size_t start, size_t end);
static size_t engine_split_from(struct ja_core *core, const struct ja_text *text, size_t first, size_t start);
static size_t engine_form_text(const struct ja_core *core, const struct ja_segment *segment, enum ja_form form, char *out, size_t size);
static void engine_key(struct ime_engine *engine, const struct ime_key *key, struct ime_output *out);
static void engine_reset(struct ime_engine *engine, bool commit, struct ime_output *out);
static void engine_surrounding(struct ime_engine *engine, const char *text, uint32_t cursor, uint32_t anchor);
static void engine_content_type(struct ime_engine *engine, uint32_t hint, uint32_t purpose);
static void engine_destroy(struct ime_engine *engine);

/*
 * The Japanese engine's functions, as the Wayland side calls them.
 */
static const struct ime_engine_ops engine_ops = {
	"ja",
	"あ",
	engine_key,
	engine_reset,
	engine_surrounding,
	engine_content_type,
	engine_destroy
};

/*
 * Makes the Japanese engine, reading its dictionaries.
 *
 * Returns 0 (also when a dictionary cannot be read: the engine then
 * converts with what it has), or ENOMEM.
 */
int
ja_engine_create(
	struct ime_engine *engine,
	const struct ja_config *config)
{
	struct ja_core *core;
	int error;

	/* Allocates the engine's state. */
	core = malloc(sizeof(*core));
	if (core == NULL)
		return ENOMEM;

	/* Opens its dictionaries. */
	error = ja_core_open(core, config);
	if (error != 0) {
		free(core);
		return error;
	}

	/* Succeeded: the engine is ready for keys. */
	engine->ops = &engine_ops;
	engine->state = core;
	return 0;
}

/*
 * Opens the engine's state and its dictionaries.
 *
 * A dictionary that cannot be read is noted in the state and left out.
 * Returns 0, or ENOMEM.
 */
int
ja_core_open(
	struct ja_core *core,
	const struct ja_config *config)
{
	int error;

	memset(core, 0, sizeof(*core));

	/* The segments of a conversion, one per character at most. */
	core->segments = calloc(JA_UNITS_MAX, sizeof(core->segments[0]));
	if (core->segments == NULL)
		return ENOMEM;

	/* The user's choices come first. */
	core->user_error = ja_user_open(&core->user, config->user_dictionary);
	if (core->user_error == ENOMEM) {
		ja_core_close(core);
		return ENOMEM;
	}

	if (core->user_error == 0)
		core->lexicon.user = &core->user;

	/* Then the supplement, when there is one. */
	if (config->supplement_dictionary != NULL) {
		error = ja_dict_load(&core->supplement, config->supplement_dictionary, JA_DICT_SIZE_MAX);
		core->supplement_error = error;
		if (error == ENOMEM) {
			ja_core_close(core);
			return ENOMEM;
		}

		if (error == 0) {
			core->has_supplement = true;
			core->lexicon.dicts[core->lexicon.dict_count] = &core->supplement;
			core->lexicon.dict_count++;
		}
	}

	/* Then the system dictionary. */
	error = ja_dict_load(&core->system, config->system_dictionary, JA_DICT_SIZE_MAX);
	core->system_error = error;
	if (error == ENOMEM) {
		ja_core_close(core);
		return ENOMEM;
	}

	if (error == 0) {
		core->lexicon.dicts[core->lexicon.dict_count] = &core->system;
		core->lexicon.dict_count++;
	}

	/* Succeeded: the engine rests with nothing composed, and learns. */
	ja_romaji_reset(&core->romaji);
	core->learning = true;
	return 0;
}

/*
 * Frees the engine's state and its dictionaries.
 */
void
ja_core_close(
	struct ja_core *core)
{
	/* The dictionaries, then the segments. */
	ja_user_free(&core->user);
	ja_dict_free(&core->supplement);
	ja_dict_free(&core->system);
	free(core->segments);
	memset(core, 0, sizeof(*core));
}

/*
 * Tells whether anything is being composed.
 */
bool
ja_core_composing(
	const struct ja_core *core)
{
	/* Characters, or letters waiting to become one. */
	if (core->unit_count != 0U)
		return true;

	if (core->romaji.pending_length != 0U)
		return true;

	/* Nothing. */
	return false;
}

/*
 * Types a character into the composition: as romaji, or as a literal
 * while letters typed with shift are latched.
 *
 * Returns false when the composition is full and the character is not
 * added.
 */
bool
ja_core_type(
	struct ja_core *core,
	char letter)
{
	struct ja_romaji_result result;
	bool appended;

	/* A full composition takes no more. */
	if (core->unit_count + JA_RAW_MAX > JA_UNITS_MAX)
		return false;

	/* While latched, every character is kept as typed. */
	if (core->literal_latch) {
		result.count = 0;
		memset(&result.units[0], 0, sizeof(result.units[0]));
		result.units[0].code = (unsigned char)letter;
		result.units[0].kind = JA_UNIT_LITERAL;
		result.units[0].raw[0] = letter;
		result.count = 1;
		appended = engine_append_units(core, &result);
		if (!appended)
			return false;

		/* Succeeded: the character is in the composition. */
		return true;
	}

	/* Otherwise the letter goes through the romaji rules. */
	ja_romaji_feed(&core->romaji, letter, &result);
	appended = engine_append_units(core, &result);
	if (!appended)
		return false;

	/* Succeeded: what the letter completed is in the composition. */
	return true;
}

/*
 * Types a character as a literal, whatever the romaji rules would make of
 * it.
 *
 * Returns false when the composition is full.
 */
bool
ja_core_type_literal(
	struct ja_core *core,
	char letter)
{
	struct ja_romaji_result result;
	bool appended;

	/* A full composition takes no more. */
	if (core->unit_count + JA_RAW_MAX > JA_UNITS_MAX)
		return false;

	/* Ends the pending letters first. */
	engine_flush_romaji(core);

	/* The character as typed. */
	memset(&result.units[0], 0, sizeof(result.units[0]));
	result.units[0].code = (unsigned char)letter;
	result.units[0].kind = JA_UNIT_LITERAL;
	result.units[0].raw[0] = letter;
	result.count = 1;
	appended = engine_append_units(core, &result);
	if (!appended)
		return false;

	/* Succeeded: the character is in the composition. */
	return true;
}

/*
 * Keeps every character typed from now until the next conversion or
 * commit as it is typed (a letter typed with shift starts a word in
 * letters).
 */
void
ja_core_latch_literal(
	struct ja_core *core)
{
	/* The pending letters end first, so that they are not mixed in. */
	engine_flush_romaji(core);
	core->literal_latch = true;
}

/*
 * Tells whether characters are being kept as typed.
 */
bool
ja_core_literal_latched(
	const struct ja_core *core)
{
	/* The latch set by a letter typed with shift. */
	return core->literal_latch;
}

/*
 * Takes back the last thing typed: a pending letter, or the last
 * character.  While converting, it ends the conversion instead.
 */
void
ja_core_backspace(
	struct ja_core *core)
{
	bool taken;

	/* A conversion goes back to its kana. */
	if (core->converting) {
		ja_core_cancel_conversion(core);
		return;
	}

	/* A pending letter goes first. */
	taken = ja_romaji_backspace(&core->romaji);
	if (taken)
		return;

	/* Then the last character. */
	if (core->unit_count != 0U)
		core->unit_count--;

	/* An empty composition types kana again. */
	if (core->unit_count == 0U)
		core->literal_latch = false;
}

/*
 * Drops everything being composed.
 */
void
ja_core_clear(
	struct ja_core *core)
{
	/* Back to the resting state. */
	core->unit_count = 0;
	core->segment_count = 0;
	core->focus = 0;
	core->converting = false;
	core->literal_latch = false;
	ja_romaji_reset(&core->romaji);
}

/*
 * Converts the whole composition: splits it into segments and puts the
 * focus on the first.
 *
 * Returns false when there is nothing to convert.
 */
bool
ja_core_convert(
	struct ja_core *core)
{
	struct ja_text *text;

	/* A lone n and the other pending letters end first. */
	engine_flush_romaji(core);

	/* Nothing to convert. */
	if (core->unit_count == 0U)
		return false;

	/* Reads the composition as the segmenter does. */
	text = malloc(sizeof(*text));
	if (text == NULL)
		return false;

	ja_text_build(text, core->units, core->unit_count);

	/* Splits all of it. */
	core->segment_count = engine_split_from(core, text, 0, 0);
	free(text);

	/* Succeeded: the conversion shows its first candidates. */
	core->focus = 0;
	core->converting = true;
	core->literal_latch = false;
	return true;
}

/*
 * Ends a conversion and goes back to the kana it was made from.
 */
void
ja_core_cancel_conversion(
	struct ja_core *core)
{
	/* The characters stay; the segments go. */
	core->converting = false;
	core->segment_count = 0;
	core->focus = 0;
}

/*
 * Moves the segment in focus to another of its candidates, going round.
 */
void
ja_core_next_candidate(
	struct ja_core *core,
	int step)
{
	struct ja_segment *segment;
	size_t count;
	size_t moves;

	/* Only a conversion has candidates. */
	if (!core->converting || core->segment_count == 0U)
		return;

	segment = &core->segments[core->focus];
	count = segment->candidate_count;
	if (count == 0U)
		return;

	/* Steps forward or back round the list. */
	if (step >= 0) {
		moves = (size_t)step % count;
		segment->selected = (segment->selected + moves) % count;
	} else {
		moves = (size_t)(-step) % count;
		segment->selected = (segment->selected + count - moves) % count;
	}

	/* Each move counts towards opening the window. */
	segment->presses++;
}

/*
 * Chooses a candidate by its number on the window's page (0 for the
 * first).
 *
 * Returns false when the window is not open or the page has no such
 * candidate.
 */
bool
ja_core_select_on_page(
	struct ja_core *core,
	size_t index)
{
	struct ja_segment *segment;
	size_t target;

	/* Only a conversion has a window. */
	if (!core->converting || core->segment_count == 0U)
		return false;

	/* The window must be open for its numbers to be seen. */
	segment = &core->segments[core->focus];
	if (segment->presses < ENGINE_PRESSES_WINDOW)
		return false;

	/* The candidate on the page the selected one is on. */
	target = segment->selected - segment->selected % ENGINE_PAGE_SIZE + index;
	if (index >= ENGINE_PAGE_SIZE || target >= segment->candidate_count)
		return false;

	/* Succeeded: the candidate is chosen. */
	segment->selected = target;
	return true;
}

/*
 * Moves the focus to the segment before or after.
 */
void
ja_core_move_focus(
	struct ja_core *core,
	int step)
{
	/* Only a conversion has segments. */
	if (!core->converting)
		return;

	/* Stops at the first and the last segment. */
	if (step < 0) {
		if (core->focus > 0U)
			core->focus--;
	} else if (step > 0) {
		if (core->focus + 1U < core->segment_count)
			core->focus++;
	}
}

/*
 * Makes the segment in focus one character longer or shorter, and splits
 * what follows it again.
 */
void
ja_core_resize_focus(
	struct ja_core *core,
	int step)
{
	struct ja_segment *segment;
	struct ja_text *text;
	size_t new_end;

	/* Only a conversion has segments. */
	if (!core->converting || core->segment_count == 0U)
		return;

	/* The new end must leave the segment at least one character. */
	segment = &core->segments[core->focus];
	new_end = segment->end;
	if (step < 0) {
		if (new_end <= segment->start + 1U)
			return;
		new_end--;
	} else if (step > 0) {
		if (new_end >= core->unit_count)
			return;
		new_end++;
	} else {
		return;
	}

	/* Reads the composition as the segmenter does. */
	text = malloc(sizeof(*text));
	if (text == NULL)
		return;

	ja_text_build(text, core->units, core->unit_count);

	/* The segment in focus takes its new length, and the rest is split again. */
	engine_fill_segment(core, text, core->focus, segment->start, new_end);
	core->segment_count = engine_split_from(core, text, core->focus + 1U, new_end);
	free(text);
}

/*
 * Gives the segment in focus in one form (hiragana, katakana, half-width
 * katakana, or the letters typed in full or half width).  Called while
 * composing, it first makes the whole composition one segment.
 */
void
ja_core_apply_form(
	struct ja_core *core,
	enum ja_form form)
{
	struct ja_segment *segment;
	struct ja_text *text;
	char written[IME_CANDIDATE_MAX];
	size_t length;
	size_t i;
	bool added;
	bool same;

	/* A composition becomes one segment first. */
	if (!core->converting) {
		engine_flush_romaji(core);

		/* Nothing is composed to give a form to. */
		if (core->unit_count == 0U)
			return;

		text = malloc(sizeof(*text));
		if (text == NULL)
			return;

		ja_text_build(text, core->units, core->unit_count);
		engine_fill_segment(core, text, 0, 0, core->unit_count);
		free(text);
		core->segment_count = 1;
		core->focus = 0;
		core->converting = true;
		core->literal_latch = false;
	}

	/* Writes the segment in the form. */
	segment = &core->segments[core->focus];
	length = engine_form_text(core, segment, form, written, sizeof(written));
	if (length == 0U)
		return;

	/* Makes the form one of the candidates, and chooses it. */
	added = ja_segment_add_candidate(segment, written, length);
	if (!added)
		return;

	for (i = 0; i < segment->candidate_count; i++) {
		same = ja_bytes_equal(segment->candidates[i], strlen(segment->candidates[i]), written, length);
		if (same) {
			segment->selected = i;
			break;
		}
	}
}

/*
 * Commits what is being composed: the chosen candidates of a conversion
 * (teaching the user dictionary each choice), or the characters of a
 * composition.  The engine then rests with nothing composed.
 */
void
ja_core_commit(
	struct ja_core *core,
	struct ime_output *out)
{
	/* A conversion commits its chosen candidates; a composition its characters. */
	if (core->converting) {
		engine_commit_conversion(core, out);
	} else {
		engine_commit_composition(core, out);
	}

	/* The engine rests with nothing composed. */
	ja_core_clear(core);
}

/*
 * Fills an output with what the engine shows: the preedit and its cursor,
 * and the candidates of the segment in focus.
 */
void
ja_core_output(
	const struct ja_core *core,
	struct ime_output *out)
{
	size_t i;

	/* Starts from nothing shown. */
	out->preedit_length = 0;
	out->preedit[0] = '\0';
	out->cursor_begin = 0;
	out->cursor_end = 0;
	out->candidate_count = 0;
	out->candidate_selected = 0;
	out->candidates_shown = false;

	/* A conversion shows its chosen candidates and the window's state. */
	if (core->converting) {
		engine_output_conversion(core, out);
		return;
	}

	/* A composition shows its characters and the pending letters, the cursor after them. */
	for (i = 0; i < core->unit_count; i++)
		out->preedit_length += ja_utf8_encode(core->units[i].code, out->preedit + out->preedit_length);

	memcpy(out->preedit + out->preedit_length, core->romaji.pending, core->romaji.pending_length);
	out->preedit_length += core->romaji.pending_length;
	out->preedit[out->preedit_length] = '\0';
	out->cursor_begin = (int32_t)out->preedit_length;
	out->cursor_end = (int32_t)out->preedit_length;

	/* Something is being composed when the preedit is not empty. */
	out->composing = false;
	if (out->preedit_length != 0U)
		out->composing = true;
}

/*
 * Commits the chosen candidates of a conversion, and teaches the user
 * dictionary each segment's choice.
 */
static void
engine_commit_conversion(
	struct ja_core *core,
	struct ime_output *out)
{
	struct ja_text *text;
	struct ja_segment *segment;
	const char *chosen;
	size_t i;
	bool learned;
	int error;

	/* Reads the composition, for the readings the choices are learned under. */
	text = malloc(sizeof(*text));
	if (text != NULL)
		ja_text_build(text, core->units, core->unit_count);

	/* Commits each segment's choice in turn. */
	learned = false;
	for (i = 0; i < core->segment_count; i++) {
		segment = &core->segments[i];
		if (segment->candidate_count == 0U)
			continue;

		chosen = segment->candidates[segment->selected];
		(void)ime_output_append_commit(out, chosen, strlen(chosen));

		/*
		 * A segment with one candidate had no choice to learn; nothing is
		 * learned in a secret field, or without memory or a dictionary.
		 */
		if (!core->learning || text == NULL || segment->candidate_count < 2U || core->lexicon.user == NULL)
			continue;

		/* Puts the choice first for the segment's reading. */
		error = ja_user_learn(&core->user, text->bytes + text->offsets[segment->start],
				      text->offsets[segment->end] - text->offsets[segment->start], chosen);
		if (error == 0)
			learned = true;
	}

	free(text);

	/*
	 * The choices outlive the input method; the file is written by a
	 * thread of its own, so that a slow disk does not hold the commit
	 * (BUG-143).
	 */
	if (learned)
		(void)ja_user_save_later(&core->user);
}

/*
 * Commits the characters of a composition as they are.
 */
static void
engine_commit_composition(
	struct ja_core *core,
	struct ime_output *out)
{
	char bytes[8];
	size_t used;
	size_t i;

	/* A lone n and the other pending letters end first. */
	engine_flush_romaji(core);

	/* Writes each character. */
	for (i = 0; i < core->unit_count; i++) {
		used = ja_utf8_encode(core->units[i].code, bytes);
		(void)ime_output_append_commit(out, bytes, used);
	}
}

/*
 * Fills an output with a conversion: its chosen candidates with the
 * segment in focus as the cursor range, and that segment's candidates.
 */
static void
engine_output_conversion(
	const struct ja_core *core,
	struct ime_output *out)
{
	const struct ja_segment *segment;
	const char *candidate;
	size_t length;
	size_t i;

	/* Writes each segment's choice, as much as one message carries. */
	for (i = 0; i < core->segment_count; i++) {
		segment = &core->segments[i];
		if (segment->candidate_count == 0U)
			continue;

		candidate = segment->candidates[segment->selected];
		length = strlen(candidate);
		if (out->preedit_length + length >= IME_TEXT_MAX)
			break;

		/* The segment in focus is the cursor range. */
		if (i == core->focus) {
			out->cursor_begin = (int32_t)out->preedit_length;
			out->cursor_end = (int32_t)(out->preedit_length + length);
		}

		memcpy(out->preedit + out->preedit_length, candidate, length);
		out->preedit_length += length;
	}

	out->preedit[out->preedit_length] = '\0';

	/* The candidates of the segment in focus. */
	segment = &core->segments[core->focus];
	for (i = 0; i < segment->candidate_count && i < IME_CANDIDATES_MAX; i++)
		strcpy(out->candidates[i], segment->candidates[i]);

	out->candidate_count = segment->candidate_count;
	out->candidate_selected = segment->selected;

	/* The window opens once the next candidate has been asked for twice. */
	if (segment->presses >= ENGINE_PRESSES_WINDOW)
		out->candidates_shown = true;

	out->composing = true;
}

/*
 * Turns learning the user's choices on or off (off in a field whose text
 * is secret).
 */
void
ja_core_set_learning(
	struct ja_core *core,
	bool learning)
{
	/* Read by each commit. */
	core->learning = learning;
}

/*
 * Appends the characters of a romaji step to the composition.
 */
static bool
engine_append_units(
	struct ja_core *core,
	const struct ja_romaji_result *result)
{
	size_t i;

	/* The room was checked by the caller; guards against a step larger than it. */
	if (core->unit_count + result->count > JA_UNITS_MAX)
		return false;

	/* Copies each character. */
	for (i = 0; i < result->count; i++) {
		core->units[core->unit_count] = result->units[i];
		core->unit_count++;
	}

	/* Succeeded: the characters are in the composition. */
	return true;
}

/*
 * Ends the pending letters into the composition.
 */
static void
engine_flush_romaji(
	struct ja_core *core)
{
	struct ja_romaji_result result;

	/* A lone n becomes ん; other letters stay as typed. */
	result.count = 0;
	ja_romaji_flush(&core->romaji, &result);
	(void)engine_append_units(core, &result);
}

/*
 * Makes one segment over a span, with its candidates.
 */
static void
engine_fill_segment(
	struct ja_core *core,
	const struct ja_text *text,
	size_t index,
	size_t start,
	size_t end)
{
	struct ja_segment *segment;

	/* The span, then its candidates. */
	segment = &core->segments[index];
	segment->start = start;
	segment->end = end;
	ja_segment_candidates(&core->lexicon, text, segment);
}

/*
 * Splits the composition from a unit to its end into segments, written
 * from a segment index on.
 *
 * Returns the number of segments the conversion then has.
 */
static size_t
engine_split_from(
	struct ja_core *core,
	const struct ja_text *text,
	size_t first,
	size_t start)
{
	struct ja_span spans[JA_UNITS_MAX];
	size_t count;
	size_t i;

	/* Nothing is left after the start. */
	if (start >= core->unit_count)
		return first;

	/* Splits the rest and makes each span a segment. */
	count = ja_segment_split(&core->lexicon, text, start, spans, JA_UNITS_MAX - first);
	for (i = 0; i < count; i++)
		engine_fill_segment(core, text, first + i, spans[i].start, spans[i].end);

	/* The segments before the first, and the new ones. */
	return first + count;
}

/*
 * Writes a segment in a form.
 *
 * Returns the length written, 0 when it does not fit.
 */
static size_t
engine_form_text(
	const struct ja_core *core,
	const struct ja_segment *segment,
	enum ja_form form,
	char *out,
	size_t size)
{
	const struct ja_unit *unit;
	const char *half;
	size_t length;
	size_t i;
	size_t j;
	size_t half_length;
	uint32_t code;

	/* Writes each character of the span. */
	length = 0;
	for (i = segment->start; i < segment->end; i++) {
		unit = &core->units[i];

		/* Room for the longest thing one character becomes. */
		if (length + JA_RAW_MAX * 4U >= size)
			return 0;

		/* Each form writes the character its own way. */
		switch (form) {
		case JA_FORM_HIRAGANA:
			length += ja_utf8_encode(unit->code, out + length);
			break;
		case JA_FORM_KATAKANA:
			length += ja_utf8_encode(ja_to_katakana(unit->code), out + length);
			break;
		case JA_FORM_HALF_KATAKANA:
			half = ja_to_half_katakana(unit->code);
			if (half == NULL) {
				length += ja_utf8_encode(unit->code, out + length);
				break;
			}

			half_length = strlen(half);
			memcpy(out + length, half, half_length);
			length += half_length;
			break;
		case JA_FORM_FULL_ASCII:
			for (j = 0; unit->raw[j] != '\0'; j++) {
				code = ja_to_full_ascii((unsigned char)unit->raw[j]);
				length += ja_utf8_encode(code, out + length);
			}

			break;
		case JA_FORM_HALF_ASCII:
			for (j = 0; unit->raw[j] != '\0'; j++) {
				out[length] = unit->raw[j];
				length++;
			}

			break;
		default:
			break;
		}
	}

	/* Succeeded: the segment in the form, terminated. */
	out[length] = '\0';
	return length;
}

/*
 * Handles a key for the Wayland side.
 */
static void
engine_key(
	struct ime_engine *engine,
	const struct ime_key *key,
	struct ime_output *out)
{
	struct ja_core *core;

	core = engine->state;

	/* Starts from an empty output, lets the key act, then shows the state. */
	ime_output_clear(out);
	ja_keys_handle(core, key, out);
	ja_core_output(core, out);
}

/*
 * Ends the composition for the Wayland side, committing it or dropping it.
 */
static void
engine_reset(
	struct ime_engine *engine,
	bool commit,
	struct ime_output *out)
{
	struct ja_core *core;

	core = engine->state;

	/* Starts from an empty output. */
	ime_output_clear(out);

	/* Commits what is composed, or drops it. */
	if (commit) {
		ja_core_commit(core, out);
	} else {
		ja_core_clear(core);
	}

	ja_core_output(core, out);
}

/*
 * Takes the text around the cursor, which the Japanese engine does not use
 * yet.
 */
static void
engine_surrounding(
	struct ime_engine *engine,
	const char *text,
	uint32_t cursor,
	uint32_t anchor)
{
	UNUSED_PARAMETER(engine);
	UNUSED_PARAMETER(text);
	UNUSED_PARAMETER(cursor);
	UNUSED_PARAMETER(anchor);
}

/*
 * Takes what the field holds: in a field whose text is secret (a
 * password, a PIN, or text the application marks sensitive or hidden)
 * nothing is learned.
 */
static void
engine_content_type(
	struct ime_engine *engine,
	uint32_t hint,
	uint32_t purpose)
{
	struct ja_core *core;
	bool secret;

	core = engine->state;

	/* Any sign of a secret turns learning off. */
	secret = false;
	if ((hint & (IME_HINT_SENSITIVE_DATA | IME_HINT_HIDDEN_TEXT)) != 0U)
		secret = true;
	if (purpose == IME_PURPOSE_PASSWORD || purpose == IME_PURPOSE_PIN)
		secret = true;

	ja_core_set_learning(core, !secret);
}

/*
 * Frees the engine.
 */
static void
engine_destroy(
	struct ime_engine *engine)
{
	struct ja_core *core;

	core = engine->state;

	/* The state and its dictionaries. */
	ja_core_close(core);
	free(core);
	engine->state = NULL;
	engine->ops = NULL;
}
