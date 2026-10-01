/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Exercise the real renderer against an independent recording observer. */
#include <assert.h>
#include "../../../userland/tests/vkdemo/renderer.c"

/* This single-threaded observer counts work and records its visibility contract. */
static unsigned draws;
static unsigned copies;
static unsigned host_reads;
static unsigned present_transitions;
static VkImageLayout pass_final;
static VkImageLayout present_previous;

VKAPI_ATTR VkResult VKAPI_CALL
vkBeginCommandBuffer(VkCommandBuffer command, const VkCommandBufferBeginInfo *info)
{
	(void)command;
	assert(info->flags == VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *info, const VkAllocationCallbacks *allocator, VkRenderPass *pass)
{
	(void)device;
	(void)allocator;
	assert(info->attachmentCount == 2);
	assert(info->pAttachments[1].format == VK_FORMAT_D32_SFLOAT);
	pass_final = info->pAttachments[0].finalLayout;
	*pass = (VkRenderPass)(uintptr_t)1;
	return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
vkCmdPipelineBarrier(VkCommandBuffer command, VkPipelineStageFlags source, VkPipelineStageFlags destination, VkDependencyFlags flags, uint32_t memory_count, const VkMemoryBarrier *memory, uint32_t buffer_count, const VkBufferMemoryBarrier *buffers, uint32_t image_count, const VkImageMemoryBarrier *images)
{
	uint32_t index;

	(void)command;
	(void)source;
	(void)destination;
	(void)flags;
	(void)memory_count;
	(void)memory;
	for (index = 0; index < buffer_count; index++) {
		if ((buffers[index].srcAccessMask | buffers[index].dstAccessMask) & VK_ACCESS_HOST_READ_BIT)
			host_reads++;
	}

	for (index = 0; index < image_count; index++) {
		if (images[index].newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
			present_transitions++;
			present_previous = images[index].oldLayout;
		}
	}
}

VKAPI_ATTR void VKAPI_CALL
vkCmdBeginRenderPass(VkCommandBuffer command, const VkRenderPassBeginInfo *info, VkSubpassContents contents)
{
	(void)command;
	(void)contents;
	assert(info->renderArea.extent.width == 320);
	assert(info->renderArea.extent.height == 240);
	assert(info->clearValueCount == 2);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdBindPipeline(VkCommandBuffer command, VkPipelineBindPoint point, VkPipeline pipeline)
{
	(void)command;
	(void)pipeline;
	assert(point == VK_PIPELINE_BIND_POINT_GRAPHICS);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdBindVertexBuffers(VkCommandBuffer command, uint32_t first, uint32_t count, const VkBuffer *buffers, const VkDeviceSize *offsets)
{
	(void)command;
	(void)buffers;
	assert(first == 0 && count == 1 && offsets[0] == 0);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdBindDescriptorSets(VkCommandBuffer command, VkPipelineBindPoint point, VkPipelineLayout layout, uint32_t first, uint32_t count, const VkDescriptorSet *sets, uint32_t offsets_count, const uint32_t *offsets)
{
	(void)command;
	(void)point;
	(void)layout;
	(void)sets;
	(void)offsets;
	assert(first == 0 && count == 1 && offsets_count == 0);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdPushConstants(VkCommandBuffer command, VkPipelineLayout layout, VkShaderStageFlags stages, uint32_t offset, uint32_t size, const void *values)
{
	(void)command;
	(void)layout;
	assert(stages == VK_SHADER_STAGE_VERTEX_BIT);
	assert(offset == 0 && size == sizeof(float));
	assert(*(const float *)values == 1.0f);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdDraw(VkCommandBuffer command, uint32_t vertices, uint32_t instances, uint32_t first_vertex, uint32_t first_instance)
{
	(void)command;
	assert(vertices == 36 && instances == 1);
	assert(first_vertex == 0 && first_instance == 0);
	draws++;
}

VKAPI_ATTR void VKAPI_CALL
vkCmdEndRenderPass(VkCommandBuffer command)
{
	(void)command;
}

VKAPI_ATTR void VKAPI_CALL
vkCmdCopyImageToBuffer(VkCommandBuffer command, VkImage image, VkImageLayout layout, VkBuffer buffer, uint32_t count, const VkBufferImageCopy *regions)
{
	(void)command;
	(void)image;
	(void)buffer;
	assert(layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && count == 1);
	assert(regions->imageExtent.width == 320 && regions->imageExtent.height == 240);
	copies++;
}

int
main(void)
{
	struct demo_target target;
	int error;

	memset(&renderer, 0, sizeof(renderer));
	memset(&target, 0, sizeof(target));
	renderer.targets = &target;
	renderer.color_format = VK_FORMAT_R8G8B8A8_UNORM;
	error = create_render_pass();
	assert(error == 0 && pass_final == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	error = record_frame(1000);
	assert(error == 0 && draws == 1 && copies == 0 && host_reads == 0);
	assert(present_transitions == 1 && present_previous == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	renderer.readback_enabled = 1;
	error = create_render_pass();
	assert(error == 0 && pass_final == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	error = record_frame(1000);
	assert(error == 0 && draws == 2 && copies == 1 && host_reads == 2);
	assert(present_transitions == 2 && present_previous == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	renderer.offscreen = 1;
	error = record_frame(1000);
	assert(error == 0 && draws == 3 && copies == 2 && host_reads == 4);
	assert(present_transitions == 2);
	puts("vkdemo recording: normal color-to-present without readback; diagnostic and offscreen GPU copies PASS");
	return 0;
}
