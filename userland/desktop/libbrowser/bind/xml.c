/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Genuine XML character interfaces expose native identity without synthesizing public factories. */

#include "bind/internal.h"

static int pi_target(struct vm_realm *realm, vm_value receiver, const vm_value *args, unsigned count, vm_value *result);

/* The exact PI-only target getter never consults script-side target properties. */
static const struct bind_attribute pi_attributes[] = {
	{ "target", pi_target, NULL },
	{ NULL, NULL, NULL }
};

/* CDATA inherits genuine Text operations while preserving its own actual native prototype. */
const struct bind_interface bind_cdata_interface = {
	"CDATASection", BIND_TEXT, 0, NULL, NULL, NULL, NULL
};

/* This partial PI core inherits real CharacterData and exposes its actual immutable target. */
const struct bind_interface bind_pi_interface = {
	"ProcessingInstruction", BIND_CHARACTER_DATA, 0, NULL, pi_attributes, NULL, NULL
};

/* Returns only a genuine ProcessingInstruction's native traced target. */
static int
pi_target(
	struct vm_realm *realm,
	vm_value receiver,
	const vm_value *args,
	unsigned count,
	vm_value *result)
{
	struct dom_node *node;
	struct dom_character_data *data;
	int status;

	UNUSED_PARAMETER(args);
	UNUSED_PARAMETER(count);

	/* Native node identity precedes any public prototype or expando metadata. */
	node = bind_node_of(receiver);
	if (node == NULL || node->type != DOM_PROCESSING_INSTRUCTION) {
		status = bind_throw_illegal(realm);
		return status;
	}

	/* The actual target is already retained by the receiver's native node graph. */
	data = (struct dom_character_data *)node;
	*result = vm_value_cell(data->target);

	/* Succeeded: no allocation or copied substitute changes target identity. */
	return 0;
}
