/* vkexp.c -- lavapipe dma-buf / DRM modifier / sync_file experiments (WS105 design check). */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <linux/dma-buf.h>
#include <vulkan/vulkan.h>
#include <wayland-client.h>
#include "linux-dmabuf-v1-client-protocol.h"

#define CK(x) do { VkResult r_ = (x); if (r_ != VK_SUCCESS) { printf("FAIL %s = %d (line %d)\n", #x, r_, __LINE__); exit(1); } } while (0)
#define W 64
#define H 32

static VkInstance inst;
static VkPhysicalDevice pd;

static PFN_vkGetMemoryFdKHR pGetMemoryFd;
static PFN_vkGetMemoryFdPropertiesKHR pGetMemoryFdProps;
static PFN_vkGetImageDrmFormatModifierPropertiesEXT pGetImgMod;
static PFN_vkGetSemaphoreFdKHR pGetSemFd;
static PFN_vkImportSemaphoreFdKHR pImportSemFd;
static PFN_vkGetFenceFdKHR pGetFenceFd;

static const char *dev_exts[] = {
    "VK_KHR_external_memory_fd", "VK_EXT_external_memory_dma_buf", "VK_EXT_image_drm_format_modifier",
    "VK_KHR_external_semaphore_fd", "VK_KHR_external_fence_fd", "VK_EXT_queue_family_foreign",
};

static void make_instance(void)
{
	VkApplicationInfo ai = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_3};
	VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &ai};
	CK(vkCreateInstance(&ici, NULL, &inst));
	uint32_t n = 8;
	VkPhysicalDevice pds[8];
	CK(vkEnumeratePhysicalDevices(inst, &n, pds));
	pd = pds[0];
}

static VkDevice make_device(VkQueue *q)
{
	float pr = 1;
	VkDeviceQueueCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueCount = 1, .pQueuePriorities = &pr};
	VkDeviceCreateInfo dci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci,
	                          .enabledExtensionCount = sizeof dev_exts / sizeof dev_exts[0], .ppEnabledExtensionNames = dev_exts};
	VkDevice d;
	CK(vkCreateDevice(pd, &dci, NULL, &d));
	vkGetDeviceQueue(d, 0, 0, q);
	pGetMemoryFd = (void *)vkGetDeviceProcAddr(d, "vkGetMemoryFdKHR");
	pGetMemoryFdProps = (void *)vkGetDeviceProcAddr(d, "vkGetMemoryFdPropertiesKHR");
	pGetImgMod = (void *)vkGetDeviceProcAddr(d, "vkGetImageDrmFormatModifierPropertiesEXT");
	pGetSemFd = (void *)vkGetDeviceProcAddr(d, "vkGetSemaphoreFdKHR");
	pImportSemFd = (void *)vkGetDeviceProcAddr(d, "vkImportSemaphoreFdKHR");
	pGetFenceFd = (void *)vkGetDeviceProcAddr(d, "vkGetFenceFdKHR");
	return d;
}

static void list_modifiers(VkFormat f, const char *name)
{
	VkDrmFormatModifierProperties2EXT m2[16]; memset(m2, 0, sizeof m2);
	VkDrmFormatModifierPropertiesList2EXT l2 = {.sType = VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_2_EXT, .drmFormatModifierCount = 16, .pDrmFormatModifierProperties = m2};
	VkDrmFormatModifierPropertiesEXT m[16];
	VkDrmFormatModifierPropertiesListEXT l = {.sType = VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT, .pNext = &l2, .drmFormatModifierCount = 16, .pDrmFormatModifierProperties = m};
	VkFormatProperties2 fp = {.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2, .pNext = &l};
	vkGetPhysicalDeviceFormatProperties2(pd, f, &fp);
	printf("format %s: linearTiling=0x%x optimal=0x%x modifiers=%u\n", name, fp.formatProperties.linearTilingFeatures,
	       fp.formatProperties.optimalTilingFeatures, l.drmFormatModifierCount);
	for (uint32_t i = 0; i < l.drmFormatModifierCount; i++)
		printf("  modifier 0x%016" PRIx64 " planes=%u tilingFeatures=0x%x (list2 count=%u features2=0x%" PRIx64 ")\n", m[i].drmFormatModifier,
		       m[i].drmFormatModifierPlaneCount, m[i].drmFormatModifierTilingFeatures, l2.drmFormatModifierCount, (uint64_t)m2[i].drmFormatModifierTilingFeatures);
	/* image format properties with modifier + external DMA_BUF */
	for (uint32_t i = 0; i < l.drmFormatModifierCount; i++) {
		VkPhysicalDeviceImageDrmFormatModifierInfoEXT mi = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT,
		                                                    .drmFormatModifier = m[i].drmFormatModifier, .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
		VkPhysicalDeviceExternalImageFormatInfo ei = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO, .pNext = &mi,
		                                              .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
		VkPhysicalDeviceImageFormatInfo2 ii = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2, .pNext = &ei, .format = f,
		                                       .type = VK_IMAGE_TYPE_2D, .tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT,
		                                       .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT};
		VkExternalImageFormatProperties eip = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES};
		VkImageFormatProperties2 ip = {.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, .pNext = &eip};
		VkResult r = vkGetPhysicalDeviceImageFormatProperties2(pd, &ii, &ip);
		printf("  imageFormatProps2(mod 0x%" PRIx64 ", DMA_BUF, color|sampled|xfer)=%d maxExtent=%ux%u extFeatures=0x%x export=0x%x compat=0x%x\n",
		       m[i].drmFormatModifier, r, ip.imageFormatProperties.maxExtent.width, ip.imageFormatProperties.maxExtent.height,
		       eip.externalMemoryProperties.externalMemoryFeatures, eip.externalMemoryProperties.exportFromImportedHandleTypes,
		       eip.externalMemoryProperties.compatibleHandleTypes);
	}
}

static uint32_t pick_mem(uint32_t bits, VkMemoryPropertyFlags want)
{
	VkPhysicalDeviceMemoryProperties mp;
	vkGetPhysicalDeviceMemoryProperties(pd, &mp);
	for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
		if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & want) == want)
			return i;
	return UINT32_MAX;
}

static VkCommandBuffer begin_cb(VkDevice d, VkCommandPool *pool)
{
	VkCommandPoolCreateInfo pci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	CK(vkCreateCommandPool(d, &pci, NULL, pool));
	VkCommandBufferAllocateInfo cai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = *pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1};
	VkCommandBuffer cb;
	CK(vkAllocateCommandBuffers(d, &cai, &cb));
	VkCommandBufferBeginInfo bi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	CK(vkBeginCommandBuffer(cb, &bi));
	return cb;
}

static void barrier(VkCommandBuffer cb, VkImage img, VkImageLayout from, VkImageLayout to, uint32_t srcQF, uint32_t dstQF)
{
	VkImageMemoryBarrier b = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
	                          .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, .oldLayout = from, .newLayout = to,
	                          .srcQueueFamilyIndex = srcQF, .dstQueueFamilyIndex = dstQF, .image = img,
	                          .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
	vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, 1, &b);
}

/* Import a dma-buf, copy it into a host buffer and return pixel(x,y). */
static void import_and_read(VkDevice d, VkQueue q, int fd, uint64_t mod, uint64_t offset, uint64_t stride, const char *tag)
{
	VkSubresourceLayout pl = {.offset = offset, .rowPitch = stride};
	VkImageDrmFormatModifierExplicitCreateInfoEXT ex = {.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT,
	                                                     .drmFormatModifier = mod, .drmFormatModifierPlaneCount = 1, .pPlaneLayouts = &pl};
	VkExternalMemoryImageCreateInfo emi = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, .pNext = &ex,
	                                       .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
	VkImageCreateInfo ic = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .pNext = &emi, .imageType = VK_IMAGE_TYPE_2D,
	                        .format = VK_FORMAT_B8G8R8A8_UNORM, .extent = {W, H, 1}, .mipLevels = 1, .arrayLayers = 1,
	                        .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT,
	                        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
	VkImage img;
	CK(vkCreateImage(d, &ic, NULL, &img));
	VkMemoryFdPropertiesKHR fp = {.sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR};
	VkResult r = pGetMemoryFdProps(d, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, fd, &fp);
	VkMemoryRequirements mr;
	vkGetImageMemoryRequirements(d, img, &mr);
	printf("[%s] vkGetMemoryFdPropertiesKHR=%d memoryTypeBits=0x%x; image req size=%" PRIu64 " bits=0x%x\n", tag, r, fp.memoryTypeBits, (uint64_t)mr.size, mr.memoryTypeBits);
	uint32_t mt = pick_mem(fp.memoryTypeBits & mr.memoryTypeBits, 0);
	int dfd = dup(fd);
	VkMemoryDedicatedAllocateInfo ded = {.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, .image = img};
	VkImportMemoryFdInfoKHR imp = {.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR, .pNext = &ded, .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, .fd = dfd};
	VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &imp, .allocationSize = mr.size, .memoryTypeIndex = mt};
	VkDeviceMemory mem;
	r = vkAllocateMemory(d, &mai, NULL, &mem);
	printf("[%s] import vkAllocateMemory(type %u)=%d; fcntl(dupfd)=%d after import (-1 => consumed/closed)\n", tag, mt, r, fcntl(dfd, F_GETFD));
	if (r != VK_SUCCESS)
		exit(1);
	CK(vkBindImageMemory(d, img, mem, 0));
	/* host buffer */
	VkBufferCreateInfo bc = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = W * H * 4, .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT};
	VkBuffer buf;
	CK(vkCreateBuffer(d, &bc, NULL, &buf));
	VkMemoryRequirements br;
	vkGetBufferMemoryRequirements(d, buf, &br);
	VkMemoryAllocateInfo bai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .allocationSize = br.size,
	                            .memoryTypeIndex = pick_mem(br.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};
	VkDeviceMemory bmem;
	CK(vkAllocateMemory(d, &bai, NULL, &bmem));
	CK(vkBindBufferMemory(d, buf, bmem, 0));
	VkCommandPool pool;
	VkCommandBuffer cb = begin_cb(d, &pool);
	/* acquire from foreign queue family, keeping contents (GENERAL -> TRANSFER_SRC) */
	barrier(cb, img, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_QUEUE_FAMILY_FOREIGN_EXT, 0);
	VkBufferImageCopy cp = {.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, .imageExtent = {W, H, 1}};
	vkCmdCopyImageToBuffer(cb, img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf, 1, &cp);
	CK(vkEndCommandBuffer(cb));
	VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cb};
	CK(vkQueueSubmit(q, 1, &si, VK_NULL_HANDLE));
	CK(vkQueueWaitIdle(q));
	uint32_t *p;
	CK(vkMapMemory(d, bmem, 0, VK_WHOLE_SIZE, 0, (void **)&p));
	printf("[%s] copy-back pixel(0,0)=0x%08x pixel(%d,%d)=0x%08x pixel(%d,%d)=0x%08x\n", tag, p[0], W / 2, H / 2, p[(H / 2) * W + W / 2], W - 1, H - 1, p[W * H - 1]);
	vkUnmapMemory(d, bmem);
	vkDestroyCommandPool(d, pool, NULL);
	vkDestroyBuffer(d, buf, NULL);
	vkFreeMemory(d, bmem, NULL);
	vkDestroyImage(d, img, NULL);
	vkFreeMemory(d, mem, NULL);
}

static int poll_ms(int fd, int ms)
{
	struct pollfd p = {.fd = fd, .events = POLLIN};
	int r = poll(&p, 1, ms);
	return r < 0 ? -errno : (r == 0 ? 0 : p.revents);
}

static void ext_caps(void)
{
	static const struct { VkExternalSemaphoreHandleTypeFlagBits t; const char *n; } st[] = {
	    {VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT, "OPAQUE_FD"}, {VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT, "SYNC_FD"}};
	for (int i = 0; i < 2; i++) {
		VkPhysicalDeviceExternalSemaphoreInfo si = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO, .handleType = st[i].t};
		VkExternalSemaphoreProperties sp = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES};
		vkGetPhysicalDeviceExternalSemaphoreProperties(pd, &si, &sp);
		printf("semaphore(binary) %s: features=0x%x (1=export 2=import) exportFrom=0x%x compat=0x%x\n", st[i].n, sp.externalSemaphoreFeatures,
		       sp.exportFromImportedHandleTypes, sp.compatibleHandleTypes);
		VkPhysicalDeviceExternalFenceInfo fi = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FENCE_INFO, .handleType = i ? VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT : VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT};
		VkExternalFenceProperties fpp = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_FENCE_PROPERTIES};
		vkGetPhysicalDeviceExternalFenceProperties(pd, &fi, &fpp);
		printf("fence %s: features=0x%x exportFrom=0x%x compat=0x%x\n", st[i].n, fpp.externalFenceFeatures, fpp.exportFromImportedHandleTypes, fpp.compatibleHandleTypes);
	}
	VkPhysicalDeviceExternalBufferInfo bi = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO, .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                                         .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
	VkExternalBufferProperties bp = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES};
	vkGetPhysicalDeviceExternalBufferProperties(pd, &bi, &bp);
	printf("buffer DMA_BUF: features=0x%x compat=0x%x\n", bp.externalMemoryProperties.externalMemoryFeatures, bp.externalMemoryProperties.compatibleHandleTypes);
	VkPhysicalDeviceMemoryProperties mp;
	vkGetPhysicalDeviceMemoryProperties(pd, &mp);
	for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
		printf("memoryType[%u] flags=0x%x heap=%u\n", i, mp.memoryTypes[i].propertyFlags, mp.memoryTypes[i].heapIndex);
}


static struct wl_compositor *comp;
static struct zwp_linux_dmabuf_v1 *dmabuf;
static void reg_global(void *d, struct wl_registry *r, uint32_t name, const char *iface, uint32_t v)
{
	(void)d;
	if (!strcmp(iface, "wl_compositor")) comp = wl_registry_bind(r, name, &wl_compositor_interface, 4);
	if (!strcmp(iface, "zwp_linux_dmabuf_v1")) dmabuf = wl_registry_bind(r, name, &zwp_linux_dmabuf_v1_interface, v < 3 ? v : 3);
}
static void reg_remove(void *d, struct wl_registry *r, uint32_t n) { (void)d, (void)r, (void)n; }
static const struct wl_registry_listener reg_l = {reg_global, reg_remove};
static void dm_format(void *d, struct zwp_linux_dmabuf_v1 *z, uint32_t f) { (void)d, (void)z; printf("client: format 0x%x\n", f); }
static void dm_modifier(void *d, struct zwp_linux_dmabuf_v1 *z, uint32_t f, uint32_t hi, uint32_t lo) { (void)d, (void)z; printf("client: modifier fmt=0x%x mod=0x%llx\n", f, (unsigned long long)hi << 32 | lo); }
static const struct zwp_linux_dmabuf_v1_listener dm_l = {dm_format, dm_modifier};
static int released;
static void buf_release(void *d, struct wl_buffer *b) { (void)d, (void)b; released = 1; }
static const struct wl_buffer_listener buf_l = {buf_release};

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	struct wl_display *dpy = wl_display_connect(argc > 1 ? argv[1] : "wl-probe");
	if (!dpy) { perror("connect"); return 1; }
	struct wl_registry *reg = wl_display_get_registry(dpy);
	wl_registry_add_listener(reg, &reg_l, NULL);
	wl_display_roundtrip(dpy);
	zwp_linux_dmabuf_v1_add_listener(dmabuf, &dm_l, NULL);
	wl_display_roundtrip(dpy);
	struct wl_surface *surf = wl_compositor_create_surface(comp);
	make_instance();
	VkQueue q;
	VkDevice d = make_device(&q);
	uint64_t mods[1] = {0};
	VkImageDrmFormatModifierListCreateInfoEXT ml = {.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT, .drmFormatModifierCount = 1, .pDrmFormatModifiers = mods};
	VkExternalMemoryImageCreateInfo emi = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, .pNext = &ml, .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
	VkImageCreateInfo ic = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .pNext = &emi, .imageType = VK_IMAGE_TYPE_2D, .format = VK_FORMAT_B8G8R8A8_UNORM,
	                        .extent = {W, H, 1}, .mipLevels = 1, .arrayLayers = 1, .samples = VK_SAMPLE_COUNT_1_BIT,
	                        .tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT, .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};
	VkImage img; CK(vkCreateImage(d, &ic, NULL, &img));
	VkMemoryRequirements mr; vkGetImageMemoryRequirements(d, img, &mr);
	VkMemoryDedicatedAllocateInfo ded = {.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, .image = img};
	VkExportMemoryAllocateInfo exa = {.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO, .pNext = &ded, .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
	VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &exa, .allocationSize = mr.size, .memoryTypeIndex = 0};
	VkDeviceMemory mem; CK(vkAllocateMemory(d, &mai, NULL, &mem)); CK(vkBindImageMemory(d, img, mem, 0));
	VkMemoryGetFdInfoKHR gfi = {.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR, .memory = mem, .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
	int fd; CK(pGetMemoryFd(d, &gfi, &fd));
	VkImageSubresource sub = {VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT, 0, 0};
	VkSubresourceLayout lay; vkGetImageSubresourceLayout(d, img, &sub, &lay);
	VkExportSemaphoreCreateInfo esc = {.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO, .handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};
	VkSemaphoreCreateInfo sci = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &esc};
	VkSemaphore sem; CK(vkCreateSemaphore(d, &sci, NULL, &sem));
	VkImageSubresourceRange rng = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	float cols[3][4] = {{1, 0, 0, 1}, {0, 1, 0, 1}, {0, 0, 1, 1}};
	for (int f = 0; f < 3; f++) {
		VkCommandPool pool; VkCommandBuffer cb = begin_cb(d, &pool);
		barrier(cb, img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED);
		VkClearColorValue cc; memcpy(cc.float32, cols[f], sizeof cc.float32);
		vkCmdClearColorImage(cb, img, VK_IMAGE_LAYOUT_GENERAL, &cc, 1, &rng);
		barrier(cb, img, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, 0, VK_QUEUE_FAMILY_FOREIGN_EXT);
		CK(vkEndCommandBuffer(cb));
		VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cb, .signalSemaphoreCount = 1, .pSignalSemaphores = &sem};
		CK(vkQueueSubmit(q, 1, &si, VK_NULL_HANDLE));
		VkSemaphoreGetFdInfoKHR sg = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR, .semaphore = sem, .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};
		int sfd; CK(pGetSemFd(d, &sg, &sfd));
		struct dma_buf_import_sync_file is = {.flags = DMA_BUF_SYNC_WRITE, .fd = sfd};
		int ir = ioctl(fd, DMA_BUF_IOCTL_IMPORT_SYNC_FILE, &is);
		close(sfd);
		struct zwp_linux_buffer_params_v1 *pr = zwp_linux_dmabuf_v1_create_params(dmabuf);
		zwp_linux_buffer_params_v1_add(pr, fd, 0, lay.offset, lay.rowPitch, 0, 0);
		struct wl_buffer *wb = zwp_linux_buffer_params_v1_create_immed(pr, W, H, 0x34325241, 0);
		zwp_linux_buffer_params_v1_destroy(pr);
		wl_buffer_add_listener(wb, &buf_l, NULL);
		released = 0;
		wl_surface_attach(surf, wb, 0, 0);
		wl_surface_damage_buffer(surf, 0, 0, W, H);
		wl_surface_commit(surf);
		wl_display_flush(dpy);
		while (!released && wl_display_dispatch(dpy) >= 0) {}
		printf("client: frame %d import_sync=%d released\n", f, ir);
		wl_buffer_destroy(wb);
		CK(vkQueueWaitIdle(q));
		vkDestroyCommandPool(d, pool, NULL);
	}
	wl_display_roundtrip(dpy);
	return 0;
}
