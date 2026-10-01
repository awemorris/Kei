#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <vulkan/vulkan.h>
static const char *where(void *p) { Dl_info d; return p && dladdr(p, &d) ? d.dli_fname : "(null)"; }
int main(void)
{
	void *b = dlopen("/usr/lib/x86_64-linux-gnu/libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
	void *byname = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
	printf("dlopen(abs backend)=%p dlopen(\"libvulkan.so.1\")=%p -> %s\n", b, byname, where(dlsym(byname, "vkGetInstanceProcAddr")));
	PFN_vkGetInstanceProcAddr bg = (PFN_vkGetInstanceProcAddr)dlsym(b, "vkGetInstanceProcAddr");
	printf("backend GIPA(NULL,vkCreateInstance) -> %s\n", where((void *)bg(NULL, "vkCreateInstance")));
	VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	VkInstance inst;
	((PFN_vkCreateInstance)dlsym(b, "vkCreateInstance"))(&ici, NULL, &inst);
	const char *names[] = {"vkCreateDevice", "vkGetPhysicalDeviceProperties", "vkCmdDraw", "vkQueueSubmit", "vkGetDeviceProcAddr", "vkDestroyDevice", "vkGetInstanceProcAddr"};
	for (unsigned i = 0; i < 7; i++)
		printf("backend GIPA(inst,%s) -> %s\n", names[i], where((void *)bg(inst, names[i])));
	VkPhysicalDevice pd; uint32_t n = 1;
	((PFN_vkEnumeratePhysicalDevices)bg(inst, "vkEnumeratePhysicalDevices"))(inst, &n, &pd);
	float pr = 1;
	VkDeviceQueueCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueCount = 1, .pQueuePriorities = &pr};
	VkDeviceCreateInfo dci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci};
	VkDevice d;
	((PFN_vkCreateDevice)bg(inst, "vkCreateDevice"))(pd, &dci, NULL, &d);
	PFN_vkGetDeviceProcAddr gd = (PFN_vkGetDeviceProcAddr)dlsym(b, "vkGetDeviceProcAddr");
	const char *dn[] = {"vkCmdDraw", "vkQueueSubmit", "vkGetDeviceProcAddr", "vkDestroyDevice", "vkGetDeviceQueue", "vkCreateImage"};
	for (unsigned i = 0; i < 6; i++)
		printf("backend GDPA(dev,%s) -> %s\n", dn[i], where((void *)gd(d, dn[i])));
	return 0;
}
