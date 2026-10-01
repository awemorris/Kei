/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Presents three full-screen colors through our own API-1.0 KMS display swapchain. */
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <wayland-client.h>

/* One finite probe workload; all Vulkan and Wayland handles retire before the process succeeds. */
struct client_probe {
	VkDisplayKHR output;
	int master_fd;
	unsigned acquire;
	uint32_t width;
	uint32_t height;
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_surface *wayland_surface;
	VkInstance instance;
	VkPhysicalDevice physical;
	VkDevice device;
	VkQueue queue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	VkCommandPool pool;
	VkCommandBuffer command;
	VkSemaphore available;
	VkSemaphore rendered;
	VkFence fence;
	VkImage images[8];
	unsigned used[8];
	uint32_t image_count;
	uint32_t family;
	unsigned mailbox;
};

static VkResult client_setup(struct client_probe *probe);
static VkResult client_swapchain(struct client_probe *probe, uint32_t width, uint32_t height);
static VkResult client_frame(struct client_probe *probe, unsigned frame);
static void client_cleanup(struct client_probe *probe);
static int client_failed(struct client_probe *probe, VkResult error);

/* Runs both seat-descriptor and direct-root display ownership with bounded visible color intervals. */
int
main(
	int argc,
	char **argv)
{
	struct client_probe probe;
	VkResult error;
	unsigned frame;
	unsigned colors[3];
	struct timespec hold;
	int index;
	int evaluated;

	/* Initializes all partial-cleanup fields before any Vulkan or DRM call. */
	memset(&probe, 0, sizeof(probe));
	probe.master_fd = -1;
	for (index = 1; index < argc; index++) {
		/* The only option exercises the compositor seat's explicit descriptor ownership. */
		evaluated = strcmp(argv[index], "--acquire");
		if (evaluated != 0)
			return 2;

		/* Opens the card before Vulkan inquiry can drop automatic master on its separate file. */
		probe.acquire = 1;
	}

	/* Flushes state evidence before the five-second screenshot interval begins. */
	(void)setvbuf(stdout, NULL, _IOLBF, 0);
	error = client_setup(&probe);
	if (error != VK_SUCCESS)
		return client_failed(&probe, error);

	/* Uses the connector's preferred physical resolution for all four sampled pixels. */
	error = client_swapchain(&probe, probe.width, probe.height);
	if (error != VK_SUCCESS)
		return client_failed(&probe, error);

	/* The same N-modulo-three GPU clear operation drives each screen color. */
	colors[0] = 0xff0000;
	colors[1] = 0x00ff00;
	colors[2] = 0x0000ff;
	for (frame = 0; frame < 3; frame++) {
		/* Replaces a live chain before green to verify master ownership survives old-chain destruction. */
		if (frame == 1) {
			error = client_swapchain(&probe, probe.width, probe.height);
			if (error != VK_SUCCESS)
				return client_failed(&probe, error);
		}

		/* Rendering and presentation failures retain their numeric Vulkan result. */
		error = client_frame(&probe, frame);
		if (error != VK_SUCCESS)
			return client_failed(&probe, error);

		/* The host takes its QMP screenshot after observing this completed-present marker. */
		printf("DISPLAY color=%06x\n", colors[frame]);
		hold.tv_sec = 5;
		hold.tv_nsec = 0;
		(void)nanosleep(&hold, NULL);
	}

	/* Retires scanout and restores the acquired console before reporting success. */
	client_cleanup(&probe);
	printf("display-probe: PASS\n");

	/* Succeeded: every color reached KMS and all application resources retired. */
	return 0;
}

/* Creates an API-1.0 graphics device and ordinary reusable rendering synchronization. */
static VkResult
client_setup(
	struct client_probe *probe)
{
	VkApplicationInfo application;
	VkInstanceCreateInfo instance;
	VkDisplaySurfaceCreateInfoKHR surface;
	VkDisplayPropertiesKHR outputs[16];
	VkDisplayModePropertiesKHR modes[128];
	uint32_t mode_index;
	VkDeviceQueueCreateInfo queue;
	VkDeviceCreateInfo device;
	VkCommandPoolCreateInfo pool;
	VkCommandBufferAllocateInfo allocation;
	VkSemaphoreCreateInfo semaphore;
	VkFenceCreateInfo fence;
	VkQueueFamilyProperties *families;
	VkBool32 supported;
	VkResult error;
	const char *instance_extensions[4];
	const char *device_extension;
	uint32_t count;
	uint32_t index;
	float priority;
	int answer;

	/* Explicit seat ownership opens the master file before any Vulkan display inquiry. */
	if (probe->acquire != 0) {
		probe->master_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
		if (probe->master_fd < 0)
			return VK_ERROR_INITIALIZATION_FAILED;

		/* Inquiry must address the same primary card as the seat's master descriptor. */
		answer = setenv("KEILAND_DRM_DEVICE", "/dev/dri/card0", 1);
		if (answer != 0)
			return VK_ERROR_INITIALIZATION_FAILED;
	}

	/* Deliberately requests API 1.0 to exercise internal KHR dependency addition. */
	memset(&application, 0, sizeof(application));
	application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application.apiVersion = VK_API_VERSION_1_0;
	instance_extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
	instance_extensions[1] = VK_KHR_DISPLAY_EXTENSION_NAME;
	instance_extensions[2] = VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME;
	instance_extensions[3] = VK_EXT_ACQUIRE_DRM_DISPLAY_EXTENSION_NAME;
	memset(&instance, 0, sizeof(instance));
	instance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance.pApplicationInfo = &application;
	instance.enabledExtensionCount = 4;
	instance.ppEnabledExtensionNames = instance_extensions;
	error = vkCreateInstance(&instance, NULL, &probe->instance);
	fprintf(stderr, "SETUP instance result=%d\n", error);
	if (error != VK_SUCCESS)
		return error;

	/* Selects the first device from the isolated CPU-only host environment. */
	count = 1;
	error = vkEnumeratePhysicalDevices(probe->instance, &count, &probe->physical);
	if (error != VK_SUCCESS)
		return error;

	/* Enumerates the card independently of the CPU rendering driver's lack of display WSI. */
	count = 16;
	error = vkGetPhysicalDeviceDisplayPropertiesKHR(probe->physical, &count, outputs);
	if (error != VK_SUCCESS)
		return error;

	/* A real connected connector is required for visible display acceptance. */
	if (count == 0)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Uses the preferred physical resolution used by ordinary compositor startup. */
	probe->output = outputs[0].display;
	probe->width = outputs[0].physicalResolution.width;
	probe->height = outputs[0].physicalResolution.height;
	count = 128;
	error = vkGetDisplayModePropertiesKHR(probe->physical, probe->output, &count, modes);
	if (error != VK_SUCCESS)
		return error;

	/* Finds the exact preferred visible extent in the stable timing list. */
	for (mode_index = 0; mode_index < count; mode_index++) {
		/* Both timing dimensions must match the preferred connector resolution. */
		if (modes[mode_index].parameters.visibleRegion.width != probe->width)
			continue;

		/* A timing with matching height supplies the intended full-screen geometry. */
		if (modes[mode_index].parameters.visibleRegion.height == probe->height)
			break;
	}

	/* A preferred mode must exist in the queried connector's actual timing list. */
	if (mode_index == count)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Explicit acquisition transfers only a duplicate and remains releasable after the caller closes its file. */
	if (probe->acquire != 0) {
		error = vkAcquireDrmDisplayEXT(probe->physical, probe->master_fd, probe->output);
		if (error != VK_SUCCESS)
			return error;

		/* Closing the original proves that display presentation uses the library's own duplicate. */
		(void)close(probe->master_fd);
		probe->master_fd = -1;
	}

	/* The test's full-plane opaque surface matches the kernel-selected timing. */
	memset(&surface, 0, sizeof(surface));
	surface.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
	surface.displayMode = modes[mode_index].displayMode;
	surface.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	surface.globalAlpha = 1.0f;
	surface.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
	surface.imageExtent.width = probe->width;
	surface.imageExtent.height = probe->height;
	error = vkCreateDisplayPlaneSurfaceKHR(probe->instance, &surface, NULL, &probe->surface);
	if (error != VK_SUCCESS)
		return error;

	/* Records actual connector geometry and timing before any visible frame. */
	printf("DISPLAY name=%s width=%u height=%u refresh_mhz=%u\n", outputs[0].displayName, probe->width, probe->height, modes[mode_index].parameters.refreshRate);

	/* Measures graphics families before selecting a presentation-capable queue. */
	vkGetPhysicalDeviceQueueFamilyProperties(probe->physical, &count, NULL);
	families = calloc((size_t)count + 1, sizeof(*families));
	if (families == NULL)
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	/* Selects a graphics/export family using the public surface support query. */
	vkGetPhysicalDeviceQueueFamilyProperties(probe->physical, &count, families);
	for (index = 0; index < count; index++) {
		/* A transfer-only queue does not provide the surface's graphics contract. */
		if ((families[index].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
			continue;

		/* The support query must succeed and report a real DMA-BUF export path. */
		error = vkGetPhysicalDeviceSurfaceSupportKHR(probe->physical, index, probe->surface, &supported);
		if (error != VK_SUCCESS)
			break;

		/* Chooses only an actually supported graphics family. */
		if (supported != VK_FALSE)
			break;
	}

	/* Releases the temporary family list before creating the selected device. */
	free(families);
	if (index == count)
		return VK_ERROR_FEATURE_NOT_PRESENT;

	/* Preserves a query failure rather than treating it as queue support. */
	if (error != VK_SUCCESS)
		return error;

	/* Requests only the public swapchain extension, relying on the chain for its private dependencies. */
	probe->family = index;
	priority = 1.0f;
	memset(&queue, 0, sizeof(queue));
	queue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue.queueFamilyIndex = index;
	queue.queueCount = 1;
	queue.pQueuePriorities = &priority;
	device_extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	memset(&device, 0, sizeof(device));
	device.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device.queueCreateInfoCount = 1;
	device.pQueueCreateInfos = &queue;
	device.enabledExtensionCount = 1;
	device.ppEnabledExtensionNames = &device_extension;
	error = vkCreateDevice(probe->physical, &device, NULL, &probe->device);
	fprintf(stderr, "SETUP device result=%d\n", error);
	if (error != VK_SUCCESS)
		return error;

	/* Retrieves the backend queue through the ownership-recording interceptor. */
	vkGetDeviceQueue(probe->device, index, 0, &probe->queue);
	memset(&pool, 0, sizeof(pool));
	pool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool.queueFamilyIndex = index;
	error = vkCreateCommandPool(probe->device, &pool, NULL, &probe->pool);
	if (error != VK_SUCCESS)
		return error;

	/* One command buffer is reused only after its prior rendering fence completes. */
	memset(&allocation, 0, sizeof(allocation));
	allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocation.commandPool = probe->pool;
	allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocation.commandBufferCount = 1;
	error = vkAllocateCommandBuffers(probe->device, &allocation, &probe->command);
	if (error != VK_SUCCESS)
		return error;

	/* Creates independent acquire and rendering-completion binary semaphores. */
	memset(&semaphore, 0, sizeof(semaphore));
	semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	error = vkCreateSemaphore(probe->device, &semaphore, NULL, &probe->available);
	if (error != VK_SUCCESS)
		return error;

	/* The rendered semaphore is consumed by the public present call once per frame. */
	error = vkCreateSemaphore(probe->device, &semaphore, NULL, &probe->rendered);
	if (error != VK_SUCCESS)
		return error;

	/* An initially signaled fence allows the first checked command-buffer reuse. */
	memset(&fence, 0, sizeof(fence));
	fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	/* Returns the last resource allocation outcome. */
	return vkCreateFence(probe->device, &fence, NULL, &probe->fence);
}

/* Replaces a chain while keeping the old handle alive until successful new creation. */
static VkResult
client_swapchain(
	struct client_probe *probe,
	uint32_t width,
	uint32_t height)
{
	VkSwapchainCreateInfoKHR create;
	VkSwapchainKHR next;
	VkResult error;

	/* Drains rendering before retiring any image-specific application state. */
	error = vkDeviceWaitIdle(probe->device);
	if (error != VK_SUCCESS)
		return error;

	/* Chooses an advertised opaque-compatible ARGB layout and client-selected extent. */
	memset(&create, 0, sizeof(create));
	create.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create.surface = probe->surface;
	create.minImageCount = 2;
	create.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
	create.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	create.imageExtent.width = width;
	create.imageExtent.height = height;
	create.imageArrayLayers = 1;
	create.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	create.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	create.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	if (probe->mailbox != 0)
		create.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

	/* Supplies the existing chain so replacement semantics are exercised explicitly. */
	create.clipped = VK_TRUE;
	create.oldSwapchain = probe->swapchain;
	error = vkCreateSwapchainKHR(probe->device, &create, NULL, &next);
	if (error != VK_SUCCESS)
		return error;

	/* Old callback storage must remain valid even if its last release is still queued. */
	if (probe->swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(probe->device, probe->swapchain, NULL);

	/* Resets application layout tracking for the newly allocated images. */
	probe->swapchain = next;
	memset(probe->used, 0, sizeof(probe->used));
	probe->image_count = 8;

	/* Returns the public enumeration result for the newly selected extent. */
	return vkGetSwapchainImagesKHR(probe->device, next, &probe->image_count, probe->images);
}

/* Clears one acquired image to the deterministic N-modulo-three color and presents it. */
static VkResult
client_frame(
	struct client_probe *probe,
	unsigned frame)
{
	VkCommandBufferBeginInfo begin;
	VkImageMemoryBarrier barrier;
	VkImageSubresourceRange range;
	VkClearColorValue color;
	VkSubmitInfo submit;
	VkPresentInfoKHR present;
	VkPipelineStageFlags stage;
	VkResult error;
	VkResult individual;
	uint32_t index;

	/* Reuses the rendering command buffer only after its prior frame completes. */
	error = vkWaitForFences(probe->device, 1, &probe->fence, VK_TRUE, 5000000000ULL);
	if (error != VK_SUCCESS)
		return error;

	/* A finite acquire deadline exercises the private event queue's bounded release wait. */
	error = vkAcquireNextImageKHR(probe->device, probe->swapchain, 5000000000ULL, probe->available, VK_NULL_HANDLE, &index);
	if (error != VK_SUCCESS)
		return error;

	/* Resets the signaled render fence only after an image has actually been acquired. */
	error = vkResetFences(probe->device, 1, &probe->fence);
	if (error != VK_SUCCESS)
		return error;

	/* Resets one completed application command buffer. */
	error = vkResetCommandBuffer(probe->command, 0);
	if (error != VK_SUCCESS)
		return error;

	/* Starts explicit rendering into the unchanged backend image handle. */
	memset(&begin, 0, sizeof(begin));
	begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	error = vkBeginCommandBuffer(probe->command, &begin);
	if (error != VK_SUCCESS)
		return error;

	/* Moves the acquired image from its tracked prior layout into transfer-destination layout. */
	memset(&barrier, 0, sizeof(barrier));
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	if (probe->used[index] != 0)
		barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	/* Queue ownership has already returned through the acquire completion semaphore. */
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = probe->images[index];
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(probe->command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

	/* Frame zero is red, frame one green, and frame two blue, repeating exactly. */
	memset(&color, 0, sizeof(color));
	color.float32[frame % 3] = 1.0f;
	color.float32[3] = 1.0f;
	range = barrier.subresourceRange;
	vkCmdClearColorImage(probe->command, probe->images[index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);

	/* Publishes completed writes in the ordinary presentation layout. */
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	vkCmdPipelineBarrier(probe->command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
	error = vkEndCommandBuffer(probe->command);
	if (error != VK_SUCCESS)
		return error;

	/* Rendering waits for acquire completion and signals the application's present semaphore. */
	memset(&submit, 0, sizeof(submit));
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &probe->available;
	submit.pWaitDstStageMask = &stage;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &probe->command;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &probe->rendered;
	error = vkQueueSubmit(probe->queue, 1, &submit, probe->fence);
	if (error != VK_SUCCESS)
		return error;

	/* The public present call must return both aggregate and individual success. */
	memset(&present, 0, sizeof(present));
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = &probe->rendered;
	present.swapchainCount = 1;
	present.pSwapchains = &probe->swapchain;
	present.pImageIndices = &index;
	present.pResults = &individual;
	error = vkQueuePresentKHR(probe->queue, &present);
	if (error != VK_SUCCESS)
		return error;

	/* Per-swapchain failure cannot be hidden by an aggregate success. */
	if (individual != VK_SUCCESS)
		return individual;

	/* Tracks the image layout for its next acquisition. */
	probe->used[index] = 1;

	/* Succeeded: the probe server will independently read this frame's actual pixels and fences. */
	return VK_SUCCESS;
}

/* Retires every fully or partially initialized application object in dependency order. */
static void
client_cleanup(
	struct client_probe *probe)
{
	/* Completes outstanding GPU work before removing submission resources. */
	if (probe->device != VK_NULL_HANDLE)
		(void)vkDeviceWaitIdle(probe->device);

	/* Releases the chain before its surface and device lifetimes end. */
	if (probe->swapchain != VK_NULL_HANDLE)
		vkDestroySwapchainKHR(probe->device, probe->swapchain, NULL);

	/* Retires private Wayland callback storage while the backend device is still live. */
	if (probe->surface != VK_NULL_HANDLE)
		vkDestroySurfaceKHR(probe->instance, probe->surface, NULL);

	/* The rendering fence has no outstanding owner after device idle. */
	if (probe->fence != VK_NULL_HANDLE)
		vkDestroyFence(probe->device, probe->fence, NULL);

	/* Both application binary semaphores are retired after the final presentation wait. */
	if (probe->available != VK_NULL_HANDLE)
		vkDestroySemaphore(probe->device, probe->available, NULL);

	/* Rendering completion belongs only to the application until present consumes it. */
	if (probe->rendered != VK_NULL_HANDLE)
		vkDestroySemaphore(probe->device, probe->rendered, NULL);

	/* The pool also retires its one rendering command buffer. */
	if (probe->pool != VK_NULL_HANDLE)
		vkDestroyCommandPool(probe->device, probe->pool, NULL);

	/* Explicit display acquisition has its own lifetime, after all swapchain framebuffer resources retire. */
	if (probe->output != VK_NULL_HANDLE)
		(void)vkReleaseDisplayEXT(probe->physical, probe->output);

	/* Backend device ownership ends after all image and submission objects retire. */
	if (probe->device != VK_NULL_HANDLE)
		vkDestroyDevice(probe->device, NULL);

	/* No physical-device or private surface record remains in use. */
	if (probe->instance != VK_NULL_HANDLE)
		vkDestroyInstance(probe->instance, NULL);

	/* A setup failure may still own the caller's original master file. */
	if (probe->master_fd >= 0)
		(void)close(probe->master_fd);
}

/* Reports a failed frame or setup and retires every partially initialized application resource. */
static int
client_failed(
	struct client_probe *probe,
	VkResult error)
{
	/* Failure retains the numeric Vulkan outcome and retires all partial resources. */
	fprintf(stderr, "display-probe: FAIL result=%d\n", error);
	client_cleanup(probe);

	/* No failed or timed-out frame sequence may clear the Phase. */
	return 1;
}
