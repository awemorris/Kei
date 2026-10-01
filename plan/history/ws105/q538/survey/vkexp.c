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

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	make_instance();
	if (argc == 6 && !strcmp(argv[1], "import")) { /* child process: import fd mod offset stride */
		VkQueue q;
		VkDevice d = make_device(&q);
		import_and_read(d, q, atoi(argv[2]), strtoull(argv[3], 0, 0), strtoull(argv[4], 0, 0), strtoull(argv[5], 0, 0), "process2");
		vkDestroyDevice(d, NULL);
		vkDestroyInstance(inst, NULL);
		return 0;
	}
	VkPhysicalDeviceProperties pp;
	vkGetPhysicalDeviceProperties(pd, &pp);
	printf("device: %s api %u.%u.%u\n", pp.deviceName, VK_API_VERSION_MAJOR(pp.apiVersion), VK_API_VERSION_MINOR(pp.apiVersion), VK_API_VERSION_PATCH(pp.apiVersion));
	list_modifiers(VK_FORMAT_B8G8R8A8_UNORM, "B8G8R8A8_UNORM");
	list_modifiers(VK_FORMAT_R8G8B8A8_UNORM, "R8G8B8A8_UNORM");
	list_modifiers(VK_FORMAT_B8G8R8A8_SRGB, "B8G8R8A8_SRGB");
	ext_caps();

	VkQueue q;
	VkDevice d = make_device(&q);
	/* ---- 1. export image ---- */
	uint64_t mods[1] = {0 /* DRM_FORMAT_MOD_LINEAR */};
	VkImageDrmFormatModifierListCreateInfoEXT ml = {.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT, .drmFormatModifierCount = 1, .pDrmFormatModifiers = mods};
	VkExternalMemoryImageCreateInfo emi = {.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO, .pNext = &ml, .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
	VkImageCreateInfo ic = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .pNext = &emi, .imageType = VK_IMAGE_TYPE_2D, .format = VK_FORMAT_B8G8R8A8_UNORM,
	                        .extent = {W, H, 1}, .mipLevels = 1, .arrayLayers = 1, .samples = VK_SAMPLE_COUNT_1_BIT,
	                        .tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT,
	                        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED};
	VkImage img;
	VkResult r = vkCreateImage(d, &ic, NULL, &img);
	printf("vkCreateImage(DRM_FORMAT_MODIFIER tiling, list{LINEAR}, DMA_BUF) = %d\n", r);
	if (r)
		return 1;
	VkMemoryDedicatedRequirements dr = {.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
	VkMemoryRequirements2 mr2 = {.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, .pNext = &dr};
	VkImageMemoryRequirementsInfo2 mri = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2, .image = img};
	vkGetImageMemoryRequirements2(d, &mri, &mr2);
	printf("image req size=%" PRIu64 " align=%" PRIu64 " bits=0x%x prefersDedicated=%u requiresDedicated=%u\n", (uint64_t)mr2.memoryRequirements.size,
	       (uint64_t)mr2.memoryRequirements.alignment, mr2.memoryRequirements.memoryTypeBits, dr.prefersDedicatedAllocation, dr.requiresDedicatedAllocation);
	VkMemoryDedicatedAllocateInfo ded = {.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, .image = img};
	VkExportMemoryAllocateInfo exa = {.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO, .pNext = &ded, .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
	uint32_t mt = pick_mem(mr2.memoryRequirements.memoryTypeBits, 0);
	VkMemoryAllocateInfo mai = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = &exa, .allocationSize = mr2.memoryRequirements.size, .memoryTypeIndex = mt};
	VkDeviceMemory mem;
	r = vkAllocateMemory(d, &mai, NULL, &mem);
	printf("vkAllocateMemory(export DMA_BUF + dedicated, type %u) = %d\n", mt, r);
	if (r)
		return 1;
	CK(vkBindImageMemory(d, img, mem, 0));
	VkImageDrmFormatModifierPropertiesEXT imp = {.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_PROPERTIES_EXT};
	r = pGetImgMod(d, img, &imp);
	VkImageSubresource sub = {VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT, 0, 0};
	VkSubresourceLayout lay;
	vkGetImageSubresourceLayout(d, img, &sub, &lay);
	printf("vkGetImageDrmFormatModifierPropertiesEXT=%d modifier=0x%" PRIx64 "; layout(MEMORY_PLANE_0) offset=%" PRIu64 " size=%" PRIu64 " rowPitch=%" PRIu64 " arrayPitch=%" PRIu64 " depthPitch=%" PRIu64 "\n",
	       r, imp.drmFormatModifier, (uint64_t)lay.offset, (uint64_t)lay.size, (uint64_t)lay.rowPitch, (uint64_t)lay.arrayPitch, (uint64_t)lay.depthPitch);
	VkMemoryGetFdInfoKHR gfi = {.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR, .memory = mem, .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT};
	int fd = -1;
	r = pGetMemoryFd(d, &gfi, &fd);
	char link[256] = "";
	char path[64];
	snprintf(path, sizeof path, "/proc/self/fd/%d", fd);
	ssize_t ln = readlink(path, link, sizeof link - 1);
	if (ln > 0)
		link[ln] = 0;
	off_t sz = lseek(fd, 0, SEEK_END);
	printf("vkGetMemoryFdKHR(DMA_BUF)=%d fd=%d CLOEXEC=%d -> %s lseek(SEEK_END)=%lld\n", r, fd, fcntl(fd, F_GETFD) & FD_CLOEXEC, link, (long long)sz);
	lseek(fd, 0, SEEK_SET);
	{
		char fi[512];
		snprintf(path, sizeof path, "/proc/self/fdinfo/%d", fd);
		FILE *f = fopen(path, "r");
		size_t n = f ? fread(fi, 1, sizeof fi - 1, f) : 0;
		fi[n] = 0;
		if (f)
			fclose(f);
		printf("fdinfo:\n%s", fi);
	}
	uint32_t *map = mmap(NULL, sz, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	printf("mmap(dmabuf fd)=%s%s\n", map == MAP_FAILED ? "FAILED " : "ok", map == MAP_FAILED ? strerror(errno) : "");

	/* clear with export semaphore SYNC_FD */
	VkExportSemaphoreCreateInfo esc = {.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO, .handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};
	VkSemaphoreCreateInfo sci = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &esc};
	VkSemaphore sem;
	CK(vkCreateSemaphore(d, &sci, NULL, &sem));
	VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	VkFence fence;
	CK(vkCreateFence(d, &fci, NULL, &fence));
	VkCommandPool pool;
	VkCommandBuffer cb = begin_cb(d, &pool);
	barrier(cb, img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED);
	VkClearColorValue cc = {.float32 = {1.0f, 0.25f, 0.5f, 1.0f}}; /* R G B A -> BGRA bytes 80 40 FF FF -> 0xFFFF4080 */
	VkImageSubresourceRange rng = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	vkCmdClearColorImage(cb, img, VK_IMAGE_LAYOUT_GENERAL, &cc, 1, &rng);
	barrier(cb, img, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, 0, VK_QUEUE_FAMILY_FOREIGN_EXT); /* release to foreign */
	CK(vkEndCommandBuffer(cb));
	VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cb, .signalSemaphoreCount = 1, .pSignalSemaphores = &sem};
	CK(vkQueueSubmit(q, 1, &si, fence));
	VkSemaphoreGetFdInfoKHR sgi = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR, .semaphore = sem, .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};
	int sfd = -2;
	r = pGetSemFd(d, &sgi, &sfd);
	printf("after clear submit: vkGetSemaphoreFdKHR(SYNC_FD)=%d fd=%d", r, sfd);
	if (sfd >= 0) {
		snprintf(path, sizeof path, "/proc/self/fd/%d", sfd);
		ln = readlink(path, link, sizeof link - 1);
		link[ln > 0 ? ln : 0] = 0;
		printf(" -> %s poll(1000ms)=0x%x", link, poll_ms(sfd, 1000));
	}
	printf("\n");
	CK(vkWaitForFences(d, 1, &fence, VK_TRUE, UINT64_MAX));
	if (map != MAP_FAILED) {
		uint32_t pitch = lay.rowPitch / 4;
		printf("mmap pixels after fence: (0,0)=0x%08x (%d,%d)=0x%08x (%d,%d)=0x%08x\n", map[lay.offset / 4], W / 2, H / 2,
		       map[lay.offset / 4 + (H / 2) * pitch + W / 2], W - 1, H - 1, map[lay.offset / 4 + (H - 1) * pitch + W - 1]);
	}

	/* ---- 3. sync_file ioctls on the dma-buf ---- */
	if (sfd >= 0) {
		struct dma_buf_import_sync_file is = {.flags = DMA_BUF_SYNC_WRITE, .fd = sfd};
		int ir = ioctl(fd, DMA_BUF_IOCTL_IMPORT_SYNC_FILE, &is);
		printf("IMPORT_SYNC_FILE(WRITE, semaphore sync_file) = %d errno=%d (%s)\n", ir, ir ? errno : 0, ir ? strerror(errno) : "");
	}
	for (int k = 0; k < 2; k++) {
		struct dma_buf_export_sync_file es = {.flags = k ? DMA_BUF_SYNC_WRITE : DMA_BUF_SYNC_READ, .fd = -1};
		int er = ioctl(fd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &es);
		printf("EXPORT_SYNC_FILE(%s) = %d errno=%d fd=%d", k ? "WRITE" : "READ", er, er ? errno : 0, es.fd);
		if (es.fd >= 0) {
			snprintf(path, sizeof path, "/proc/self/fd/%d", es.fd);
			ln = readlink(path, link, sizeof link - 1);
			link[ln > 0 ? ln : 0] = 0;
			printf(" -> %s poll(0)=0x%x", link, poll_ms(es.fd, 0));
			/* import exported sync_file into a semaphore (TEMPORARY) and wait it on the GPU */
			VkSemaphoreCreateInfo s2 = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
			VkSemaphore isem;
			CK(vkCreateSemaphore(d, &s2, NULL, &isem));
			VkImportSemaphoreFdInfoKHR ii = {.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR, .semaphore = isem,
			                                 .flags = VK_SEMAPHORE_IMPORT_TEMPORARY_BIT, .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT, .fd = es.fd};
			VkResult ir2 = pImportSemFd(d, &ii);
			printf(" vkImportSemaphoreFdKHR(SYNC_FD,TEMP,fd)=%d fcntl(fd)=%d", ir2, fcntl(es.fd, F_GETFD));
			VkPipelineStageFlags ws = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			VkSubmitInfo ws_si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .waitSemaphoreCount = 1, .pWaitSemaphores = &isem, .pWaitDstStageMask = &ws};
			CK(vkResetFences(d, 1, &fence));
			CK(vkQueueSubmit(q, 1, &ws_si, fence));
			printf(" waitFence=%d", vkWaitForFences(d, 1, &fence, VK_TRUE, 2000000000ull));
			vkDestroySemaphore(d, isem, NULL);
		}
		printf("\n");
	}
	/* import -1 */
	{
		VkSemaphoreCreateInfo s2 = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		VkSemaphore isem;
		CK(vkCreateSemaphore(d, &s2, NULL, &isem));
		VkImportSemaphoreFdInfoKHR ii = {.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR, .semaphore = isem,
		                                 .flags = VK_SEMAPHORE_IMPORT_TEMPORARY_BIT, .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT, .fd = -1};
		VkResult ir2 = pImportSemFd(d, &ii);
		VkPipelineStageFlags ws = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkSubmitInfo ws_si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .waitSemaphoreCount = 1, .pWaitSemaphores = &isem, .pWaitDstStageMask = &ws};
		CK(vkResetFences(d, 1, &fence));
		CK(vkQueueSubmit(q, 1, &ws_si, fence));
		printf("vkImportSemaphoreFdKHR(SYNC_FD,TEMP,-1)=%d then wait-submit fence=%d\n", ir2, vkWaitForFences(d, 1, &fence, VK_TRUE, 2000000000ull));
		vkDestroySemaphore(d, isem, NULL);
	}
	/* empty batch signalling an exportable semaphore, idle queue */
	{
		CK(vkQueueWaitIdle(q));
		VkSemaphore s3;
		CK(vkCreateSemaphore(d, &sci, NULL, &s3));
		VkSubmitInfo e = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .signalSemaphoreCount = 1, .pSignalSemaphores = &s3};
		CK(vkQueueSubmit(q, 1, &e, VK_NULL_HANDLE));
		VkSemaphoreGetFdInfoKHR g3 = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR, .semaphore = s3, .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};
		int f3 = -2;
		r = pGetSemFd(d, &g3, &f3);
		printf("empty batch (no wait) signal: vkGetSemaphoreFdKHR(SYNC_FD)=%d fd=%d%s\n", r, f3, f3 >= 0 ? "" : " (-1 = already signaled)");
		if (f3 >= 0) {
			printf("  poll(0)=0x%x\n", poll_ms(f3, 0));
			struct dma_buf_import_sync_file is = {.flags = DMA_BUF_SYNC_WRITE, .fd = f3};
			int ir = ioctl(fd, DMA_BUF_IOCTL_IMPORT_SYNC_FILE, &is);
			printf("  IMPORT_SYNC_FILE(WRITE, this fd) = %d errno=%d\n", ir, ir ? errno : 0);
			close(f3);
		}
		/* empty batch waiting on a just-signalled semaphore from a clear (in-flight case) */
		VkSemaphore s4, s5;
		CK(vkCreateSemaphore(d, &sci, NULL, &s4));
		CK(vkCreateSemaphore(d, &sci, NULL, &s5));
		VkCommandBuffer cb2;
		VkCommandBufferAllocateInfo cai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1};
		CK(vkAllocateCommandBuffers(d, &cai, &cb2));
		VkCommandBufferBeginInfo bi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
		CK(vkBeginCommandBuffer(cb2, &bi));
		barrier(cb2, img, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED);
		VkClearColorValue cc2 = {.float32 = {0.0f, 1.0f, 0.0f, 1.0f}};
		vkCmdClearColorImage(cb2, img, VK_IMAGE_LAYOUT_GENERAL, &cc2, 1, &rng);
		CK(vkEndCommandBuffer(cb2));
		VkSubmitInfo c = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cb2, .signalSemaphoreCount = 1, .pSignalSemaphores = &s4};
		CK(vkQueueSubmit(q, 1, &c, VK_NULL_HANDLE));
		VkPipelineStageFlags ws = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		VkSubmitInfo e2 = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .waitSemaphoreCount = 1, .pWaitSemaphores = &s4, .pWaitDstStageMask = &ws,
		                   .signalSemaphoreCount = 1, .pSignalSemaphores = &s5};
		CK(vkQueueSubmit(q, 1, &e2, VK_NULL_HANDLE));
		VkSemaphoreGetFdInfoKHR g5 = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR, .semaphore = s5, .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};
		int f5 = -2;
		r = pGetSemFd(d, &g5, &f5);
		printf("present-like: clear(signal s4) + empty batch(wait s4, signal s5): vkGetSemaphoreFdKHR(s5)=%d fd=%d", r, f5);
		if (f5 >= 0) {
			printf(" poll(0)=0x%x", poll_ms(f5, 0));
			struct dma_buf_import_sync_file is = {.flags = DMA_BUF_SYNC_WRITE, .fd = f5};
			int ir = ioctl(fd, DMA_BUF_IOCTL_IMPORT_SYNC_FILE, &is);
			printf(" IMPORT_SYNC_FILE(WRITE)=%d errno=%d", ir, ir ? errno : 0);
			struct dma_buf_export_sync_file es = {.flags = DMA_BUF_SYNC_READ, .fd = -1};
			int er = ioctl(fd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &es);
			struct timespec t0, t1;
			clock_gettime(CLOCK_MONOTONIC, &t0);
			int pr = es.fd >= 0 ? poll_ms(es.fd, 2000) : -1;
			clock_gettime(CLOCK_MONOTONIC, &t1);
			printf(" EXPORT_SYNC_FILE(READ)=%d fd=%d poll(2000)=0x%x in %ld us", er, es.fd, pr, (t1.tv_sec - t0.tv_sec) * 1000000 + (t1.tv_nsec - t0.tv_nsec) / 1000);
			if (es.fd >= 0)
				close(es.fd);
			close(f5);
		}
		printf("\n");
		CK(vkQueueWaitIdle(q));
		printf("mmap pixel(0,0) after green clear=0x%08x\n", map[lay.offset / 4]);
	}
	/* fence export SYNC_FD: does it reset the fence? */
	{
		VkExportFenceCreateInfo efc = {.sType = VK_STRUCTURE_TYPE_EXPORT_FENCE_CREATE_INFO, .handleTypes = VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT};
		VkFenceCreateInfo f2 = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = &efc};
		VkFence ef;
		CK(vkCreateFence(d, &f2, NULL, &ef));
		VkSubmitInfo e = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
		CK(vkQueueSubmit(q, 1, &e, ef));
		CK(vkWaitForFences(d, 1, &ef, VK_TRUE, UINT64_MAX));
		printf("fence SYNC_FD: status before export=%d", vkGetFenceStatus(d, ef));
		VkFenceGetFdInfoKHR gf = {.sType = VK_STRUCTURE_TYPE_FENCE_GET_FD_INFO_KHR, .fence = ef, .handleType = VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT};
		int ffd = -2;
		r = pGetFenceFd(d, &gf, &ffd);
		printf(" vkGetFenceFdKHR=%d fd=%d status after export=%d (0=SIGNALED, 1=NOT_READY)\n", r, ffd, vkGetFenceStatus(d, ef));
		if (ffd >= 0)
			close(ffd);
		vkDestroyFence(d, ef, NULL);
		/* OPAQUE_FD fence creation */
		efc.handleTypes = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT;
		r = vkCreateFence(d, &f2, NULL, &ef);
		printf("fence OPAQUE_FD create=%d", r);
		if (r == VK_SUCCESS) {
			gf.fence = ef;
			gf.handleType = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT;
			ffd = -2;
			r = pGetFenceFd(d, &gf, &ffd);
			printf(" vkGetFenceFdKHR(OPAQUE_FD)=%d fd=%d", r, ffd);
			vkDestroyFence(d, ef, NULL);
		}
		printf("\n");
	}

	/* ---- 2. import: second VkDevice in this process, then second process ---- */
	{
		VkQueue q2;
		VkDevice d2 = make_device(&q2);
		import_and_read(d2, q2, fd, imp.drmFormatModifier, lay.offset, lay.rowPitch, "device2");
		vkDestroyDevice(d2, NULL);

	}
	{
		int cfd = fcntl(fd, F_DUPFD, 100); /* no CLOEXEC */
		char a[4][32];
		snprintf(a[0], 32, "%d", cfd);
		snprintf(a[1], 32, "0x%" PRIx64, imp.drmFormatModifier);
		snprintf(a[2], 32, "%" PRIu64, (uint64_t)lay.offset);
		snprintf(a[3], 32, "%" PRIu64, (uint64_t)lay.rowPitch);
		pid_t pid = fork();
		if (pid == 0) {
			execl("/proc/self/exe", argv[0], "import", a[0], a[1], a[2], a[3], (char *)NULL);
			_exit(127);
		}
		int st;
		waitpid(pid, &st, 0);
		printf("process2 exit=%d\n", WEXITSTATUS(st));
	}
	return 0;
}
