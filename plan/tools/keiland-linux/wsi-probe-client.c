/*
 * zedBSD
 * Copyright (C) 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/* Renders a bounded three-color sequence through our public API-1.0 Wayland swapchain. */
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

static void client_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
static void client_remove(void *data, struct wl_registry *registry, uint32_t name);
static VkResult client_setup(struct client_probe *probe);
static VkResult client_swapchain(struct client_probe *probe, uint32_t width, uint32_t height);
static VkResult client_frame(struct client_probe *probe, unsigned frame);
static void client_cleanup(struct client_probe *probe);
static uint64_t client_time(void);
static int client_failed(struct client_probe *probe, VkResult error);

/*
 * Runs the finite public WSI workload and reports PASS only after checked GPU and protocol operations.
 */
int
main(
	int argc,
	char **argv)
{
	int exit_status;
	struct client_probe probe;
	unsigned frames;
	unsigned timeout;
	unsigned resize;
	unsigned frame;
	int index;
	VkResult error;
	uint64_t start;
	int evaluated;
	uint64_t measured_time;

	/* Initializes ownership before parsing the finite acceptance options. */
	memset(&probe, 0, sizeof(probe));
	frames = 90;
	timeout = 60;
	resize = 0;
	for (index = 1; index < argc; index++) {
		/* MAILBOX changes the advertised present mode without changing frame colors. */
		evaluated = strcmp(argv[index], "--mailbox");
		if (evaluated == 0) {
			probe.mailbox = 1;
			continue;
		}

		/* Resize recreates the swapchain every thirty frames with the prior handle supplied. */
		evaluated = strcmp(argv[index], "--resize");
		if (evaluated == 0) {
			resize = 1;
			continue;
		}

		/* Value-taking options require a following argument. */
		if (index + 1 >= argc)
			return 2;

		/* Selects the finite frame workload. */
		evaluated = strcmp(argv[index], "--frames");
		if (evaluated == 0) {
			frames = (unsigned)strtoul(argv[++index], NULL, 10);
			continue;
		}

		/* Selects the checked wall-clock deadline in seconds. */
		evaluated = strcmp(argv[index], "--timeout");
		if (evaluated == 0) {
			timeout = (unsigned)strtoul(argv[++index], NULL, 10);
			continue;
		}

		/* Unknown options cannot silently modify acceptance behavior. */
		return 2;
	}

	/* The finite positive workload must supply actual frame evidence. */
	if (frames == 0)
		return 2;

	/* Prevents unreasonable timeout arithmetic in the acceptance client. */
	if (timeout > 600)
		return 2;

	/* Rejects a deadline that cannot allow any initialization. */
	if (timeout == 0)
		return 2;

	/* Checks setup through the same API version and extension rewrite used by ordinary applications. */
	start = client_time();
	error = client_setup(&probe);
	if (error != VK_SUCCESS) {
		exit_status = client_failed(&probe, error);

		/* Preserves the reported failure after its cleanup. */
		return exit_status;
	}

	/* Builds the initial client-selected extent. */
	error = client_swapchain(&probe, 320, 240);
	if (error != VK_SUCCESS) {
		exit_status = client_failed(&probe, error);

		/* Preserves the reported failure after its cleanup. */
		return exit_status;
	}

	/* Every frame renders to the actual exported Vulkan image. */
	for (frame = 0; frame < frames; frame++) {
		/* Enforces the one original deadline across resize and rendering. */
		measured_time = client_time();
		if (measured_time - start >= (uint64_t)timeout * 1000000000ULL) {
			error = VK_TIMEOUT;
			exit_status = client_failed(&probe, error);

			/* Preserves the reported failure after its cleanup. */
			return exit_status;
		}

		/* Alternates client-selected extents without waiting for compositor configure events. */
		if (resize != 0) {
			if (frame != 0) {
				if (frame % 30 == 0) {
					/* Odd intervals grow, and even intervals restore the original extent. */
					if ((frame / 30) % 2 != 0)
						error = client_swapchain(&probe, 400, 300);
					else
						error = client_swapchain(&probe, 320, 240);

					/* Resize failure ends acceptance without rewriting the expected color sequence. */
					if (error != VK_SUCCESS) {
						exit_status = client_failed(&probe, error);

						/* Preserves the reported failure after its cleanup. */
						return exit_status;
					}
				}
			}
		}

		/* Checks acquire, clear, submission and per-swapchain presentation result. */
		error = client_frame(&probe, frame);
		if (error != VK_SUCCESS) {
			exit_status = client_failed(&probe, error);

			/* Preserves the reported failure after its cleanup. */
			return exit_status;
		}
	}

	/* Waits for the last GPU submission before reporting the finite workload complete. */
	error = vkDeviceWaitIdle(probe.device);
	if (error != VK_SUCCESS) {
		exit_status = client_failed(&probe, error);

		/* Preserves the reported failure after its cleanup. */
		return exit_status;
	}

	/* Retires all application resources before claiming success. */
	client_cleanup(&probe);
	printf("wsi-probe-client: PASS frames=%u api=1.0\n", frames);

	/* Succeeded: every expected frame used the public Wayland WSI boundary. */
	return 0;
}

/* Constructs the application's core compositor without binding the WSI's private DMA-BUF factory. */
static void
client_global(
	void *data,
	struct wl_registry *registry,
	uint32_t name,
	const char *interface,
	uint32_t version)
{
	struct client_probe *probe;
	int evaluated;

	/* Only the application's wl_surface comes from this default event queue. */
	probe = data;
	evaluated = strcmp(interface, "wl_compositor");
	if (evaluated != 0)
		return;

	/* Buffer damage requires version four in this acceptance client. */
	if (version < 4)
		return;

	/* Binds exactly the core request version understood by the independent test server. */
	probe->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);

	/* Succeeded: the client retained any supported advertised interface. */
	return;
}

/* Bound globals remain usable until the test connection ends. */
static void
client_remove(
	void *data,
	struct wl_registry *registry,
	uint32_t name)
{
	/* The finite test server does not remove its compositor global. */
	(void)data;
	(void)registry;
	(void)name;

	/* Succeeded: the fixture retains no per-global removal state. */
	return;
}

/* Creates an API-1.0 graphics device and ordinary reusable rendering synchronization. */
static VkResult
client_setup(
	struct client_probe *probe)
{
	VkResult submission;

	/* Registry callbacks construct only the application's own core compositor object. */
	static const struct wl_registry_listener client_registry_listener = {
	    client_global, client_remove};
	VkApplicationInfo application;
	VkInstanceCreateInfo instance;
	VkWaylandSurfaceCreateInfoKHR surface;
	VkDeviceQueueCreateInfo queue;
	VkDeviceCreateInfo device;
	VkCommandPoolCreateInfo pool;
	VkCommandBufferAllocateInfo allocation;
	VkSemaphoreCreateInfo semaphore;
	VkFenceCreateInfo fence;
	VkQueueFamilyProperties *families;
	VkBool32 supported;
	VkResult error;
	const char *instance_extensions[2];
	const char *device_extension;
	uint32_t count;
	uint32_t index;
	float priority;
	int answer;

	/* Connects only to the explicitly selected private test socket. */
	probe->display = wl_display_connect(NULL);
	if (probe->display == NULL)
		return VK_ERROR_SURFACE_LOST_KHR;

	/* The outer timeout bounds the application's ordinary registry roundtrip. */
	probe->registry = wl_display_get_registry(probe->display);
	if (probe->registry == NULL)
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	/* Installs the core compositor callback before receiving globals. */
	answer = wl_registry_add_listener(probe->registry, &client_registry_listener, probe);
	if (answer != 0)
		return VK_ERROR_INITIALIZATION_FAILED;

	/* Receives only application core globals before creating the Vulkan surface. */
	answer = wl_display_roundtrip(probe->display);
	if (answer < 0)
		return VK_ERROR_SURFACE_LOST_KHR;

	/* A missing core compositor cannot provide an application surface. */
	if (probe->compositor == NULL)
		return VK_ERROR_SURFACE_LOST_KHR;

	/* Owns the wl_surface separately from every private WSI proxy. */
	probe->wayland_surface = wl_compositor_create_surface(probe->compositor);
	if (probe->wayland_surface == NULL)
		return VK_ERROR_OUT_OF_HOST_MEMORY;

	/* Deliberately requests API 1.0 to exercise internal KHR dependency addition. */
	memset(&application, 0, sizeof(application));
	application.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application.apiVersion = VK_API_VERSION_1_0;
	instance_extensions[0] = VK_KHR_SURFACE_EXTENSION_NAME;
	instance_extensions[1] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
	memset(&instance, 0, sizeof(instance));
	instance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance.pApplicationInfo = &application;
	instance.enabledExtensionCount = 2;
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

	/* Builds our surface through the public own-function procedure lookup. */
	memset(&surface, 0, sizeof(surface));
	surface.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	surface.display = probe->display;
	surface.surface = probe->wayland_surface;
	error = vkCreateWaylandSurfaceKHR(probe->instance, &surface, NULL, &probe->surface);
	if (error != VK_SUCCESS)
		return error;

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
	submission = vkCreateFence(probe->device, &fence, NULL, &probe->fence);
	if (submission != VK_SUCCESS)
		return submission;

	/* Succeeded: the client owns its reusable rendering synchronization. */
	return VK_SUCCESS;
}

/* Replaces a chain while keeping the old handle alive until successful new creation. */
static VkResult
client_swapchain(
	struct client_probe *probe,
	uint32_t width,
	uint32_t height)
{
	VkResult submission;
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
	submission = vkGetSwapchainImagesKHR(probe->device, next, &probe->image_count, probe->images);
	if (submission != VK_SUCCESS)
		return submission;

	/* Succeeded: the client retained the new swapchain image handles. */
	return VK_SUCCESS;
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

	/* Backend device ownership ends after all image and submission objects retire. */
	if (probe->device != VK_NULL_HANDLE)
		vkDestroyDevice(probe->device, NULL);

	/* No physical-device or private surface record remains in use. */
	if (probe->instance != VK_NULL_HANDLE)
		vkDestroyInstance(probe->instance, NULL);

	/* WSI wrappers never destroyed this application-owned core surface. */
	if (probe->wayland_surface != NULL)
		wl_surface_destroy(probe->wayland_surface);

	/* Application core globals retire before disconnecting the socket. */
	if (probe->compositor != NULL)
		wl_compositor_destroy(probe->compositor);

	/* The application's registry is independent of the WSI's private registry. */
	if (probe->registry != NULL)
		wl_registry_destroy(probe->registry);

	/* Disconnect closes all application transport state. */
	if (probe->display != NULL)
		wl_display_disconnect(probe->display);
}

/* Returns monotonic nanoseconds for the one finite client workload deadline. */
static uint64_t
client_time(
	void)
{
	struct timespec now;
	int error;

	/* Failed clock queries cannot support a meaningful deadline. */
	error = clock_gettime(CLOCK_MONOTONIC, &now);
	if (error != 0)
		return 0;

	/* Uses a wall-clock-independent timebase shared across all frames. */
	return (uint64_t)now.tv_sec * 1000000000ULL + (uint64_t)now.tv_nsec;
}

/* Reports a failed frame or setup and retires every partially initialized application resource. */
static int
client_failed(
	struct client_probe *probe,
	VkResult error)
{
	/* Failure retains the numeric Vulkan outcome and retires all partial resources. */
	fprintf(stderr, "wsi-probe-client: FAIL result=%d\n", error);
	client_cleanup(probe);

	/* No failed or timed-out frame sequence may clear the Phase. */
	return 1;
}
