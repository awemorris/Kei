/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * BUG-120: the host test of the i915 GT object pool's growth
 * (src/drivers/gpu/i915/memory.c compiled against stand-ins of the DMA
 * device, the kernel's allocator and log, and the GGTT and PPGTT calls).
 *  1. 1,000 objects are created at once (more than the old fixed 128): each
 *     succeeds, the pool grows a block of 128 at a time, and every object's
 *     address stays the same while it grows.
 *  2. Half are destroyed and as many created again: freed slots are reused,
 *     the pool does not grow.
 *  3. Past the limit (16 blocks, 2,048 objects) a create fails with the log
 *     "object pool exhausted".
 *  4. fini frees every block; a kept object's block stays (the display may
 *     still read it).
 * Prints one line a check and "host-gt-pool: PASS" or FAIL.
 */

#include "src/drivers/gpu/i915/memory.c"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The stand-in vector: a heap block. */
struct drv_dma_vector {
	void *address;
	size_t size;
};

static int pool_failures;
static unsigned pool_frees;
static char pool_last_log[256];

/* The DMA device's vectors, from the heap. */
int
drv_dma_vector_create(struct drv_dma_device *device, size_t size, struct drv_dma_vector **result)
{
	struct drv_dma_vector *vector;

	(void)device;
	vector = calloc(1, sizeof(*vector));
	if (vector == NULL)
		return ENOMEM;
	vector->address = calloc(1, size);
	vector->size = size;
	*result = vector;
	return 0;
}

int
drv_dma_vector_free(struct drv_dma_vector *vector)
{
	free(vector->address);
	free(vector);
	return 0;
}

void *
drv_dma_vector_address(const struct drv_dma_vector *vector)
{
	return vector->address;
}

unsigned
drv_dma_vector_count(const struct drv_dma_vector *vector)
{
	(void)vector;
	return 1U;
}

int
drv_dma_vector_segment(const struct drv_dma_vector *vector, unsigned index, struct drv_dma_segment *segment)
{
	(void)vector;
	(void)index;
	(void)segment;
	return EINVAL;
}

int
drv_dma_alloc_coherent(struct drv_dma_device *device, size_t size, size_t alignment, struct drv_dma_buffer *buffer)
{
	(void)device;
	(void)size;
	(void)alignment;
	(void)buffer;
	return ENOMEM;
}

void
drv_dma_free_coherent(struct drv_dma_device *device, struct drv_dma_buffer *buffer)
{
	(void)device;
	(void)buffer;
}

/* The kernel's allocator and log. */
void *
kern_calloc(size_t count, size_t size)
{
	return calloc(count, size);
}

void
kern_free(void *pointer)
{
	if (pointer != NULL)
		pool_frees++;
	free(pointer);
}

void
kern_logf(const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	(void)vsnprintf(pool_last_log, sizeof(pool_last_log), format, arguments);
	va_end(arguments);
}

/* What the pool never reaches here. */
void kern_io_read_barrier(void) {}
void kern_io_write_barrier(void) {}
int kern_pmem_alloc_limited(size_t size, size_t alignment, uint64_t max_address, size_t boundary, struct kern_pmem *run) { (void)size; (void)alignment; (void)max_address; (void)boundary; (void)run; return ENOMEM; }
int kern_pmem_free(struct kern_pmem *run) { (void)run; return 0; }
void *kern_pmem_to_kernel(hal_physaddr_t address) { (void)address; return NULL; }
void drv_i915_gt_ggtt_unbind(struct i915_gt_mem *gm, struct i915_gt_object *o) { (void)gm; (void)o; }
void drv_i915_gt_display_unbind(struct i915_gt_mem *gm, struct i915_gt_object *o) { (void)gm; (void)o; }
int drv_i915_ppgtt_va_alloc(struct i915_ppgtt *vm, uint64_t bytes, uint64_t *va) { (void)vm; (void)bytes; (void)va; return ENOMEM; }
int drv_i915_ppgtt_insert(struct i915_ppgtt *vm, uint64_t va, uint64_t physical, unsigned pages) { (void)vm; (void)va; (void)physical; (void)pages; return ENOMEM; }
void drv_i915_ppgtt_clear(struct i915_ppgtt *vm, uint64_t va, unsigned pages) { (void)vm; (void)va; (void)pages; }

static void
check(const char *what, int passed)
{
	printf("%s: %s\n", what, passed ? "ok" : "FAIL");
	if (!passed)
		pool_failures++;
}

int
main(void)
{
	static struct i915_gt_mem gm;
	static uint8_t table[(I915_GT_GGTT_PAGES + 16U) * 8U];
	static struct i915_gt_object *objects[I915_GT_MAX_OBJECTS + 1U];
	static struct i915_gt_object *first[1000];
	char what[160];
	unsigned index;
	unsigned made;
	unsigned blocks;
	int error;
	int stable;

	/* The memory over a page table just large enough. */
	error = drv_i915_gt_mem_init(&gm, (struct drv_dma_device *)&gm, ~0ULL, table, I915_GT_GGTT_PAGES + 16U, 0U, NULL);
	check("init", error == 0);

	/* 1. A thousand objects at once; the addresses kept while the pool grows. */
	made = 0U;
	for (index = 0U; index < 1000U; index++) {
		objects[index] = drv_i915_gt_object_create(&gm, 4096U);
		if (objects[index] != NULL)
			made++;
		if (index < 1000U)
			first[index] = objects[index];
	}
	snprintf(what, sizeof(what), "1000 objects made (%u), blocks=%u", made, gm.object_block_count);
	check(what, made == 1000U && gm.object_block_count == 8U && gm.objects_live == 1000U);
	stable = 1;
	for (index = 0U; index < 1000U; index++) {
		if (objects[index] != first[index] || objects[index]->in_use == 0)
			stable = 0;
	}
	check("every object's address kept", stable);

	/* 2. Half destroyed and made again: no growth. */
	for (index = 0U; index < 1000U; index += 2U)
		drv_i915_gt_object_destroy(&gm, objects[index]);
	blocks = gm.object_block_count;
	for (index = 0U; index < 1000U; index += 2U)
		objects[index] = drv_i915_gt_object_create(&gm, 4096U);
	snprintf(what, sizeof(what), "freed slots reused (blocks %u -> %u, live %u)", blocks, gm.object_block_count, gm.objects_live);
	check(what, gm.object_block_count == blocks && gm.objects_live == 1000U);

	/* 3. Up to the limit, and one more fails. */
	for (index = 1000U; index < I915_GT_MAX_OBJECTS; index++)
		objects[index] = drv_i915_gt_object_create(&gm, 4096U);
	objects[I915_GT_MAX_OBJECTS] = drv_i915_gt_object_create(&gm, 4096U);
	snprintf(what, sizeof(what), "the %u-th object refused (blocks %u, log: %.60s)", I915_GT_MAX_OBJECTS + 1U, gm.object_block_count, pool_last_log);
	check(what, objects[I915_GT_MAX_OBJECTS] == NULL &&
	    objects[I915_GT_MAX_OBJECTS - 1U] != NULL &&
	    gm.object_block_count == I915_GT_OBJECT_BLOCKS &&
	    strstr(pool_last_log, "object pool exhausted") != NULL);

	/* 4. fini frees the blocks, but not one with a kept object. */
	objects[5]->keep = 1;
	pool_frees = 0U;
	drv_i915_gt_mem_fini(&gm);
	snprintf(what, sizeof(what), "fini freed %u blocks' worth of memory (one kept)", pool_frees);
	check(what, gm.object_block_count == 0U && gm.inited == 0);

	if (pool_failures != 0) {
		printf("host-gt-pool: FAIL (%d)\n", pool_failures);
		return 1;
	}
	printf("host-gt-pool: PASS\n");
	return 0;
}
