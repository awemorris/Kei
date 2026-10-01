#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <vulkan/vulkan.h>
int main(void)
{
	Dl_info di;
	dladdr((void *)vkCreateInstance, &di);
	printf("vkCreateInstance resolved in %s\n", di.dli_fname);
	VkApplicationInfo ai = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_3};
	VkInstanceCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &ai};
	VkInstance inst;
	VkResult r = vkCreateInstance(&ici, NULL, &inst);
	printf("vkCreateInstance=%d\n", r);
	if (r)
		return 1;
	uint32_t n = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &n, NULL);
	printf("instance extensions=%u\n", n);
	VkPhysicalDevice pds[8];
	n = 8;
	vkEnumeratePhysicalDevices(inst, &n, pds);
	for (uint32_t i = 0; i < n; i++) {
		VkPhysicalDeviceProperties p;
		vkGetPhysicalDeviceProperties(pds[i], &p);
		printf("GPU%u: %s\n", i, p.deviceName);
	}
	float pr = 1;
	VkDeviceQueueCreateInfo qci = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueCount = 1, .pQueuePriorities = &pr};
	VkDeviceCreateInfo dci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci};
	VkDevice d;
	r = vkCreateDevice(pds[0], &dci, NULL, &d);
	VkQueue q;
	vkGetDeviceQueue(d, 0, 0, &q);
	VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	VkFence f;
	vkCreateFence(d, &fci, NULL, &f);
	VkSubmitInfo si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
	vkQueueSubmit(q, 1, &si, f);
	printf("vkCreateDevice=%d waitFence=%d\n", r, vkWaitForFences(d, 1, &f, VK_TRUE, UINT64_MAX));
	vkDestroyFence(d, f, NULL);
	vkDestroyDevice(d, NULL);
	vkDestroyInstance(inst, NULL);
	return 0;
}
