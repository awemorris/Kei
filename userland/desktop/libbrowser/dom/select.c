/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Select option membership follows current ordinary DOM ancestry without cached script state. */

#include "dom/dom.h"

static int select_html(const struct dom_node *node, int tag);
static struct dom_node *select_ancestor(struct dom_node *node);
static int select_disabled(struct dom_node *node);
static int select_display_size(struct dom_node *select);
static void select_prefer(struct dom_node *select, struct dom_node *preferred);
static struct dom_node *select_subtree_next(struct dom_node *root, struct dom_node *node);
static struct dom_node *select_tree_preferred(struct dom_node *root, struct dom_node *owner);

/*
 * Finds the actual select owning an option in the current ordinary tree.
 */
struct dom_node *
dom_option_select(
	struct dom_node *option)
{
	struct dom_node *ancestor;
	int grouped;
	int actual;

	/* Only exact HTML options supply select membership. */
	actual = select_html(option, DOM_TAG_OPTION);
	if (!actual)
		return NULL;

	/* Blocking option containers and multiple optgroups terminate the owner search. */
	grouped = 0;
	ancestor = option->parent;
	while (ancestor != NULL) {
		actual = select_html(ancestor, DOM_TAG_DATALIST);
		if (actual)
			return NULL;
		actual = select_html(ancestor, DOM_TAG_HR);
		if (actual)
			return NULL;
		actual = select_html(ancestor, DOM_TAG_OPTION);
		if (actual)
			return NULL;
		actual = select_html(ancestor, DOM_TAG_OPTGROUP);
		if (actual) {
			/* A second group does not belong to the option list of an outer select. */
			if (grouped)
				return NULL;
			grouped = 1;
		}

		/* The first actual select is the option's current native owner. */
		actual = select_html(ancestor, DOM_TAG_SELECT);
		if (actual)
			return ancestor;
		ancestor = ancestor->parent;
	}

	/* Detached or blocked options have no owning select. */
	return NULL;
}

/*
 * Advances through the current native select option list in tree order.
 */
struct dom_node *
dom_select_option_next(
	struct dom_node *select,
	struct dom_node *previous)
{
	struct dom_node *node;
	struct dom_node *owner;

	/* Start at the actual root or the last member without retaining a snapshot. */
	node = previous;
	if (node == NULL)
		node = select;

	/* Walk ordinary descendants and return only options currently owned by this select. */
	while (node != NULL) {
		if (node->first_child != NULL) {
			node = node->first_child;
		} else {
			/* Climb to the next subtree without leaving the requested select. */
			while (node != select && node->next == NULL)
				node = node->parent;

			/* The root's next sibling belongs to a different option list. */
			if (node == select)
				return NULL;
			node = node->next;
		}

		/* Derive membership from the complete current ancestor chain. */
		owner = dom_option_select(node);
		if (owner == select)
			return node;
	}

	/* Exhausted: no eligible option follows the supplied member. */
	return NULL;
}

/*
 * Normalizes the owned option states of a single select without invoking script.
 */
void
dom_select_reset(
	struct dom_node *select)
{
	struct dom_node *node;
	struct dom_element *option;
	struct dom_element *last;
	struct dom_element *first;
	struct vm_string *attribute;
	int disabled;
	int size;

	/* Optional detached owners and multiple-select lists need no single-selection fallback. */
	if (select == NULL)
		return;
	attribute = dom_attribute_ascii((struct dom_element *)select, "multiple");
	if (attribute != NULL)
		return;

	/* Keep the last selected member and remember the first enabled fallback candidate. */
	last = NULL;
	first = NULL;
	node = dom_select_option_next(select, NULL);
	while (node != NULL) {
		option = (struct dom_element *)node;
		if (option->option_selected) {
			/* Later selected members take precedence during ordinary normalization. */
			if (last != NULL)
				last->option_selected = 0;
			last = option;
		}

		/* Disabled options remain selected when explicit, but never supply automatic fallback. */
		if (first == NULL) {
			disabled = select_disabled(node);
			if (!disabled)
				first = option;
		}

		/* Membership always comes from the current actual option owner algorithm. */
		node = dom_select_option_next(select, node);
	}

	/* Explicit list-box display sizes allow a list to have no selected option. */
	size = select_display_size(select);
	if (last == NULL && first != NULL && size == 1)
		first->option_selected = 1;
}

/*
 * Updates an option's owned selectedness and dirtiness without altering its default attribute.
 */
void
dom_option_set_selected(
	struct dom_element *option,
	int selected,
	int dirty)
{
	struct dom_node *owner;

	/* Only exact native options participate in this allocation-free state transition. */
	selected = !!selected;
	option->option_selected = selected;
	if (dirty)
		option->option_dirty = 1;
	owner = dom_option_select(&option->node);

	/* A new single-select selection clears peers without dirtying their default reflection. */
	if (selected)
		select_prefer(owner, &option->node);
	dom_select_reset(owner);
	option->node.document->generation++;
}

/*
 * Reconciles current option owners after a complete subtree insertion or removal.
 */
void
dom_select_tree_changed(
	struct dom_node *root)
{
	struct dom_node *node;
	struct dom_node *owner;
	struct dom_node *old;
	struct dom_node *preferred;
	struct dom_element *option;
	int actual;

	/* The linked tree is already consistent; normalization cannot allocate or trigger GC. */
	node = root;
	while (node != NULL) {
		actual = select_html(node, DOM_TAG_OPTION);
		if (actual) {
			/* The cache is an actual traced owner, never a realm or script-visible collection. */
			option = (struct dom_element *)node;
			owner = dom_option_select(node);
			old = option->option_select;
			option->option_select = owner;
			if (old != owner) {
				/* The last selected incoming subtree member wins over an existing destination peer. */
				preferred = select_tree_preferred(root, owner);
				select_prefer(owner, preferred);
				dom_select_reset(old);
				dom_select_reset(owner);
			}
		}

		/* Visit every ordinary descendant, stopping before the root's next sibling. */
		node = select_subtree_next(root, node);
	}
}

/*
 * Applies select-family attribute transitions after actual no-namespace mutation.
 */
void
dom_select_attribute_changed(
	struct dom_element *element,
	struct vm_string *name,
	int before,
	int after)
{
	struct dom_node *node;
	struct dom_node *owner;
	struct dom_node *member;
	struct dom_element *option;
	int actual;
	int matches;
	int kept;

	/* Attribute names remain exact and their presence transition is already committed. */
	node = &element->node;
	actual = select_html(node, DOM_TAG_OPTION);
	if (actual) {
		matches = vm_string_equal_ascii(name, "selected");
		if (matches && before != after) {
			/* Dirty selectedness is independent of subsequent default attribute changes. */
			if (!element->option_dirty) {
				dom_option_set_selected(element, after, 0);
			} else {
				/* Dirty defaults still request ordinary fallback without overwriting owned state. */
				owner = dom_option_select(node);
				dom_select_reset(owner);
			}

			/* This committed default-presence transition has no other select-family meaning. */
			return;
		}

		/* Disabled presence can affect the first enabled single-select fallback. */
		matches = vm_string_equal_ascii(name, "disabled");
		if (matches && before != after) {
			owner = dom_option_select(node);
			dom_select_reset(owner);
		}

		/* Other option attributes never overwrite owned selectedness or dirtiness. */
		return;
	}

	/* Group disabling affects its owning option list without creating reflected IDL features. */
	actual = select_html(node, DOM_TAG_OPTGROUP);
	matches = vm_string_equal_ascii(name, "disabled");
	if (actual && matches && before != after) {
		owner = select_ancestor(node);
		dom_select_reset(owner);
		return;
	}

	/* Only actual selects have display-size and multiple normalization hooks. */
	actual = select_html(node, DOM_TAG_SELECT);
	if (!actual)
		return;
	matches = vm_string_equal_ascii(name, "size");
	if (matches) {
		dom_select_reset(node);
		return;
	}

	/* Removing multiple retains the first selected member, independent of ordinary last-wins repair. */
	matches = vm_string_equal_ascii(name, "multiple");
	if (!matches || before == after)
		return;
	if (!after) {
		kept = 0;
		member = dom_select_option_next(node, NULL);
		while (member != NULL) {
			option = (struct dom_element *)member;
			if (option->option_selected) {
				/* Only the first existing selection survives this attribute transition. */
				if (kept)
					option->option_selected = 0;
				kept = 1;
			}

			/* Continue in current option tree order, ignoring foreign and blocked descendants. */
			member = dom_select_option_next(node, member);
		}
	}

	/* Added or removed multiple updates single-selection fallback according to the new attribute. */
	dom_select_reset(node);
}

/* Distinguishes exact HTML local names from folded XML and foreign tags. */
static int
select_html(
	const struct dom_node *node,
	int tag)
{
	const struct dom_element *element;
	const char *name;
	int actual;

	/* Optional nodes and non-elements cannot own or join an option list. */
	if (node == NULL || node->type != DOM_ELEMENT)
		return 0;
	element = (const struct dom_element *)node;

	/* Internal folded tag codes never override namespace or exact local case. */
	if (element->ns != DOM_NS_HTML || element->tag != tag)
		return 0;
	name = dom_tag_name(tag);
	actual = vm_string_equal_ascii(element->local_name, name);

	/* Succeeded: this identity check does not allocate or invoke script. */
	return actual;
}

/* Finds a nearest actual select while honoring option-list blocking ancestors. */
static struct dom_node *
select_ancestor(
	struct dom_node *node)
{
	struct dom_node *ancestor;
	int grouped;
	int actual;

	/* An option list may cross one group and ordinary wrappers, but no blocking container. */
	grouped = 0;
	ancestor = node->parent;
	while (ancestor != NULL) {
		actual = select_html(ancestor, DOM_TAG_OPTION);
		if (actual)
			return NULL;
		actual = select_html(ancestor, DOM_TAG_DATALIST);
		if (actual)
			return NULL;
		actual = select_html(ancestor, DOM_TAG_HR);
		if (actual)
			return NULL;
		actual = select_html(ancestor, DOM_TAG_OPTGROUP);
		if (actual) {
			/* Two nested groups block membership of the outer select. */
			if (grouped)
				return NULL;
			grouped = 1;
		}

		/* An actual nearest select terminates the ordinary ancestor search. */
		actual = select_html(ancestor, DOM_TAG_SELECT);
		if (actual)
			return ancestor;
		ancestor = ancestor->parent;
	}

	/* No ordinary ancestor establishes option ownership. */
	return NULL;
}

/* Classifies option disability for automatic selection, independently of select disabling. */
static int
select_disabled(
	struct dom_node *node)
{
	struct vm_string *attribute;
	struct dom_node *ancestor;
	int actual;

	/* An option's own disabled attribute always suppresses automatic fallback. */
	attribute = dom_attribute_ascii((struct dom_element *)node, "disabled");
	if (attribute != NULL)
		return 1;

	/* The nearest optgroup contributes disability until a blocking container or select is reached. */
	ancestor = node->parent;
	while (ancestor != NULL) {
		actual = select_html(ancestor, DOM_TAG_SELECT);
		if (actual)
			return 0;
		actual = select_html(ancestor, DOM_TAG_OPTION);
		if (actual)
			return 0;
		actual = select_html(ancestor, DOM_TAG_DATALIST);
		if (actual)
			return 0;
		actual = select_html(ancestor, DOM_TAG_HR);
		if (actual)
			return 0;
		actual = select_html(ancestor, DOM_TAG_OPTGROUP);
		if (actual) {
			attribute = dom_attribute_ascii((struct dom_element *)ancestor, "disabled");
			if (attribute != NULL)
				return 1;
			return 0;
		}

		/* Ordinary wrapper elements do not disable their descendants. */
		ancestor = ancestor->parent;
	}

	/* An option outside any disabling group remains enabled. */
	return 0;
}

/* Parses only the display-size classes needed by single-select fallback without integer overflow. */
static int
select_display_size(
	struct dom_node *select)
{
	struct vm_string *attribute;
	size_t offset;
	uint16_t unit;
	int size;
	int digit;
	int negative;

	/* Missing size defaults to one here, because multiple lists already bypass normalization. */
	attribute = dom_attribute_ascii((struct dom_element *)select, "size");
	if (attribute == NULL)
		return 1;

	/* The nonnegative-integer parser skips ASCII whitespace and accepts an optional plus. */
	offset = 0;
	while (offset < attribute->length) {
		unit = vm_string_at(attribute, offset);
		if (unit != ' ' &&
		    unit != '\t' &&
		    unit != '\r' &&
		    unit != '\n' &&
		    unit != '\f')
			break;
		offset++;
	}

	/* A missing digit sequence uses the single-select default. */
	if (offset == attribute->length)
		return 1;

	/* Integer parsing accepts negative zero, while negative nonzero is not nonnegative. */
	unit = vm_string_at(attribute, offset);
	negative = 0;
	if (unit == '-') {
		negative = 1;
		offset++;
	} else if (unit == '+') {
		offset++;
	}

	/* Accumulate zero, one or greater-than-one while preserving arbitrarily long valid digit strings. */
	size = 0;
	digit = 0;
	while (offset < attribute->length) {
		unit = vm_string_at(attribute, offset);
		if (unit < '0' || unit > '9')
			break;
		digit = 1;
		if (size < 2) {
			size = size * 10 + unit - '0';
			if (size > 2)
				size = 2;
		}

		/* Trailing non-digits terminate the parsed unsigned prefix. */
		offset++;
	}

	/* Invalid unsigned prefixes use the default instead of a negative display size. */
	if (!digit || (negative && size != 0))
		return 1;

	/* Succeeded: this finite class preserves exactly whether the normative display size is one. */
	return size;
}

/* Makes a preferred option the sole selected member without changing peer dirtiness. */
static void
select_prefer(
	struct dom_node *select,
	struct dom_node *preferred)
{
	struct vm_string *attribute;
	struct dom_node *node;
	struct dom_element *option;

	/* Detached owners and absent preferred candidates need no exclusive selection. */
	if (select == NULL || preferred == NULL)
		return;
	attribute = dom_attribute_ascii((struct dom_element *)select, "multiple");
	if (attribute != NULL)
		return;

	/* Clear all other current option members, including disabled ones, without touching defaults. */
	node = dom_select_option_next(select, NULL);
	while (node != NULL) {
		option = (struct dom_element *)node;
		if (node != preferred)
			option->option_selected = 0;
		node = dom_select_option_next(select, node);
	}
}

/* Advances through a finite ordinary subtree without entering separate template contents. */
static struct dom_node *
select_subtree_next(
	struct dom_node *root,
	struct dom_node *node)
{
	/* Descendant preorder prefers the current first child. */
	if (node->first_child != NULL)
		return node->first_child;

	/* Find a later subtree without crossing the supplied root's sibling boundary. */
	while (node != root && node->next == NULL)
		node = node->parent;
	if (node == root)
		return NULL;

	/* Succeeded: the following sibling remains inside the requested ordinary subtree. */
	return node->next;
}

/* Finds the last selected member of an incoming subtree for one actual destination owner. */
static struct dom_node *
select_tree_preferred(
	struct dom_node *root,
	struct dom_node *owner)
{
	struct dom_node *node;
	struct dom_node *actual;
	struct dom_node *preferred;
	struct dom_element *option;
	int html;

	/* Detached subtrees cannot establish exclusive selection in any owner. */
	if (owner == NULL)
		return NULL;

	/* Select only owned current option members from the incoming ordinary subtree. */
	preferred = NULL;
	node = root;
	while (node != NULL) {
		html = select_html(node, DOM_TAG_OPTION);
		if (html) {
			option = (struct dom_element *)node;
			actual = dom_option_select(node);
			if (actual == owner && option->option_selected)
				preferred = node;
		}

		/* Continue through the incoming subtree while leaving outside peers untouched. */
		node = select_subtree_next(root, node);
	}

	/* Succeeded: this candidate preserves incoming selectedness before peers are normalized. */
	return preferred;
}
