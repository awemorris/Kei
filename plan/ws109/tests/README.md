# WS109 native contract probes

Run from the source root on FreeBSD15 with the selected native dependency packages.
`gmake -j16 -f userland/desktop/keiland-freebsd.mk libraries` builds the L1 foundation;
`gmake -f userland/desktop/keiland-freebsd.mk install DESTDIR=/tmp/keiland-stage` stages it.
Full compositor/apps need later backend integration; this target currently builds libraries only.

- public-libraries.c: compile with `-I/tmp/keiland-stage/opt/keiland/include -I/usr/local/include`,
  link staged libwayland-client.so/libtruetype.so/libvulkan.so.1. Use `-fPIE -pie` for native clients.
- ancillary.c: compile `-I. -Iuserland/desktop/keiland -pthread`, link staged libwayland-client.so.
  Uses public calls and private FIFO inspection to verify actual native kernel rights and ownership.
- sync-rejected.c: compile `-I.` with the selected `libvulkan-compat/freebsd/sync-freebsd.c`
  (Linux uses linux/sync-linux.c). Checks real rejected ioctls, not positive hardware fences.

All probes: `-std=gnu17 -Wall -Wextra -Werror`; run with LD_LIBRARY_PATH set to the staged lib directory.
The existing plan/tools/keiland-linux/vk-chain-test.c runs natively with system Vulkan headers,
staged own library and VK_DRIVER_FILES=/usr/local/share/vulkan/icd.d/lvp_icd.x86_64.json,
DISPLAY/WAYLAND_DISPLAY unset and KEILAND_DRM_DEVICE=none. It checks a real headless1MiB workload.
Never label that result physical GPU/display acceptance. Exact environment/results: q553 history.
