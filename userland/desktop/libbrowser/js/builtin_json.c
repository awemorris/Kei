/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * JSON: parse (with a reviver) and stringify (with a replacer function or
 * property list, indentation, toJSON, and the error for a cycle).  The
 * parser is recursive descent over the string's code units; nesting deeper
 * than JSON_DEPTH_MAX is a RangeError rather than a C stack overflow.
 */

#include "js/builtin.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The deepest nesting JSON.parse and JSON.stringify follow. */
#define JSON_DEPTH_MAX 2000U

/* The longest numeral read. */
#define JSON_NUMERAL_MAX 400U

/*
 * One JSON.parse owns this stack state while following its coerced text.
 */
struct json_parser {
	struct vm_realm *realm;
	const struct vm_string *text;
	uint32_t position;
	unsigned depth;
};

/*
 * The state of one JSON.stringify: the replacer (a function or
 * undefined), the property list (an array or undefined), the indentation
 * unit and the current one, the objects being serialized (for cycles) and
 * the output. Its four C buffers are released before this call returns;
 * VM values follow ordinary realm and conservative native caller ownership.
 */
struct json_writer {
	struct vm_realm *realm;
	vm_value replacer;
	vm_value keys;
	struct wb_units gap;
	struct wb_units indent;
	struct wb_vector stack;
	struct wb_units out;
};

static int json_parse(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int json_stringify(struct vm_realm *realm, vm_value this_value, const vm_value *args, unsigned count, vm_value *result);
static int json_fail(struct json_parser *parser, const char *message);
static void json_skip_space(struct json_parser *parser);
static uint16_t json_peek(const struct json_parser *parser);
static int json_value(struct json_parser *parser, vm_value *value);
static int json_object(struct json_parser *parser, vm_value *value);
static int json_array(struct json_parser *parser, vm_value *value);
static int json_string(struct json_parser *parser, vm_value *value);
static int json_number(struct json_parser *parser, vm_value *value);
static int json_word(struct json_parser *parser, const char *word, vm_value word_value, vm_value *value);
static int json_internalize(struct vm_realm *realm, vm_value reviver, vm_value holder, vm_value key, vm_value *result, unsigned depth);
static int json_property(struct json_writer *writer, vm_value holder, vm_value key, int *written);
static int json_quote(struct json_writer *writer, const struct vm_string *string);
static int json_serialize_object(struct json_writer *writer, vm_value object);
static int json_serialize_array(struct json_writer *writer, vm_value array);
static int json_enter(struct json_writer *writer, vm_value object);
static int json_text(struct json_writer *writer, const char *text);
static int json_newline(struct json_writer *writer);
static int json_key_value(struct vm_realm *realm, vm_value key, vm_value *value);
static int json_property_list(struct json_writer *writer, vm_value replacer);
static int json_space(struct json_writer *writer, vm_value space);

/*
 * Installs JSON.
 */
int
js_builtin_install_json(
	struct vm_realm *realm)
{
	struct vm_object *json;
	int error;

	/* The object, a global, with its two functions. */
	json = vm_object_create(realm->heap, realm->object_prototype);
	if (json == NULL)
		return ENOMEM;

	/* Publishes the allocated object in the global namespace. */
	error = js_builtin_value(realm, realm->global, "JSON", vm_value_cell(json), JS_BUILTIN_METHOD);
	if (error != 0)
		return error;

	/* Publishes parsing only after the global object is installed. */
	error = js_builtin_method(realm, json, "parse", 2, json_parse);
	if (error != 0)
		return error;

	/* Adds serialization to the same published JSON object. */
	error = js_builtin_method(realm, json, "stringify", 3, json_stringify);
	if (error != 0)
		return error;

	/* Succeeded: JSON is installed. */
	return 0;
}

/* JSON.parse(text, reviver). */
static int
json_parse(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct json_parser parser;
	struct vm_string *text;
	struct vm_object *root;
	vm_value reviver;
	vm_value key;
	vm_value argument;
	int callable;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The text, then one value and nothing after it but blanks. */
	argument = js_argument(args, count, 0);
	status = vm_to_string(realm, argument, &text);
	if (status != 0)
		return status;

	/* Initializes the parser after text coercion succeeds. */
	memset(&parser, 0, sizeof(parser));
	parser.realm = realm;
	parser.text = text;
	json_skip_space(&parser);
	status = json_value(&parser, result);
	if (status != 0)
		return status;

	/* Checks for text following the complete parsed value. */
	json_skip_space(&parser);
	if (parser.position < text->length) {
		status = json_fail(&parser, "Unexpected text after JSON");
		if (status != 0)
			return status;

		/* Succeeded: the VM accepted the JSON exception report. */
		return 0;
	}

	/* A reviver walks the result from a holder { "": result }. */
	reviver = js_argument(args, count, 1);
	callable = vm_value_is_callable(reviver);
	if (!callable)
		return 0;
	root = vm_object_create(realm->heap, realm->object_prototype);
	if (root == NULL)
		return ENOMEM;

	/* Allocates the empty root key before interning it. */
	status = js_builtin_string(realm, "", &key);
	if (status != 0)
		return status;

	/* Interns the root holder key before defining the parsed result. */
	status = vm_key_from_string(realm->heap, (struct vm_string *)vm_value_as_cell(key), &key);
	if (status != 0)
		return status;

	/* The temporary holder supplies the reviver's root receiver. */
	status = vm_object_define(realm->heap, root, key, *result, VM_PROPERTY_DEFAULT);
	if (status != 0)
		return status;

	/* Revives the already parsed root through the actual callback. */
	status = json_internalize(realm, reviver, vm_value_cell(root), key, result, 0);
	if (status != 0)
		return status;

	/* Succeeded: the final revived root is available. */
	return 0;
}

/* JSON.stringify(value, replacer, space). */
static int
json_stringify(
	struct vm_realm *realm,
	vm_value this_value,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct json_writer writer;
	struct vm_object *wrapper;
	struct vm_string *text;
	vm_value key;
	vm_value argument;
	vm_value replacer;
	int written;
	int callable;
	int status;

	UNUSED_PARAMETER(this_value);

	/* The writer, the replacer or property list, and the gap. */
	memset(&writer, 0, sizeof(writer));
	writer.realm = realm;
	writer.replacer = VM_VALUE_UNDEFINED;
	writer.keys = VM_VALUE_UNDEFINED;
	wb_units_init(&writer.gap);
	wb_units_init(&writer.indent);
	wb_units_init(&writer.out);
	wb_vector_init(&writer.stack, sizeof(vm_value));
	replacer = js_argument(args, count, 1);
	callable = vm_value_is_callable(replacer);
	status = 0;
	if (callable) {
		writer.replacer = replacer;
	} else {
		status = json_property_list(&writer, replacer);
		if (status != 0)
			goto cleanup;
	}

	/* The optional gap is prepared after the replacer list has settled. */
	argument = js_argument(args, count, 2);
	status = json_space(&writer, argument);
	if (status != 0)
		goto cleanup;

	/* The value, as the "" property of a wrapper object. */
	wrapper = vm_object_create(realm->heap, realm->object_prototype);
	if (wrapper == NULL) {
		status = ENOMEM;
		goto cleanup;
	}

	/* The key "". */
	status = js_builtin_string(realm, "", &key);
	if (status != 0)
		goto cleanup;

	/* Interns the key before publishing the original value in its holder. */
	status = vm_key_from_string(realm->heap, (struct vm_string *)vm_value_as_cell(key), &key);
	if (status != 0)
		goto cleanup;

	/* The original argument remains unchanged until serialization starts. */
	argument = js_argument(args, count, 0);
	status = vm_object_define(realm->heap, wrapper, key, argument, VM_PROPERTY_DEFAULT);
	if (status != 0)
		goto cleanup;

	/* Serializes the holder property with its root key. */
	written = 0;
	status = json_property(&writer, vm_value_cell(wrapper), key, &written);
	if (status != 0)
		goto cleanup;

	/* The text, or undefined when the value has none. */
	*result = VM_VALUE_UNDEFINED;
	if (written) {
		text = vm_string_from_units(realm->heap, writer.out.data, writer.out.length);
		if (text == NULL) {
			status = ENOMEM;
			goto cleanup;
		}

		/* Publishes the complete allocated serialization. */
		*result = vm_value_cell(text);
	}

	/* The writer's buffers are no longer needed. */
cleanup:
	wb_units_release(&writer.gap);
	wb_units_release(&writer.indent);
	wb_units_release(&writer.out);
	wb_vector_release(&writer.stack);
	if (status != 0) {
		*result = VM_VALUE_UNDEFINED;
		return status;
	}

	/* Succeeded: complete text or the original undefined outcome is available. */
	return 0;
}

/* Throws the SyntaxError of a text that is not JSON. */
static int
json_fail(
	struct json_parser *parser,
	const char *message)
{
	char text[160];
	int printed;
	int status;

	/* The message with the place. */
	printed = snprintf(text, sizeof(text), "%s at position %u", message, parser->position);
	if (printed < 0)
		return EIO;

	/* Raises the formatted syntax diagnostic through the existing VM. */
	status = vm_throw_error(parser->realm, VM_ERROR_SYNTAX, text);
	if (status != 0)
		return status;

	/* Succeeded: the diagnostic was delivered to the VM exception state. */
	return 0;
}

/* Skips JSON's white space. */
static void
json_skip_space(
	struct json_parser *parser)
{
	uint16_t unit;

	/* Tab, line feed, carriage return and space. */
	while (parser->position < parser->text->length) {
		unit = vm_string_at(parser->text, parser->position);
		if (unit != 0x09U &&
		    unit != 0x0AU &&
		    unit != 0x0DU &&
		    unit != 0x20U)
			break;
		parser->position++;
	}

	/* Succeeded: the cursor names the next non-whitespace unit or the end. */
	return;
}

/* Reports the next code unit (0 at the end). */
static uint16_t
json_peek(
	const struct json_parser *parser)
{
	/* The end. */
	if (parser->position >= parser->text->length)
		return 0;

	/* The unit. */
	return vm_string_at(parser->text, parser->position);
}

/* Reads a value. */
static int
json_value(
	struct json_parser *parser,
	vm_value *value)
{
	uint16_t unit;
	int status;

	/* By its first character. */
	unit = json_peek(parser);
	switch (unit) {
	case '{':
		status = json_object(parser, value);
		if (status != 0)
			return status;
		break;
	case '[':
		status = json_array(parser, value);
		if (status != 0)
			return status;
		break;
	case '"':
		status = json_string(parser, value);
		if (status != 0)
			return status;
		break;
	case 't':
		status = json_word(parser, "true", VM_VALUE_TRUE, value);
		if (status != 0)
			return status;
		break;
	case 'f':
		status = json_word(parser, "false", VM_VALUE_FALSE, value);
		if (status != 0)
			return status;
		break;
	case 'n':
		status = json_word(parser, "null", VM_VALUE_NULL, value);
		if (status != 0)
			return status;
		break;
	default:
		if (unit == '-' ||
		    (unit >= '0' && unit <= '9')) {
			status = json_number(parser, value);
			if (status != 0)
				return status;
		} else {
			status = json_fail(parser, "Unexpected token in JSON");
			if (status != 0)
				return status;
		}

		/* The value is read. */
		break;
	}

	/* Reports whether it was read. */
	if (status != 0)
		return status;

	/* Succeeded: the VM accepted the JSON exception report. */
	return 0;
}

/* Reads an object. */
static int
json_object(
	struct json_parser *parser,
	vm_value *value)
{
	struct vm_object *object;
	vm_value name;
	vm_value key;
	vm_value member;
	uint16_t unit;
	int status;

	/* Too deep. */
	if (parser->depth >= JSON_DEPTH_MAX) {
		status = vm_throw_range_error(parser->realm, "JSON nested too deeply");
		if (status != 0)
			return status;

		/* Succeeded: the VM accepted the JSON exception report. */
		return 0;
	}

	/* The object. */
	object = vm_object_create(parser->realm->heap, parser->realm->object_prototype);
	if (object == NULL)
		return ENOMEM;

	/* Publishes the container before reading its recursive children. */
	*value = vm_value_cell(object);
	parser->position++;
	parser->depth++;
	json_skip_space(parser);
	unit = json_peek(parser);
	if (unit == '}') {
		parser->position++;
		parser->depth--;
		return 0;
	}

	/* Each "name": value, separated by commas. */
	for (;;) {
		json_skip_space(parser);
		unit = json_peek(parser);
		if (unit != '"') {
			status = json_fail(parser, "Expected a property name in JSON");
			if (status != 0)
				return status;

			/* Succeeded: the invalid input was reported through the VM. */
			return 0;
		}

		/* Reads the checked quoted property name. */
		status = json_string(parser, &name);
		if (status != 0)
			return status;
		json_skip_space(parser);
		unit = json_peek(parser);
		if (unit != ':') {
			status = json_fail(parser, "Expected ':' in JSON");
			if (status != 0)
				return status;

			/* Succeeded: the invalid input was reported through the VM. */
			return 0;
		}

		/* Advances beyond the checked syntax token. */
		parser->position++;
		json_skip_space(parser);
		status = json_value(parser, &member);
		if (status != 0)
			return status;

		/* Interns the parsed name as an object property key. */
		status = vm_key_from_string(parser->realm->heap, (struct vm_string *)vm_value_as_cell(name), &key);
		if (status != 0)
			return status;

		/* Defines the completed value using its resolved key. */
		status = vm_object_define(parser->realm->heap, object, key, member, VM_PROPERTY_DEFAULT);
		if (status != 0)
			return status;
		json_skip_space(parser);
		unit = json_peek(parser);
		if (unit == ',') {
			parser->position++;
			continue;
		}

		/* The end of the object. */
		unit = json_peek(parser);
		if (unit == '}') {
			parser->position++;
			parser->depth--;
			return 0;
		}

		/* Anything else is not JSON. */
		status = json_fail(parser, "Expected ',' or '}' in JSON");
		if (status != 0)
			return status;

		/* Succeeded: the invalid input was reported through the VM. */
		return 0;
	}
}

/* Reads an array. */
static int
json_array(
	struct json_parser *parser,
	vm_value *value)
{
	struct vm_object *array;
	vm_value member;
	uint16_t unit;
	int status;

	/* Too deep. */
	if (parser->depth >= JSON_DEPTH_MAX) {
		status = vm_throw_range_error(parser->realm, "JSON nested too deeply");
		if (status != 0)
			return status;

		/* Succeeded: the VM accepted the JSON exception report. */
		return 0;
	}

	/* The array. */
	array = vm_array_create(parser->realm->heap, parser->realm->array_prototype);
	if (array == NULL)
		return ENOMEM;

	/* Publishes the container before reading its recursive children. */
	*value = vm_value_cell(array);
	parser->position++;
	parser->depth++;
	json_skip_space(parser);
	unit = json_peek(parser);
	if (unit == ']') {
		parser->position++;
		parser->depth--;
		return 0;
	}

	/* Each value, separated by commas. */
	for (;;) {
		json_skip_space(parser);
		status = json_value(parser, &member);
		if (status != 0)
			return status;

		/* Defines the completed value using its resolved key. */
		status = vm_object_define(parser->realm->heap, array, vm_value_int32((int32_t)array->length), member, VM_PROPERTY_DEFAULT);
		if (status != 0)
			return status;
		json_skip_space(parser);
		unit = json_peek(parser);
		if (unit == ',') {
			parser->position++;
			continue;
		}

		/* The end of the array. */
		unit = json_peek(parser);
		if (unit == ']') {
			parser->position++;
			parser->depth--;
			return 0;
		}

		/* Anything else is not JSON. */
		status = json_fail(parser, "Expected ',' or ']' in JSON");
		if (status != 0)
			return status;

		/* Succeeded: the invalid input was reported through the VM. */
		return 0;
	}
}

/* Reads a string with its escapes. */
static int
json_string(
	struct json_parser *parser,
	vm_value *value)
{
	struct wb_units units;
	struct vm_string *string;
	uint16_t unit;
	uint16_t decoded;
	unsigned digit;
	int hex;
	int status;

	/* Past the quote, each unit up to the closing one. */
	parser->position++;
	wb_units_init(&units);
	status = 0;
	for (;;) {
		if (parser->position >= parser->text->length) {
			status = json_fail(parser, "Unterminated string in JSON");
			if (status != 0)
				goto cleanup;
			break;
		}

		/* The next unit. */
		unit = vm_string_at(parser->text, parser->position);
		parser->position++;
		if (unit == '"')
			break;
		if (unit < 0x20U) {
			status = json_fail(parser, "Bad control character in string literal in JSON");
			if (status != 0)
				goto cleanup;
			break;
		}

		/* A plain unit. */
		if (unit != '\\') {
			status = wb_units_append(&units, &unit, 1);
			if (status != 0)
				goto cleanup;
			continue;
		}

		/* An escape. */
		unit = json_peek(parser);
		parser->position++;
		decoded = 0;
		switch (unit) {
		case '"':
		case '\\':
		case '/':
			decoded = unit;
			break;
		case 'b':
			decoded = 0x08U;
			break;
		case 'f':
			decoded = 0x0CU;
			break;
		case 'n':
			decoded = 0x0AU;
			break;
		case 'r':
			decoded = 0x0DU;
			break;
		case 't':
			decoded = 0x09U;
			break;
		case 'u':
			for (digit = 0; digit < 4U; digit++) {
				unit = json_peek(parser);
				hex = -1;
				if (unit >= '0' && unit <= '9')
					hex = unit - '0';
				if ((unit | 0x20U) >= 'a' && (unit | 0x20U) <= 'f')
					hex = (int)((unit | 0x20U) - 'a') + 10;
				if (hex < 0)
					break;
				decoded = (uint16_t)(decoded * 16U + (unsigned)hex);
				parser->position++;
			}

			/* Four hex digits are needed. */
			if (digit < 4U) {
				status = json_fail(parser, "Bad Unicode escape in JSON");
				if (status != 0)
					goto cleanup;
			}

			break;
		default:
			status = json_fail(parser, "Bad escaped character in JSON");
			if (status != 0)
				goto cleanup;
			break;
		}

		/* The decoded unit. */
		if (status != 0)
			goto cleanup;

		/* Appends only a completely decoded escape. */
		status = wb_units_append(&units, &decoded, 1);
		if (status != 0)
			goto cleanup;
	}

	/* The string. */
	if (status == 0) {
		string = vm_string_from_units(parser->realm->heap, units.data, units.length);
		if (string == NULL) {
			status = ENOMEM;
			goto cleanup;
		}

		/* Publishes only the complete allocated string. */
		*value = vm_value_cell(string);
	}

	/* The list is no longer needed. */
cleanup:
	wb_units_release(&units);
	if (status != 0)
		return status;

	/* Succeeded: the decoded string is available and conversion storage is released. */
	return 0;
}

/* Reads a number: -?(0|[1-9][0-9]*)(.[0-9]+)?([eE][+-]?[0-9]+)?. */
static int
json_number(
	struct json_parser *parser,
	vm_value *value)
{
	char numeral[JSON_NUMERAL_MAX + 1U];
	size_t length;
	uint16_t unit;
	uint32_t digits;
	int negative;
	double number;
	int status;

	/* The sign. */
	length = 0;
	negative = 0;
	unit = json_peek(parser);
	if (unit == '-') {
		negative = 1;
		parser->position++;
	}

	/* The integer part: 0, or a digit other than 0 and more digits. */
	unit = json_peek(parser);
	if (unit < '0' || unit > '9') {
		status = json_fail(parser, "No number after minus sign in JSON");
		if (status != 0)
			return status;

		/* Succeeded: the invalid input was reported through the VM. */
		return 0;
	}

	/* Reads integer digits after the valid first character. */
	digits = 0;
	while (unit >= '0' && unit <= '9') {
		if (length < JSON_NUMERAL_MAX)
			numeral[length++] = (char)unit;
		parser->position++;
		digits++;
		if (unit == '0' && digits == 1U)
			break;
		unit = json_peek(parser);
	}

	/* A leading 0 is alone. */
	unit = json_peek(parser);
	if (digits == 1U &&
	    numeral[0] == '0' &&
	    unit >= '0' &&
	    unit <= '9') {
		status = json_fail(parser, "Unexpected number in JSON");
		if (status != 0)
			return status;

		/* Succeeded: the invalid input was reported through the VM. */
		return 0;
	}

	/* The fraction. */
	if (unit == '.') {
		parser->position++;
		if (length < JSON_NUMERAL_MAX)
			numeral[length++] = '.';
		unit = json_peek(parser);
		if (unit < '0' || unit > '9') {
			status = json_fail(parser, "Unterminated fractional number in JSON");
			if (status != 0)
				return status;

			/* Succeeded: the invalid input was reported through the VM. */
			return 0;
		}
		while (unit >= '0' && unit <= '9') {
			if (length < JSON_NUMERAL_MAX)
				numeral[length++] = (char)unit;
			parser->position++;
			unit = json_peek(parser);
		}
	}

	/* The exponent. */
	if (unit == 'e' || unit == 'E') {
		parser->position++;
		if (length < JSON_NUMERAL_MAX)
			numeral[length++] = 'e';
		unit = json_peek(parser);
		if (unit == '+' || unit == '-') {
			if (length < JSON_NUMERAL_MAX)
				numeral[length++] = (char)unit;
			parser->position++;
			unit = json_peek(parser);
		}

		/* The exponent needs digits. */
		if (unit < '0' || unit > '9') {
			status = json_fail(parser, "Exponent part is missing a number in JSON");
			if (status != 0)
				return status;

			/* Succeeded: the invalid input was reported through the VM. */
			return 0;
		}
		while (unit >= '0' && unit <= '9') {
			if (length < JSON_NUMERAL_MAX)
				numeral[length++] = (char)unit;
			parser->position++;
			unit = json_peek(parser);
		}
	}

	/* The number, read exactly. */
	number = vm_number_parse(numeral, length);
	if (negative)
		number = -number;
	*value = vm_value_number(number);
	return 0;
}

/* Reads true, false or null. */
static int
json_word(
	struct json_parser *parser,
	const char *word,
	vm_value word_value,
	vm_value *value)
{
	size_t index;
	uint16_t unit;
	int status;

	/* Each letter. */
	for (index = 0; word[index] != '\0'; index++) {
		unit = json_peek(parser);
		if (unit != (uint16_t)word[index]) {
			status = json_fail(parser, "Unexpected token in JSON");
			if (status != 0)
				return status;

			/* Succeeded: the invalid input was reported through the VM. */
			return 0;
		}

		/* Advances beyond the checked syntax token. */
		parser->position++;
	}

	/* The value. */
	*value = word_value;
	return 0;
}

/* Walks a parsed value with a reviver (InternalizeJSONProperty). */
static int
json_internalize(
	struct vm_realm *realm,
	vm_value reviver,
	vm_value holder,
	vm_value key,
	vm_value *result,
	unsigned depth)
{
	struct wb_vector keys;
	struct vm_descriptor descriptor;
	struct vm_object *object;
	vm_value value;
	vm_value element;
	vm_value name;
	vm_value call_args[2];
	vm_value ignored;
	uint32_t length;
	uint32_t index;
	size_t item;
	int is_object;
	int is_index;
	int is_string;
	int found;
	int done;
	int status;

	/* The value at the key. */
	if (depth > JSON_DEPTH_MAX) {
		status = vm_throw_range_error(realm, "JSON nested too deeply");
		if (status != 0)
			return status;

		/* Succeeded: the VM accepted the JSON exception report. */
		return 0;
	}

	/* The value at the key. */
	status = vm_get(realm, holder, key, &value);
	if (status != 0)
		return status;

	/* Prepares the one owned key list before any recursive member work. */
	wb_vector_init(&keys, sizeof(vm_value));

	/* An object's (or an array's) members first, each revived, removed when the reviver answers undefined. */
	is_object = vm_value_is_object(value);
	if (is_object) {
		object = (struct vm_object *)vm_value_as_cell(value);
		status = 0;
		if ((object->flags & VM_OBJECT_ARRAY) != 0U) {
			status = js_builtin_length(realm, value, &length);
			if (status != 0)
				goto cleanup;

			/* Snapshots each array index before callbacks start. */
			for (index = 0; index < length; index++) {
				element = vm_value_int32((int32_t)index);
				status = wb_vector_push(&keys, &element);
				if (status != 0)
					goto cleanup;
			}
		} else {
			status = vm_object_own_keys(realm->heap, object, &keys);
			if (status != 0)
				goto cleanup;
		}

		/* Each member revived. */
		for (item = 0; item < keys.count; item++) {
			element = *(vm_value *)wb_vector_at(&keys, item);
			is_index = vm_value_is_int32(element);
			is_string = vm_value_is_string(element);
			if (!is_index && !is_string)
				continue;
			if ((object->flags & VM_OBJECT_ARRAY) == 0U) {
				found = vm_get_own_descriptor(object, element, &descriptor);
				if (found < 0) {
					status = -found;
					goto cleanup;
				}

				/* A missing descriptor follows the absence path after errors have been excluded. */
				if (!found || (descriptor.attributes & VM_PROPERTY_ENUMERABLE) == 0U)
					continue;
			}

			/* The member revived. */
			status = json_internalize(realm, reviver, value, element, &name, depth + 1U);
			if (status != 0)
				goto cleanup;
			if (name == VM_VALUE_UNDEFINED) {
				status = vm_delete(realm, value, element, 0, &ignored);
				if (status != 0)
					goto cleanup;
			} else {
				memset(&descriptor, 0, sizeof(descriptor));
				descriptor.has = VM_HAS_VALUE | VM_HAS_WRITABLE | VM_HAS_ENUMERABLE | VM_HAS_CONFIGURABLE;
				descriptor.attributes = VM_PROPERTY_DEFAULT;
				descriptor.value = name;
				status = vm_define_own_property(realm, object, element, &descriptor, &done);
				if (status != 0)
					goto cleanup;
			}
		}
	}

	/* The snapshot is released before the enclosing value callback. */
cleanup:
	wb_vector_release(&keys);
	if (status != 0)
		return status;

	/* The reviver on the value, with the key as a string. */
	status = json_key_value(realm, key, &call_args[0]);
	if (status != 0)
		return status;
	call_args[1] = value;
	status = vm_call(realm, reviver, holder, call_args, 2, result);
	if (status != 0)
		return status;

	/* Succeeded: the reviver returned the enclosing value. */
	return 0;
}

/* Writes one property of a holder (SerializeJSONProperty); written says whether it had a text. */
static int
json_property(
	struct json_writer *writer,
	vm_value holder,
	vm_value key,
	int *written)
{
	struct vm_realm *realm;
	struct vm_object *object;
	struct vm_string *string;
	vm_value value;
	vm_value method;
	vm_value method_key;
	vm_value call_args[2];
	vm_value key_value;
	double number;
	int is_object;
	int is_string;
	int is_number;
	int infinite;
	int callable;
	int status;

	/* The value. */
	realm = writer->realm;
	*written = 0;
	status = vm_get(realm, holder, key, &value);
	if (status != 0)
		return status;

	/* toJSON, then the replacer. */
	is_object = vm_value_is_object(value);
	if (is_object) {
		method_key = vm_key_from_ascii(realm->heap, "toJSON");
		if (method_key == VM_VALUE_EMPTY)
			return ENOMEM;

		/* Reads the callback only after its key allocation is checked. */
		status = vm_get(realm, value, method_key, &method);
		if (status != 0)
			return status;
		callable = vm_value_is_callable(method);
		if (callable) {
			status = json_key_value(realm, key, &key_value);
			if (status != 0)
				return status;

			/* Invokes the actual callback after its arguments are ready. */
			status = vm_call(realm, method, value, &key_value, 1, &value);
			if (status != 0)
				return status;
		}
	}

	/* The replacer. */
	if (writer->replacer != VM_VALUE_UNDEFINED) {
		status = json_key_value(realm, key, &call_args[0]);
		if (status != 0)
			return status;

		/* Calls the replacer after its string key is ready. */
		call_args[1] = value;

		/* Invokes the actual callback after its arguments are ready. */
		status = vm_call(realm, writer->replacer, holder, call_args, 2, &value);
		if (status != 0)
			return status;
	}

	/* A wrapper stands for its primitive. */
	is_object = vm_value_is_object(value);
	if (is_object) {
		object = (struct vm_object *)vm_value_as_cell(value);
		if (object->kind == VM_KIND_NUMBER) {
			status = vm_to_number(realm, value, &number);
			if (status != 0)
				return status;
			value = vm_value_number(number);
		} else if (object->kind == VM_KIND_STRING) {
			status = vm_to_string(realm, value, &string);
			if (status != 0)
				return status;
			value = vm_value_cell(string);
		} else if (object->kind == VM_KIND_BOOLEAN) {
			value = object->internal;
		}
	}

	/* The kinds of value. */
	*written = 1;
	if (value == VM_VALUE_NULL) {
		status = json_text(writer, "null");
		if (status != 0)
			return status;

		/* Succeeded: the value has its complete JSON text. */
		return 0;
	}

	/* true and false. */
	if (value == VM_VALUE_TRUE) {
		status = json_text(writer, "true");
		if (status != 0)
			return status;

		/* Succeeded: the value has its complete JSON text. */
		return 0;
	}

	/* false. */
	if (value == VM_VALUE_FALSE) {
		status = json_text(writer, "false");
		if (status != 0)
			return status;

		/* Succeeded: the value has its complete JSON text. */
		return 0;
	}

	/* A string. */
	is_string = vm_value_is_string(value);
	if (is_string) {
		status = json_quote(writer, (struct vm_string *)vm_value_as_cell(value));
		if (status != 0)
			return status;

		/* Succeeded: the value has its complete JSON text. */
		return 0;
	}

	/* A number. */
	is_number = vm_value_is_number(value);
	if (is_number) {
		number = vm_value_as_number(value);
		infinite = isinf(number);
		if (number != number || infinite) {
			status = json_text(writer, "null");
			if (status != 0)
				return status;

			/* Succeeded: the value has its complete JSON text. */
			return 0;
		}

		/* Its numeral. */
		status = vm_to_string(realm, value, &string);
		if (status != 0)
			return status;

		/* Appends the completed number representation to the output. */
		status = vm_string_append_units(string, &writer->out);
		if (status != 0)
			return status;

		/* Succeeded: the value has its complete JSON text. */
		return 0;
	}

	/* An object that is not a function. */
	is_object = vm_value_is_object(value);
	callable = vm_value_is_callable(value);
	if (is_object && !callable) {
		object = (struct vm_object *)vm_value_as_cell(value);
		if ((object->flags & VM_OBJECT_ARRAY) != 0U)
			status = json_serialize_array(writer, value);
		else
			status = json_serialize_object(writer, value);
		if (status != 0)
			return status;

		/* Succeeded: the value has its complete JSON text. */
		return 0;
	}

	/* undefined, a function, a symbol: no text. */
	*written = 0;
	return 0;
}

/* Writes a string in quotes with JSON's escapes (a lone surrogate as \u). */
static int
json_quote(
	struct json_writer *writer,
	const struct vm_string *string)
{
	char escape[8];
	uint32_t index;
	uint16_t unit;
	uint16_t next;
	int lone;
	int printed;
	int status;

	/* The opening quote. */
	status = json_text(writer, "\"");
	if (status != 0)
		return status;

	/* Each unit. */
	for (index = 0; index < string->length; index++) {
		unit = vm_string_at(string, index);
		escape[0] = '\0';
		if (unit == '"') {
			strcpy(escape, "\\\"");
		} else if (unit == '\\') {
			strcpy(escape, "\\\\");
		} else if (unit == 0x08U) {
			strcpy(escape, "\\b");
		} else if (unit == 0x0CU) {
			strcpy(escape, "\\f");
		} else if (unit == 0x0AU) {
			strcpy(escape, "\\n");
		} else if (unit == 0x0DU) {
			strcpy(escape, "\\r");
		} else if (unit == 0x09U) {
			strcpy(escape, "\\t");
		} else if (unit < 0x20U) {
			printed = snprintf(escape, sizeof(escape), "\\u%04x", unit);
			if (printed < 0)
				return EIO;
		}

		/* A surrogate without its partner. */
		lone = 0;
		if (unit >= 0xD800U && unit <= 0xDBFFU) {
			next = 0;
			if (index + 1U < string->length)
				next = vm_string_at(string, index + 1U);
			if (next < 0xDC00U || next > 0xDFFFU) {
				lone = 1;
			} else {
				status = wb_units_append(&writer->out, &unit, 1);
				if (status != 0)
					return status;

				/* Writes the paired low surrogate after its lead unit succeeds. */
				status = wb_units_append(&writer->out, &next, 1);
				if (status != 0)
					return status;

				/* Both original units have now been consumed. */
				index++;
				continue;
			}
		} else if (unit >= 0xDC00U && unit <= 0xDFFFU) {
			lone = 1;
		}

		/* A lone surrogate's escape. */
		if (lone) {
			printed = snprintf(escape, sizeof(escape), "\\u%04x", unit);
			if (printed < 0)
				return EIO;
		}

		/* The escape, or the unit itself. */
		if (escape[0] != '\0') {
			status = json_text(writer, escape);
			if (status != 0)
				return status;
		} else {
			status = wb_units_append(&writer->out, &unit, 1);
			if (status != 0)
				return status;
		}
	}

	/* The closing quote. */
	status = json_text(writer, "\"");
	if (status != 0)
		return status;

	/* Succeeded: the quoted string and all escapes are written. */
	return 0;
}

/* Writes an object's properties (the property list's, or its own enumerable string keys). */
static int
json_serialize_object(
	struct json_writer *writer,
	vm_value object)
{
	struct wb_vector keys;
	struct vm_descriptor descriptor;
	struct vm_object *source;
	size_t mark;
	size_t item;
	size_t indent_length;
	uint32_t length;
	uint32_t index;
	vm_value key;
	vm_value name;
	const char *colon;
	int first;
	int found;
	int written;
	int is_index;
	int is_string;
	int status;

	/* A cycle is an error. */
	status = json_enter(writer, object);
	if (status != 0)
		return status;

	/* The keys. */
	wb_vector_init(&keys, sizeof(vm_value));
	indent_length = writer->indent.length;
	source = (struct vm_object *)vm_value_as_cell(object);
	if (writer->keys != VM_VALUE_UNDEFINED) {
		status = js_builtin_length(writer->realm, writer->keys, &length);
		if (status != 0)
			goto cleanup;

		/* Snapshots each requested replacer property before output starts. */
		for (index = 0; index < length; index++) {
			status = vm_get(writer->realm, writer->keys, vm_value_int32((int32_t)index), &name);
			if (status != 0)
				goto cleanup;

			/* Resolves the replacer name as a native property key. */
			status = vm_to_key(writer->realm, name, &key);
			if (status != 0)
				goto cleanup;

			/* Adds the resolved key to the owned snapshot. */
			status = wb_vector_push(&keys, &key);
			if (status != 0)
				goto cleanup;
		}
	} else {
		status = vm_object_own_keys(writer->realm->heap, source, &keys);
		if (status != 0)
			goto cleanup;
	}

	/* Each property with a text: "key": value (a space after the colon when indenting). */
	colon = ":";
	if (writer->gap.length > 0)
		colon = ": ";

	/* Extends indentation by the configured gap. */
	status = wb_units_append(&writer->indent, writer->gap.data, writer->gap.length);
	if (status != 0)
		goto cleanup;

	/* Writes the opening object delimiter. */
	status = json_text(writer, "{");
	if (status != 0)
		goto cleanup;

	/* The first written member omits its leading separator. */
	first = 1;
	for (item = 0; item < keys.count; item++) {
		key = *(vm_value *)wb_vector_at(&keys, item);
		is_index = vm_value_is_int32(key);
		is_string = vm_value_is_string(key);
		if (!is_index && !is_string)
			continue;
		if (writer->keys == VM_VALUE_UNDEFINED) {
			found = vm_get_own_descriptor(source, key, &descriptor);
			if (found < 0) {
				status = -found;
				goto cleanup;
			}

			/* A missing descriptor follows the absence path after errors have been excluded. */
			if (!found || (descriptor.attributes & VM_PROPERTY_ENUMERABLE) == 0U)
				continue;
		}

		/* Where the member starts, to take it back when it has no text. */
		mark = writer->out.length;
		if (!first)
			status = json_text(writer, ",");
		if (status != 0)
			goto cleanup;

		/* Starts the member at the current indentation. */
		status = json_newline(writer);
		if (status != 0)
			goto cleanup;

		/* Converts the property key to its JSON name. */
		status = json_key_value(writer->realm, key, &name);
		if (status != 0)
			goto cleanup;

		/* Quotes the completed property name. */
		status = json_quote(writer, (struct vm_string *)vm_value_as_cell(name));
		if (status != 0)
			goto cleanup;

		/* Separates the property name from its serialized value. */
		status = json_text(writer, colon);
		if (status != 0)
			goto cleanup;

		/* Serializes the actual holder property. */
		status = json_property(writer, object, key, &written);
		if (status != 0)
			goto cleanup;

		/* Removes speculative key text only when the value has no JSON text. */
		if (!written) {
			writer->out.length = mark;
			continue;
		}

		/* The next member follows a comma. */
		first = 0;
	}

	/* The indentation back, and the end. */
	writer->indent.length = indent_length;
	if (!first) {
		status = json_newline(writer);
		if (status != 0)
			goto cleanup;
	}

	/* Writes the closing object delimiter. */
	status = json_text(writer, "}");
	if (status != 0)
		goto cleanup;

	/* Restores indentation and releases the one key snapshot on either outcome. */
cleanup:
	writer->indent.length = indent_length;
	wb_vector_release(&keys);
	if (status == 0)
		writer->stack.count--;
	if (status != 0)
		return status;

	/* Succeeded: the object output is closed and its key snapshot released. */
	return 0;
}

/* Writes an array's elements (a missing text is null). */
static int
json_serialize_array(
	struct json_writer *writer,
	vm_value array)
{
	size_t indent_length;
	uint32_t length;
	uint32_t index;
	int written;
	int status;

	/* A cycle is an error. */
	status = json_enter(writer, array);
	if (status != 0)
		return status;

	/* Each element on its line. */
	indent_length = writer->indent.length;
	status = js_builtin_length(writer->realm, array, &length);
	if (status != 0)
		goto cleanup;

	/* Extends indentation by the configured gap. */
	status = wb_units_append(&writer->indent, writer->gap.data, writer->gap.length);
	if (status != 0)
		goto cleanup;

	/* Writes the opening array delimiter. */
	status = json_text(writer, "[");
	if (status != 0)
		goto cleanup;

	/* Writes each snapshotted array position in order. */
	for (index = 0; index < length; index++) {
		if (index > 0)
			status = json_text(writer, ",");
		if (status != 0)
			goto cleanup;

		/* Starts the member at the current indentation. */
		status = json_newline(writer);
		if (status != 0)
			goto cleanup;

		/* Serializes the actual holder property. */
		status = json_property(writer, array, vm_value_int32((int32_t)index), &written);
		if (status != 0)
			goto cleanup;

		/* Array positions without JSON text retain their null placeholder. */
		if (!written) {
			status = json_text(writer, "null");
			if (status != 0)
				goto cleanup;
		}
	}

	/* The indentation back, and the end. */
	writer->indent.length = indent_length;
	if (length > 0) {
		status = json_newline(writer);
		if (status != 0)
			goto cleanup;
	}

	/* Writes the closing array delimiter. */
	status = json_text(writer, "]");
	if (status != 0)
		goto cleanup;

	/* Restores caller indentation; only successful objects leave the cycle stack. */
cleanup:
	writer->indent.length = indent_length;
	if (status == 0)
		writer->stack.count--;
	if (status != 0)
		return status;

	/* Succeeded: all array positions and its closing delimiter are written. */
	return 0;
}

/* Enters an object, throwing a TypeError when it is already being written (a cycle). */
static int
json_enter(
	struct json_writer *writer,
	vm_value object)
{
	vm_value entered;
	size_t index;
	int status;

	/* The objects being written. */
	for (index = 0; index < writer->stack.count; index++) {
		entered = *(vm_value *)wb_vector_at(&writer->stack, index);
		if (entered == object) {
			status = vm_throw_type_error(writer->realm, "Converting circular structure to JSON");
			if (status != 0)
				return status;

			/* Succeeded: the VM accepted the JSON exception report. */
			return 0;
		}
	}

	/* Too deep. */
	if (writer->stack.count >= JSON_DEPTH_MAX) {
		status = vm_throw_range_error(writer->realm, "JSON nested too deeply");
		if (status != 0)
			return status;

		/* Succeeded: the VM accepted the JSON exception report. */
		return 0;
	}

	/* This one too. */
	status = wb_vector_push(&writer->stack, &object);
	if (status != 0)
		return status;

	/* Succeeded: the current object is registered for cycle detection. */
	return 0;
}

/* Appends ASCII text to the output. */
static int
json_text(
	struct json_writer *writer,
	const char *text)
{
	uint16_t unit;
	int status;

	/* Each character. */
	status = 0;
	for (; *text != '\0'; text++) {
		unit = (uint16_t)(unsigned char)*text;
		status = wb_units_append(&writer->out, &unit, 1);
		if (status != 0)
			return status;
	}

	/* Reports whether it was pushed. */
	if (status != 0)
		return status;

	/* Succeeded: all ASCII units are appended. */
	return 0;
}

/* Starts a new line at the current indentation (nothing without a gap). */
static int
json_newline(
	struct json_writer *writer)
{
	int status;

	/* Without a gap, the text stays on one line. */
	if (writer->gap.length == 0)
		return 0;

	/* The line feed and the indentation. */
	status = json_text(writer, "\n");
	if (status != 0)
		return status;

	/* Appends the current indentation to the new line. */
	status = wb_units_append(&writer->out, writer->indent.data, writer->indent.length);
	if (status != 0)
		return status;

	/* Succeeded: the new line and current indentation are appended. */
	return 0;
}

/* Makes the string of a property key (an index as its numeral). */
static int
json_key_value(
	struct vm_realm *realm,
	vm_value key,
	vm_value *value)
{
	struct vm_string *string;
	int is_index;
	int status;

	/* A string is itself; an index its numeral. */
	is_index = vm_value_is_int32(key);
	if (!is_index) {
		*value = key;
		return 0;
	}

	/* An index's numeral. */
	status = vm_to_string(realm, key, &string);
	if (status != 0)
		return status;
	*value = vm_value_cell(string);
	return 0;
}

/* Makes the property list from an array replacer (strings and numbers, and their wrappers, each once). */
static int
json_property_list(
	struct json_writer *writer,
	vm_value replacer)
{
	struct vm_object *object;
	struct vm_object *list;
	struct vm_string *string;
	vm_value element;
	vm_value key;
	vm_value listed;
	uint32_t length;
	uint32_t index;
	uint32_t other;
	int is_object;
	int is_string;
	int is_number;
	int duplicate;
	int same;
	int status;

	/* Only an array. */
	is_object = vm_value_is_object(replacer);
	if (!is_object)
		return 0;
	object = (struct vm_object *)vm_value_as_cell(replacer);
	if ((object->flags & VM_OBJECT_ARRAY) == 0U)
		return 0;

	/* Each usable element as a string, once. */
	list = vm_array_create(writer->realm->heap, writer->realm->array_prototype);
	if (list == NULL)
		return ENOMEM;

	/* Retains the native property list in the active writer. */
	writer->keys = vm_value_cell(list);
	status = js_builtin_length(writer->realm, replacer, &length);
	if (status != 0)
		return status;

	/* Coerces and deduplicates the snapshot of requested indices. */
	for (index = 0; index < length; index++) {
		status = vm_get(writer->realm, replacer, vm_value_int32((int32_t)index), &element);
		if (status != 0)
			return status;
		is_object = vm_value_is_object(element);
		if (is_object) {
			object = (struct vm_object *)vm_value_as_cell(element);
			if (object->kind != VM_KIND_STRING && object->kind != VM_KIND_NUMBER)
				continue;
		} else {
			is_string = vm_value_is_string(element);
			is_number = vm_value_is_number(element);
			if (!is_string && !is_number)
				continue;
		}

		/* As a string. */
		status = vm_to_string(writer->realm, element, &string);
		if (status != 0)
			return status;
		duplicate = 0;
		for (other = 0; other < list->length; other++) {
			listed = list->elements[other];
			same = vm_string_equal((struct vm_string *)vm_value_as_cell(listed), string);
			if (same)
				duplicate = 1;
		}

		/* A name listed already is skipped. */
		if (duplicate)
			continue;
		key = vm_value_cell(string);
		status = vm_object_define(writer->realm->heap, list, vm_value_int32((int32_t)list->length), key, VM_PROPERTY_DEFAULT);
		if (status != 0)
			return status;
	}

	/* Reports whether the list was made. */
	if (status != 0)
		return status;

	/* Succeeded: the deduplicated property list is retained in the writer. */
	return 0;
}

/* Makes the gap from the space argument: up to 10 spaces for a number, the first 10 units of a string. */
static int
json_space(
	struct json_writer *writer,
	vm_value space)
{
	struct vm_object *object;
	struct vm_string *string;
	double number;
	uint32_t index;
	uint16_t unit;
	int is_object;
	int is_number;
	int is_string;
	int status;

	/* A wrapper stands for its primitive. */
	is_object = vm_value_is_object(space);
	if (is_object) {
		object = (struct vm_object *)vm_value_as_cell(space);
		if (object->kind == VM_KIND_NUMBER) {
			status = vm_to_number(writer->realm, space, &number);
			if (status != 0)
				return status;
			space = vm_value_number(number);
		} else if (object->kind == VM_KIND_STRING) {
			status = vm_to_string(writer->realm, space, &string);
			if (status != 0)
				return status;
			space = vm_value_cell(string);
		}
	}

	/* A number: that many spaces (at most 10). */
	is_number = vm_value_is_number(space);
	if (is_number) {
		status = js_builtin_integer(writer->realm, space, &number);
		if (status != 0)
			return status;
		if (number > 10.0)
			number = 10.0;
		unit = ' ';
		for (index = 0; (double)index < number; index++) {
			status = wb_units_append(&writer->gap, &unit, 1);
			if (status != 0)
				return status;
		}

		/* The spaces. */
		return 0;
	}

	/* A string: its first 10 units. */
	is_string = vm_value_is_string(space);
	if (is_string) {
		string = (struct vm_string *)vm_value_as_cell(space);
		for (index = 0; index < string->length && index < 10U; index++) {
			unit = vm_string_at(string, index);
			status = wb_units_append(&writer->gap, &unit, 1);
			if (status != 0)
				return status;
		}
	}

	/* Anything else: no gap. */
	return 0;
}
