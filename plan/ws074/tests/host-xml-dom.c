/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Observes real XML projection, copied native graphs, threshold collection and allocation failures. */

#include "xml/dom.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* Count independent observations without replacing any native parser or allocation operation. */
static unsigned checks;
/* Retain failed observations while normal model and heap cleanup continues. */
static unsigned failures;

static void projection_check(int condition, const char *name);
static int projection_owned(size_t limit, int pressure);
static int projection_normal(struct vm_heap *heap, struct vm_cell **held);
static int projection_pressure(struct vm_heap *heap, struct vm_cell **held, size_t limit);
static int projection_resources(void);
static int projection_resource(const char *path, const char *uri, enum dom_document_content content, int rejected);
static int projection_ascii(const struct vm_string *value, const char *expected);
static int projection_data(const struct dom_node *node, const char *expected);
static struct dom_element *projection_root(struct dom_document *document);

/*
 * Tests complete native conversion without creating a Window or activating resource scripts.
 */
int
main(
	void)
{
	int status;
	int printed;

	/* Verify ordinary copied trees before independent pinned resources. */
	status = projection_owned(0, 0);
	if (status != 0)
		return 2;

	/* Project the original actual XML resources into separately owned native heaps. */
	status = projection_resources();
	if (status != 0)
		return 2;

	/* Cross the unchanged native allocation threshold with the full original wide tree. */
	status = projection_owned(0, 1);
	if (status != 0)
		return 2;

	/* Observe real partial-publication refusal under the original bounded heap. */
	status = projection_owned(2048, 1);
	if (status != 0)
		return 2;

	/* Reports all native projection observations after independent owner teardown. */
	printed = printf("native XML DOM: %u/%u passed\n", checks - failures, checks);
	if (printed < 0)
		return 2;

	/* Refuse a fixture with any failed real native projection observation. */
	if (failures != 0)
		return 1;

	/* Succeeded: independently owned native records survived genuine collector and parser lifetimes. */
	return 0;
}

/* Preserves each semantic failure while independent checks can still complete. */
static void
projection_check(
	int condition,
	const char *name)
{
	int printed;

	/* A failed observation remains visible in the final bounded fixture result. */
	checks++;
	if (!condition) {
		failures++;
		printed = fprintf(stderr, "FAIL %s\n", name);
		if (printed < 0)
			failures++;
	}

	/* Succeeded: every observation remains in the final native fixture count. */
	return;
}

/* Owns the nullable result root and heap across every return from a semantic test. */
static int
projection_owned(
	size_t limit,
	int pressure)
{
	struct vm_heap *heap;
	struct vm_cell *held;
	int status;

	/* No conservative caller stack can conceal missing projector or native graph roots. */
	status = vm_heap_create(&heap, limit);
	if (status != 0)
		return status;

	/* Register the nullable result slot with conservative caller scanning disabled. */
	vm_heap_set_stack_base(heap, NULL);
	held = NULL;
	status = vm_heap_add_root(heap, &held);
	if (status != 0) {
		vm_heap_destroy(heap);
		return status;
	}

	/* Run only the selected original native conversion with its registered nullable result slot. */
	if (pressure) {
		status = projection_pressure(heap, &held, limit);
		if (status != 0) {
			vm_heap_remove_root(heap, &held);
			vm_heap_destroy(heap);
			return status;
		}
	} else {
		status = projection_normal(heap, &held);
		if (status != 0) {
			vm_heap_remove_root(heap, &held);
			vm_heap_destroy(heap);
			return status;
		}
	}

	/* Successful conversion no longer needs the caller-owned nullable slot. */
	vm_heap_remove_root(heap, &held);
	vm_heap_destroy(heap);

	/* Succeeded: no stack registration or private native allocation escapes. */
	return 0;
}

/* Observes namespace identity, copied character buffers and genuine owner tracing after model destruction. */
static int
projection_normal(
	struct vm_heap *heap,
	struct vm_cell **held)
{
	unsigned char input[] = "<?keep flag?><R xmlns='urn:Root' xmlns:p='urn:p' xmlns:q='urn:q' p:a='one' q:a='two' a='plain' xml:lang='en'><p:Mixed><![CDATA[<&]]><!--comment-->A&amp;&#x1f600;<?inside data?></p:Mixed><Empty xmlns=''/><\xce\xb1/></R>";
	struct xml_document *model;
	struct xml_error error;
	struct dom_document *document;
	struct dom_document *rejected;
	struct dom_element *root;
	struct dom_element *mixed;
	struct dom_element *empty;
	struct dom_element *greek;
	struct dom_node *node;
	struct dom_character_data *data;
	struct dom_attribute *attribute;
	struct vm_cell *found;
	uintptr_t document_address;
	uintptr_t target_address;
	uintptr_t uri_address;
	uint16_t unit;
	int status;
	int same;

	/* Invalid embedding arguments are rejected before allocating a native graph. */
	status = xml_document_parse(&model, input, sizeof(input) - 1, &error);
	if (status != 0)
		return status;
	status = xml_document_project(heap, model, DOM_CONTENT_HTML, &rejected);
	projection_check(status == EINVAL && rejected == NULL, "HTML content selector rejected with NULL output");
	status = xml_document_project(NULL, model, DOM_CONTENT_XML, &rejected);
	projection_check(status == EINVAL && rejected == NULL, "missing heap rejected with NULL output");
	status = xml_document_project(heap, NULL, DOM_CONTENT_XML, &rejected);
	projection_check(status == EINVAL && rejected == NULL, "missing model rejected with NULL output");
	status = xml_document_project(heap, model, DOM_CONTENT_XML, NULL);
	projection_check(status == EINVAL, "missing output rejected");
	status = xml_document_project(heap, model, DOM_CONTENT_XML, &document);
	if (status != 0) {
		xml_document_destroy(model);
		return status;
	}

	/* The complete native graph has its original caller slot before model/input destruction. */
	if (document == NULL) {
		xml_document_destroy(model);
		return EINVAL;
	}

	/* Publish only the complete result into its original caller-owned root slot. */
	*held = &document->node.cell;
	xml_document_destroy(model);
	memset(input, '!', sizeof(input));

	/* Every following observation occurs after both source input and model storage are destroyed. */
	projection_check(document->content == DOM_CONTENT_XML && document->view == NULL, "actual XML Document has explicit content and no activation owner");
	node = document->node.first_child;
	if (node == NULL ||
	    node->next == NULL ||
	    node->type != DOM_PROCESSING_INSTRUCTION)
		return EINVAL;
	data = (struct dom_character_data *)node;
	same = projection_ascii(data->target, "keep");
	projection_check(node->type == DOM_PROCESSING_INSTRUCTION && same, "Document PI target copied exactly");
	same = projection_data(node, "flag");
	projection_check(same, "Document PI copied character data");
	target_address = (uintptr_t)data->target;
	root = projection_root(document);
	if (root == NULL ||
	    root->node.first_child == NULL ||
	    root->attribute_count < 7 ||
	    root->attributes == NULL)
		return EINVAL;
	same = projection_ascii(root->local_name, "R");
	projection_check(same && root->ns == DOM_NS_OTHER, "XML root case and arbitrary namespace class preserved");
	same = projection_ascii(root->namespace_uri, "urn:Root");
	projection_check(same && root->attribute_count == 7, "root URI and all actual expanded attributes copied");
	projection_check(root->node.parent == &document->node && root->node.previous == node, "real Document preorder and parent links");
	attribute = &root->attributes[3];
	same = projection_ascii(attribute->namespace_uri, "urn:p");
	projection_check(same && attribute->ns == DOM_NS_OTHER, "custom attribute exact URI retained");
	attribute = dom_element_find_attribute_uri(root, attribute->namespace_uri, attribute->name);
	if (attribute == NULL)
		return EINVAL;
	same = projection_ascii(attribute->value, "one");
	projection_check(same, "first expanded attribute value remains distinct");
	uri_address = (uintptr_t)attribute->namespace_uri;
	attribute = &root->attributes[4];
	attribute = dom_element_find_attribute_uri(root, attribute->namespace_uri, attribute->name);
	if (attribute == NULL)
		return EINVAL;
	same = projection_ascii(attribute->value, "two");
	projection_check(same, "same local attribute in another URI remains distinct");
	attribute = &root->attributes[5];
	projection_check(attribute->namespace_uri == NULL && attribute->ns == DOM_NS_NONE, "unprefixed ordinary attribute has no default element namespace");
	same = projection_ascii(root->attributes[6].namespace_uri, "http://www.w3.org/XML/1998/namespace");
	projection_check(same && root->attributes[6].ns == DOM_NS_XML, "canonical XML attribute namespace classified exactly");
	same = projection_ascii(root->attributes[0].namespace_uri, "http://www.w3.org/2000/xmlns/");
	projection_check(same && root->attributes[0].ns == DOM_NS_XMLNS, "namespace declaration is an actual XMLNS attribute");

	/* Read element fields only from the genuine native first element child. */
	if (root->node.first_child->type != DOM_ELEMENT)
		return EINVAL;
	mixed = (struct dom_element *)root->node.first_child;
	same = projection_ascii(mixed->local_name, "Mixed");
	projection_check(same, "prefixed element local case preserved");
	same = projection_ascii(mixed->prefix, "p");
	projection_check(same, "prefixed element prefix copied");
	same = projection_ascii(mixed->namespace_uri, "urn:p");
	projection_check(same && mixed->node.document == document, "prefixed element exact URI and owner retained");
	node = mixed->node.first_child;
	if (node == NULL ||
	    node->next == NULL ||
	    node->next->next == NULL)
		return EINVAL;
	same = projection_data(node, "<&");
	projection_check(same && node->type == DOM_CDATA_SECTION, "CDATA retains subtype and literal characters");
	node = node->next;
	same = projection_data(node, "comment");
	projection_check(same && node->type == DOM_COMMENT, "Comment copied with exact native kind");
	node = node->next;

	/* Native text alone supplies the decoded character buffer representation. */
	if (node->type != DOM_TEXT)
		return EINVAL;
	data = (struct dom_character_data *)node;
	projection_check(node->type == DOM_TEXT && data->data.length == 4, "decoded supplementary text keeps exact UTF16 length");

	/* Inspect the original exact supplementary text only after its four-unit buffer exists. */
	if (data->data.length == 4) {
		if (data->data.data == NULL)
			return EINVAL;
		projection_check(data->data.data[0] == 'A' && data->data.data[1] == '&', "text entity decoding copied");
		projection_check(data->data.data[2] == 0xd83dU && data->data.data[3] == 0xde00U, "supplementary scalar surrogate pair copied");
	}

	/* The final real mixed child is its own processing instruction. */
	node = node->next;
	if (node == NULL || node->type != DOM_PROCESSING_INSTRUCTION)
		return EINVAL;

	/* The existing processing instruction supplies its actual target and character fields. */
	data = (struct dom_character_data *)node;
	same = projection_ascii(data->target, "inside");
	projection_check(same && node->type == DOM_PROCESSING_INSTRUCTION, "nested PI retains actual target and kind");
	same = projection_data(node, "data");
	projection_check(same && mixed->node.last_child == node, "nested PI data and last-child order retained");

	/* Follow only the two genuine native element siblings in the original parsed tree. */
	if (mixed->node.next == NULL || mixed->node.next->type != DOM_ELEMENT)
		return EINVAL;
	empty = (struct dom_element *)mixed->node.next;
	if (empty->node.next == NULL || empty->node.next->type != DOM_ELEMENT)
		return EINVAL;
	greek = (struct dom_element *)empty->node.next;
	if (greek->local_name == NULL)
		return EINVAL;
	projection_check(empty->ns == DOM_NS_NONE && empty->namespace_uri == NULL, "default empty namespace resets native element identity");
	unit = vm_string_at(greek->local_name, 0);
	projection_check(greek->local_name->length == 1 && unit == 0x03b1U, "Unicode element name retained as exact native atom");
	same = projection_ascii(greek->namespace_uri, "urn:Root");
	projection_check(same && root->node.last_child == &greek->node, "sibling namespace scope and native traversal stay aligned");

	/* A single subtree root must trace its actual owner, siblings, PI target and non-atom attribute URI. */
	document_address = (uintptr_t)document;
	*held = &mixed->node.cell;
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, document_address);
	projection_check(found != NULL, "subtree alone retains actual owning Document");
	found = vm_heap_find_cell(heap, target_address);
	projection_check(found != NULL, "subtree alone retains sibling PI non-atom target");
	found = vm_heap_find_cell(heap, uri_address);
	projection_check(found != NULL, "subtree alone retains non-atom expanded attribute URI");

	/* Permanent name atoms are normal heap intern state; actual Document and ordinary strings must collect. */
	*held = NULL;
	vm_heap_collect(heap);
	found = vm_heap_find_cell(heap, document_address);
	projection_check(found == NULL, "last graph root release collects actual Document");
	found = vm_heap_find_cell(heap, target_address);
	projection_check(found == NULL, "last graph root release collects PI target");
	found = vm_heap_find_cell(heap, uri_address);
	projection_check(found == NULL, "last graph root release collects expanded URI");

	/* Succeeded: no model storage or permanent projector root retained the native graph. */
	return 0;
}

/* Constructs a real wide model whose ordinary conversion crosses the unchanged production GC threshold. */
static int
projection_pressure(
	struct vm_heap *heap,
	struct vm_cell **held,
	size_t limit)
{
	struct wb_buffer bytes;
	struct xml_document *model;
	struct xml_error error;
	struct dom_document *document;
	struct dom_element *root;
	struct dom_element *element;
	struct dom_node *node;
	struct vm_heap_stats before;
	struct vm_heap_stats after;
	struct vm_heap_stats released;
	size_t records;
	size_t index;
	size_t count;
	int status;
	int same;
	int correct;

	/* Source contains real elements and attributes, not artificial VM pressure or collector hooks. */
	wb_buffer_init(&bytes);
	status = wb_buffer_append_string(&bytes, "<r xmlns='urn:pressure'>");
	if (status != 0) {
		wb_buffer_release(&bytes);
		return status;
	}

	/* Append all original thirty thousand native element/attribute sources. */
	for (index = 0; index < 30000U; index++) {
		status = wb_buffer_append_string(&bytes, "<q a='v'/>");
		if (status != 0) {
			wb_buffer_release(&bytes);
			return status;
		}
	}

	/* Close only the completely constructed original wide source. */
	status = wb_buffer_append_string(&bytes, "</r>");
	if (status != 0) {
		wb_buffer_release(&bytes);
		return status;
	}

	/* Parse the actual immutable source before releasing its input storage. */
	status = xml_document_parse(&model, bytes.data, bytes.length, &error);
	if (status != 0) {
		wb_buffer_release(&bytes);
		return status;
	}

	/* The complete model no longer borrows the temporary source buffer. */
	wb_buffer_release(&bytes);
	records = model->records;
	vm_heap_stats(heap, &before);
	status = xml_document_project(heap, model, DOM_CONTENT_XML, &document);
	if (limit == 0 && status != 0) {
		xml_document_destroy(model);
		return status;
	}

	/* Capture collection evidence only after success or the selected bounded refusal. */
	vm_heap_stats(heap, &after);

	/* A real bounded native heap fails midway, publishes nothing and leaves its caller-owned model unchanged. */
	if (limit != 0) {
		projection_check(status == ENOMEM && document == NULL, "actual heap limit leaves NULL output on projection failure");
		projection_check(model->records == records && model->node.first_child != NULL, "failed projection preserves caller model");
		xml_document_destroy(model);
		vm_heap_collect(heap);
		vm_heap_stats(heap, &released);
		projection_check(released.freed_cells > after.freed_cells, "unpublished partial native graph collected after failure");
		projection_check(released.live_bytes < after.live_bytes, "failure cleanup releases native graph bytes while normal name atoms remain");

		/* Succeeded: the original bounded failure published no retained native graph. */
		return 0;
	}

	/* The completed result is rooted before source destruction or any further heap allocation. */
	if (document == NULL) {
		xml_document_destroy(model);
		return EINVAL;
	}

	/* Publish only the complete result into its original caller-owned root slot. */
	*held = &document->node.cell;
	xml_document_destroy(model);
	projection_check(after.collections > before.collections, "actual wide-tree conversion triggers production threshold collection");
	root = projection_root(document);
	if (root == NULL)
		return EINVAL;
	count = 0;
	correct = 1;

	/* Observe every copied node and attribute after the source model no longer exists. */
	for (node = root->node.first_child;
	     node != NULL;
	     node = node->next) {
		/* Only actual native elements supply namespace and attribute fields. */
		if (node->type != DOM_ELEMENT)
			return EINVAL;
		element = (struct dom_element *)node;
		same = projection_ascii(element->namespace_uri, "urn:pressure");
		if (!same ||
		    node->document != document ||
		    node->parent != &root->node)
			correct = 0;

		/* Each copied wide element must retain its one real expanded attribute. */
		if (element->attribute_count != 1 || element->attributes == NULL) {
			correct = 0;
		} else {
			same = projection_ascii(element->attributes[0].value, "v");
			if (!same)
				correct = 0;
		}

		/* Count actual linked elements without suppressing any earlier attribute failure. */
		count++;
	}

	/* Full traversal confirms that neither collection nor iteration lost native records. */
	projection_check(correct && count == 30000U, "all 30000 actual elements and pending attributes survive mid-conversion GC");
	vm_heap_collect(heap);
	projection_check(root->node.first_child != NULL && root->node.last_child != NULL, "completed wide native graph survives precise result root");
	*held = NULL;
	vm_heap_collect(heap);
	vm_heap_stats(heap, &released);
	projection_check(released.live_bytes < after.live_bytes, "wide native graph released without permanent conversion roots");

	/* Succeeded: normal VM pressure exercised ownership throughout real conversion. */
	return 0;
}

/* Uses pinned ordinary resource bytes, preserving exact namespace and parser rejection behavior. */
static int
projection_resources(
	void)
{
	int status;

	/* Each resource owns an independent model and heap, so no previous test can supply accidental roots. */
	status = projection_resource("build/ws074-suites/wpt/acid/acid3/svg.xml", "http://www.w3.org/2000/svg", DOM_CONTENT_SVG, 0);
	if (status != 0)
		return status;

	/* Project the next independently owned pinned input after checked completion. */
	status = projection_resource("build/ws074-suites/wpt/acid/acid3/xhtml.1", "http://www.w3.org/1999/xhtml", DOM_CONTENT_XHTML, 0);
	if (status != 0)
		return status;

	/* Project the next independently owned pinned input after checked completion. */
	status = projection_resource("build/ws074-suites/wpt/acid/acid3/xhtml.3", "http://www.w3.org/1999/xhtml#", DOM_CONTENT_XHTML, 0);
	if (status != 0)
		return status;

	/* Project the next independently owned pinned input after checked completion. */
	status = projection_resource("build/ws074-suites/wpt/acid/acid3/xhtml.2", NULL, DOM_CONTENT_XHTML, 1);
	if (status != 0)
		return status;

	/* Project the next independently owned pinned input after checked completion. */
	status = projection_resource("build/ws074-suites/wpt/acid/acid3/empty.xml", NULL, DOM_CONTENT_XML, 1);
	if (status != 0)
		return status;

	/* Succeeded: all actual pinned resource projections finished with independent ownership. */
	return 0;
}

/* Copies one real validated resource into an independent heap or observes its genuine parse failure. */
static int
projection_resource(
	const char *path,
	const char *uri,
	enum dom_document_content content,
	int rejected)
{
	struct wb_buffer bytes;
	struct xml_document *model;
	struct xml_error error;
	struct vm_heap *heap;
	struct dom_document *document;
	struct dom_element *root;
	struct dom_node *node;
	struct dom_character_data *data;
	int status;
	int same;
	int script;

	/* Actual raw resource bytes pass through the production strict parser without repair. */
	model = NULL;
	wb_buffer_init(&bytes);
	status = wb_file_read(path, &bytes);
	if (status != 0) {
		wb_buffer_release(&bytes);
		return status;
	}

	/* Genuine parser acceptance or expected refusal owns the complete raw input. */
	status = xml_document_parse(&model, bytes.data, bytes.length, &error);
	if (status != 0 && !rejected) {
		wb_buffer_release(&bytes);
		return status;
	}

	/* The checked parse result no longer borrows resource bytes. */
	wb_buffer_release(&bytes);
	if (rejected) {
		projection_check(status != 0 && model == NULL, path);
		xml_document_destroy(model);

		/* Succeeded: the original malformed resource has no projected native model. */
		return 0;
	}

	/* A rejected ordinary input cannot create a recovered native resource document. */
	if (status != 0)
		return status;

	/* Allocate an independent native heap only for an accepted complete model. */
	status = vm_heap_create(&heap, 0);
	if (status != 0) {
		xml_document_destroy(model);
		return status;
	}

	/* Only the projector supplies temporary ownership during resource conversion. */
	vm_heap_set_stack_base(heap, NULL);
	status = xml_document_project(heap, model, content, &document);
	if (status != 0) {
		xml_document_destroy(model);
		vm_heap_destroy(heap);
		return status;
	}

	/* Observe only a complete successfully published native resource after model release. */
	xml_document_destroy(model);
	if (document == NULL) {
		vm_heap_destroy(heap);
		return EINVAL;
	}

	/* Inspect the actual complete native resource root and unchanged content policy. */
	root = projection_root(document);
	projection_check(root != NULL && document->content == content, path);
	if (root != NULL) {
		same = projection_ascii(root->namespace_uri, uri);
		projection_check(same, "actual pinned root URI copied exactly after model release");
	}

	/* Actual SVG PI and XHTML script nodes remain native data without browsing activation. */
	if (content == DOM_CONTENT_SVG) {
		node = document->node.first_child;
		if (node == NULL || node->type != DOM_PROCESSING_INSTRUCTION) {
			vm_heap_destroy(heap);
			return EINVAL;
		}

		/* The actual stylesheet processing instruction has native character fields. */
		data = (struct dom_character_data *)node;
		same = projection_ascii(data->target, "xml-stylesheet");
		projection_check(same && node->type == DOM_PROCESSING_INSTRUCTION, "actual SVG stylesheet PI preserved without resource effects");
	} else if (root != NULL) {
		script = 0;
		node = root->node.first_child;

		/* Traverse actual preorder to find the real unexecuted XHTML script text. */
		while (node != NULL) {
			if (node->type == DOM_ELEMENT) {
				same = projection_ascii(((struct dom_element *)node)->local_name, "script");
				if (same && node->first_child != NULL) {
					script = 0;
					if (node->first_child->type == DOM_TEXT)
						script = 1;
					break;
				}
			}

			/* Descend along the actual copied resource tree. */
			if (node->first_child != NULL) {
				node = node->first_child;
				continue;
			}

			/* Ascend completed branches to their next real sibling. */
			while (node->next == NULL && node->parent != &root->node)
				node = node->parent;
			node = node->next;
		}

		/* The real copied script is observed without an execution host. */
		projection_check(script, "actual XHTML script source copied as data without Window activation");
	}

	/* The private native heap owns every remaining allocation, including permanent name atoms. */
	vm_heap_destroy(heap);

	/* Succeeded: no resource model or private native heap escapes. */
	return 0;
}

/* Compares actual VM string characters without relying on interning or pointer equality. */
static int
projection_ascii(
	const struct vm_string *value,
	const char *expected)
{
	int same;

	/* Missing strings cannot accidentally match an expected ordinary native value. */
	if (value == NULL)
		return 0;
	same = vm_string_equal_ascii(value, expected);

	/* Succeeded: actual characters determine the observation. */
	return same;
}

/* Compares copied native character buffers independently of all parser storage. */
static int
projection_data(
	const struct dom_node *node,
	const char *expected)
{
	const struct dom_character_data *data;
	size_t length;
	size_t index;
	int character;

	/* Only genuine CharacterData kinds have this native buffer representation. */
	character = dom_is_character_data(node);
	if (!character)
		return 0;

	/* Inspect only the checked genuine CharacterData buffer. */
	data = (const struct dom_character_data *)node;
	length = strlen(expected);
	if (data->data.length != length)
		return 0;

	/* Compare every original independent expected native character. */
	if (length != 0 && data->data.data == NULL)
		return 0;

	/* Compare all actual buffer units after validating native storage. */
	for (index = 0; index < length; index++) {
		if (data->data.data[index] != (unsigned char)expected[index])
			return 0;
	}

	/* Succeeded: complete native character data matches the independent expected value. */
	return 1;
}

/* Finds the actual document element without assuming a PI-free resource. */
static struct dom_element *
projection_root(
	struct dom_document *document)
{
	struct dom_node *node;

	/* Direct native sibling order retains prolog processing instructions and comments. */
	for (node = document->node.first_child;
	     node != NULL;
	     node = node->next) {
		if (node->type == DOM_ELEMENT)
			return (struct dom_element *)node;
	}

	/* No direct element exists in this native Document. */
	return NULL;
}
