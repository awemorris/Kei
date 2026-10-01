/* compat.c -- thin libvulkan.so.1 forwarder (WS105 design check) */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

#define BACKEND "/usr/lib/x86_64-linux-gnu/libvulkan.so.1"
static void *backend;

__attribute__((unused)) static void missing(const char *n)
{
	fprintf(stderr, "libvulkan-compat: the backend has no %s\n", n);
	abort();
}
#ifndef MINIMAL
#include "forward.inc"
#else
static PFN_vkGetInstanceProcAddr next_vkGetInstanceProcAddr;
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *pName)
{
	if (getenv("COMPAT_LOG"))
		fprintf(stderr, "GIPA %s %s\n", instance ? "inst" : "null", pName);
	return next_vkGetInstanceProcAddr(instance, pName);
}
static void fill_table(void *h) { next_vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(h, "vkGetInstanceProcAddr"); }
#endif

__attribute__((constructor)) static void open_backend(void)
{
	int flags = RTLD_NOW | RTLD_LOCAL;
	if (getenv("COMPAT_DEEPBIND"))
		flags |= RTLD_DEEPBIND;
	backend = dlopen(BACKEND, flags);
	Dl_info self;
	dladdr((void *)open_backend, &self);
	void *selfh = dlopen(self.dli_fname, RTLD_NOW | RTLD_NOLOAD);
	void *bgipa = backend ? dlsym(backend, "vkGetInstanceProcAddr") : NULL;
	if (getenv("COMPAT_VERBOSE"))
		fprintf(stderr, "compat: self=%s selfhandle=%p backend=%p (%s) backend.vkGetInstanceProcAddr=%p own=%p same=%d\n", self.dli_fname, selfh,
		        backend, backend == selfh ? "SAME" : "different", bgipa, (void *)vkGetInstanceProcAddr, bgipa == (void *)vkGetInstanceProcAddr);
	if (!backend || bgipa == (void *)vkGetInstanceProcAddr) {
		fprintf(stderr, "libvulkan-compat: no backend libvulkan\n");
		return;
	}
	fill_table(backend);
}
