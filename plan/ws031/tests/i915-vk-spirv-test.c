/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * Host fixture for the SPIR-V parser (p004). Parses the vkdemo vertex and
 * fragment shaders and checks the extracted interface and instruction stream.
 * VK_REPO names the repository root so the .spv files can be read.
 */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned fixture_live;

void *
kern_calloc(size_t count, size_t size)
{
	void *pointer = calloc(count, size);
	if (pointer != NULL)
		fixture_live++;
	return pointer;
}

void
kern_free(void *pointer)
{
	if (pointer != NULL)
		fixture_live--;
	free(pointer);
}

#include "../../../src/drivers/gpu/i915/compiler/spirv.c"

/* Loads a SPIR-V file into a word buffer the caller frees. */
static uint32_t *
load_spv(const char *name, size_t *words)
{
	char path[512];
	FILE *file;
	long size;
	uint32_t *code;

	snprintf(path, sizeof(path), "%s/userland/tests/vkdemo/shaders/%s", VK_REPO, name);
	file = fopen(path, "rb");
	assert(file != NULL);
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	assert(size > 0 && (size % 4) == 0);
	code = malloc((size_t)size);
	assert(code != NULL);
	assert(fread(code, 1, (size_t)size, file) == (size_t)size);
	fclose(file);
	*words = (size_t)size / 4U;
	return code;
}

/* Counts IR instructions of one opcode. */
static unsigned
count_op(const struct i915_shader_ir *ir, enum i915_shader_ir_op op)
{
	unsigned found;
	unsigned index;

	found = 0U;
	for (index = 0U; index < ir->instruction_count; index++) {
		if (ir->instructions[index].op == op)
			found++;
	}
	return found;
}

/*
 * The vkdemo vertex shader (glslc -O0, as shipped) keeps its intermediate values in
 * Function-storage variables and uses float constants, OpFNegate, component access and
 * 3/4-element composites.  It parses now; what the IR COMPUTES is checked by the lowering
 * fixture (i915-vk-lower-test.c), here only the interface and the shape of the stream.
 */
static void
test_vertex(void)
{
	uint32_t *code;
	size_t words;
	struct i915_shader_ir *ir;
	struct i915_compile_diagnostic diag;
	int error;

	code = load_spv("cuboid.vert.spv", &words);
	error = drv_i915_shader_parse(code, words, I915_STAGE_VERTEX, &ir, &diag);
	if (error != 0)
		printf("  vkdemo VS refused: opcode %u at word %u: %s\n", diag.opcode, diag.word_offset, diag.reason);
	assert(error == 0 && ir != NULL);
	assert(ir->stage == I915_STAGE_VERTEX);
	assert(ir->input_count == 2U && ir->output_count == 1U && ir->uniform_count == 0U);
	assert(ir->inputs[0].location == 0U && ir->inputs[0].components == 3U);
	assert(ir->inputs[1].location == 1U && ir->inputs[1].components == 2U);
	assert(ir->outputs[0].location == 0U && ir->outputs[0].components == 2U);
	assert(ir->push_bytes == 4U);                                   /* one float: animation.seconds */
	assert(count_op(ir, I915_IR_SIN) == 2U && count_op(ir, I915_IR_COS) == 2U);
	assert(count_op(ir, I915_IR_FNEG) == 1U);                    /* -sy; the -1.6 is a constant */
	assert(count_op(ir, I915_IR_LOAD_PUSH) == 2U);               /* seconds is read twice */
	assert(count_op(ir, I915_IR_STORE_OUTPUT) == 6U);            /* gl_Position 4 + texture_coordinate 2 */
	drv_i915_shader_ir_free(ir);
	free(code);
}

/* a minimal module: header, OpFunction, one body instruction, OpFunctionEnd */
static int
parse_body_instruction(const uint32_t *inst, unsigned inst_words, struct i915_compile_diagnostic *diag)
{
	uint32_t module[32];
	struct i915_shader_ir *ir;
	unsigned n = 0U, i;
	int error;

	module[n++] = 0x07230203U; module[n++] = 0x00010000U; module[n++] = 0U; module[n++] = 16U; module[n++] = 0U;
	module[n++] = (5U << 16) | 54U; module[n++] = 1U; module[n++] = 2U; module[n++] = 0U; module[n++] = 3U;   /* OpFunction */
	module[n++] = (2U << 16) | 248U; module[n++] = 4U;                                                        /* OpLabel */
	for (i = 0U; i < inst_words; i++)
		module[n++] = inst[i];
	module[n++] = (1U << 16) | 253U;                                                                          /* OpReturn */
	module[n++] = (1U << 16) | 56U;                                                                           /* OpFunctionEnd */
	error = drv_i915_shader_parse(module, n, I915_STAGE_VERTEX, &ir, diag);
	if (error == 0)
		drv_i915_shader_ir_free(ir);
	else
		assert(ir == NULL);
	return error;
}

static void
test_body_classification(void)
{
	struct i915_compile_diagnostic diag;
	/* OpLine (debug): file id 5, line 1, column 1 -- no execution semantics */
	static const uint32_t op_line[4] = { (4U << 16) | 8U, 5U, 1U, 1U };
	/* OpFNegate %6 = -%7, where %7 is not a float value (nothing defines it): refused, not skipped */
	static const uint32_t op_fnegate[4] = { (4U << 16) | 127U, 1U, 6U, 7U };
	/* OpFDiv of %7, which is not a float value (nothing defines it): refused, not skipped */
	static const uint32_t op_fdiv[5] = { (5U << 16) | 136U, 1U, 6U, 7U, 8U };
	/* OpBranch back to the block it ends (%4): a loop, which is not lowered */
	static const uint32_t op_branch[2] = { (2U << 16) | 249U, 4U };
	/* OpFunctionCall */
	static const uint32_t op_call[4] = { (4U << 16) | 57U, 1U, 6U, 7U };
	/* an instruction whose length runs past the module */
	static const uint32_t op_overrun[1] = { (9U << 16) | 129U };
	/* OpExtInst with a GLSL.std.450 instruction that is not lowered (Tan = 15) */
	static const uint32_t op_tan[6] = { (6U << 16) | 12U, 1U, 6U, 9U, 15U, 7U };

	assert(parse_body_instruction(op_line, 4U, &diag) == 0);
	assert(parse_body_instruction(op_fnegate, 4U, &diag) == ENOTSUP && diag.opcode == 127U);
	assert(parse_body_instruction(op_fdiv, 5U, &diag) == ENOTSUP && diag.opcode == 136U);
	assert(parse_body_instruction(op_branch, 2U, &diag) == ENOTSUP && diag.opcode == 249U);
	assert(parse_body_instruction(op_call, 4U, &diag) == ENOTSUP && diag.opcode == 57U);
	assert(parse_body_instruction(op_tan, 6U, &diag) == ENOTSUP && diag.opcode == 12U);
	/* malformed stays EINVAL: a different thing from "valid but not lowered" */
	assert(parse_body_instruction(op_overrun, 1U, &diag) == EINVAL);
}

static void
test_fragment(void)
{
	uint32_t *code;
	size_t words;
	struct i915_shader_ir *ir;
	int error;

	code = load_spv("cuboid.frag.spv", &words);
	error = drv_i915_shader_parse(code, words, I915_STAGE_FRAGMENT, &ir, NULL);
	assert(error == 0);

	/* The fragment shader samples one texture at an interpolated coordinate. */
	assert(ir->stage == I915_STAGE_FRAGMENT);
	assert(ir->input_count == 1U);
	assert(ir->inputs[0].location == 0U && ir->inputs[0].components == 2U);
	assert(ir->output_count == 1U);
	assert(ir->outputs[0].location == 0U && ir->outputs[0].components == 4U);
	assert(ir->uniform_count == 1U);
	assert(ir->uniforms[0].set == 0U && ir->uniforms[0].binding == 0U);
	assert(count_op(ir, I915_IR_SAMPLE) >= 1U);

	drv_i915_shader_ir_free(ir);
	free(code);
}

static void
test_rejects_garbage(void)
{
	uint32_t bad[8];
	struct i915_shader_ir *ir;
	int error;

	/* A wrong magic is rejected without allocating an IR. */
	memset(bad, 0, sizeof(bad));
	bad[0] = 0x12345678U;
	error = drv_i915_shader_parse(bad, 8U, I915_STAGE_VERTEX, &ir, NULL);
	assert(error != 0);
	assert(ir == NULL);
}

int
main(void)
{
	test_vertex();
	test_body_classification();
	test_fragment();
	test_rejects_garbage();
	assert(fixture_live == 0U);
	printf("i915 vk spirv host test PASS\n");
	return 0;
}
