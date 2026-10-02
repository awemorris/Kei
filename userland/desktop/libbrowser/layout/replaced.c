/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The size of a replaced box (CSS 2 §10.3.2 and §10.6.2, for <img>): the
 * width and height the style gives, or else the element's width and
 * height attributes (their presentational hints), and for what neither
 * gives the image's own size, or the other side through the image's ratio.
 * max-width and max-height shrink it keeping the ratio.  An element with
 * no image and no size is empty.
 */

#include "layout/layout.h"

static int replaced_attribute(const struct layout_box *box, const char *name, layout_unit *value);
static int replaced_length(const struct css_length *length, layout_unit containing, layout_unit *value);
static layout_unit replaced_content_size(const struct layout_box *box, layout_unit size, int first, int second);

/*
 * Sets a replaced box's content width and height within its containing block.
 *
 * The box model is resolved already; height_definite distinguishes zero from auto.
 */
void
layout_replaced_size(
	struct layout_box *box,
	layout_unit containing_width,
	layout_unit containing_height,
	int height_definite)
{
	layout_unit natural_width;
	layout_unit natural_height;
	layout_unit width;
	layout_unit height;
	layout_unit limit;
	int has_ratio;
	int width_given;
	int height_given;
	int limited;

	/* The image's own size, which gives the ratio too. */
	natural_width = 0;
	natural_height = 0;
	has_ratio = 0;
	if (box->image != NULL) {
		natural_width = (layout_unit)box->image->width * LAYOUT_UNIT;
		natural_height = (layout_unit)box->image->height * LAYOUT_UNIT;
		has_ratio = 1;
	}

	/* A form control's natural size is its text's, and has no ratio. */
	if (box->control != DOM_CONTROL_NONE) {
		natural_width = box->natural_width;
		natural_height = box->natural_height;
	}

	/* The width: the style's (less the frame when it sizes the border box), or the attribute's. */
	width = 0;
	width_given = replaced_length(&box->style.width, containing_width, &width);
	if (width_given)
		width = replaced_content_size(box, width, CSS_LEFT, CSS_RIGHT);

	/* Only an unresolved non-control width falls back to the real content attribute. */
	if (!width_given && box->control == DOM_CONTROL_NONE)
		width_given = replaced_attribute(box, "width", &width);

	/* The height: the style's (a percentage only with a definite containing height), or the attribute's. */
	height = 0;
	height_given = 0;
	if (box->style.height.unit == CSS_UNIT_PX ||
	    (box->style.height.unit == CSS_UNIT_PERCENT && height_definite))
		height_given = replaced_length(&box->style.height, containing_height, &height);
	if (height_given)
		height = replaced_content_size(box, height, CSS_TOP, CSS_BOTTOM);

	/* Only an unresolved non-control height falls back to the real content attribute. */
	if (!height_given && box->control == DOM_CONTROL_NONE)
		height_given = replaced_attribute(box, "height", &height);

	/* What is not given comes from the other side through the ratio, or from the image. */
	if (width_given && !height_given) {
		height = natural_height;
		if (has_ratio)
			height = (layout_unit)((int64_t)width * natural_height / natural_width);
	} else if (height_given && !width_given) {
		width = natural_width;
		if (has_ratio)
			width = (layout_unit)((int64_t)height * natural_width / natural_height);
	} else if (!width_given && !height_given) {
		width = natural_width;
		height = natural_height;
	}

	/* max-width shrinks a wider box, and its height with it when that was not given. */
	limited = replaced_length(&box->style.max_width, containing_width, &limit);
	if (limited && width > limit) {
		if (!height_given && width > 0)
			height = (layout_unit)((int64_t)height * limit / width);
		width = limit;
	}

	/* max-height shrinks a taller box, and its width with it when that was not given. */
	limited = 0;
	if (box->style.max_height.unit == CSS_UNIT_PX ||
	    (box->style.max_height.unit == CSS_UNIT_PERCENT && height_definite))
		limited = replaced_length(&box->style.max_height, containing_height, &limit);
	if (limited && height > limit) {
		if (!width_given && height > 0)
			width = (layout_unit)((int64_t)width * limit / height);
		height = limit;
	}

	/* No side is negative. */
	if (width < 0)
		width = 0;

	/* The final used height cannot be negative either. */
	if (height < 0)
		height = 0;

	/* Publishes both final used content dimensions after all limits were resolved. */
	box->width = width;
	box->height = height;

	/* Succeeded: the existing box now holds its resolved content dimensions. */
	return;
}

/* Reads leading attribute digits as pixels, reporting absence when no initial digit exists. */
static int
replaced_attribute(
	const struct layout_box *box,
	const char *name,
	layout_unit *value)
{
	const struct dom_element *element;
	const struct dom_attribute *attribute;
	const struct vm_string *text;
	uint16_t unit;
	size_t index;
	int64_t number;
	int same;

	/* The element's attribute of that name. */
	if (box->node == NULL || box->node->type != DOM_ELEMENT)
		return 0;

	/* Searches only actual no-namespace attributes on the native element. */
	element = (const struct dom_element *)box->node;
	attribute = NULL;
	for (index = 0; index < element->attribute_count; index++) {
		same = vm_string_equal_ascii(element->attributes[index].name, name);
		if (same && element->attributes[index].ns == DOM_NS_NONE) {
			attribute = &element->attributes[index];
			break;
		}
	}

	/* No attribute gives no size. */
	if (attribute == NULL)
		return 0;

	/* Spaces before the number are skipped. */
	text = attribute->value;
	index = 0;
	unit = 0;
	while (index < text->length) {
		unit = vm_string_at(text, index);
		if (unit != 0x20U &&
		    unit != 0x09U &&
		    unit != 0x0aU)
			break;
		index++;
	}

	/* A value that starts with no digit gives no size. */
	if (index >= text->length ||
	    unit < '0' ||
	    unit > '9')
		return 0;

	/* The digits, up to a size no page needs. */
	number = 0;
	while (index < text->length) {
		unit = vm_string_at(text, index);
		if (unit < '0' || unit > '9')
			break;

		/* Accumulates digits only while the existing bounded pixel cap still permits growth. */
		if (number < 1000000)
			number = number * 10 + (unit - '0');
		index++;
	}

	/* Succeeded: the attribute's pixels. */
	*value = (layout_unit)(number * LAYOUT_UNIT);
	return 1;
}

/* Resolves pixels or a percentage with a known containing size, leaving other units unresolved. */
static int
replaced_length(
	const struct css_length *length,
	layout_unit containing,
	layout_unit *value)
{
	/* Pixels. */
	if (length->unit == CSS_UNIT_PX) {
		*value = layout_from_px(length->value);
		return 1;
	}

	/* A definite containing size can be exactly zero; height definiteness is checked by the caller. */
	if (length->unit == CSS_UNIT_PERCENT && containing >= 0) {
		*value = (layout_unit)((float)containing * length->value / 100.0f) + layout_from_px(length->offset);
		return 1;
	}

	/* Succeeded: auto, none or an unresolved containing size supplies no explicit length. */
	return 0;
}

/* Removes both sides' borders and paddings from a border-box size without making it negative. */
static layout_unit
replaced_content_size(
	const struct layout_box *box,
	layout_unit size,
	int first,
	int second)
{
	layout_unit frame;

	/* A content-box size is the content's already. */
	if (box->style.box_sizing != CSS_BOX_SIZING_BORDER)
		return size;

	/* The frame of the two sides comes off. */
	frame = box->border[first] + box->padding[first] + box->padding[second] + box->border[second];
	size -= frame;
	if (size < 0)
		size = 0;

	/* Succeeded: reports the nonnegative used content dimension. */
	return size;
}
