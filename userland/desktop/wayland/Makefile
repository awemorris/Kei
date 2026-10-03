WAYLAND_MAKEFILE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(WAYLAND_MAKEFILE_DIR)../package.mk
# The compositor's operating system (WS131): libkeiland-backend's zedBSD side.
include $(WAYLAND_MAKEFILE_DIR)../libkeiland-backend-zedbsd/sources.mk

# Wayland compositor: every window composed with Vulkan into a VK_KHR_display swapchain.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# The operating system's side of the client GPU buffers (zwl-gpu.h, WS103): zedBSD's kernel image capabilities.
KEILAND_ZEDBSD_SOURCES := userland/desktop/wayland/zedbsd/gpu-zedbsd.c \
	userland/desktop/wayland/zedbsd/gpu-buffer-zedbsd.c \
	userland/desktop/wayland/zedbsd/input-zedbsd.c \
	userland/desktop/wayland/zedbsd/os-zedbsd.c
KEILAND_SOURCES := userland/desktop/wayland/main.c userland/desktop/wayland/wire.c \
	userland/desktop/wayland/objects.c userland/desktop/wayland/protocol.c userland/desktop/wayland/display.c \
	$(KEILAND_ZEDBSD_SOURCES) $(KL_BACKEND_ZEDBSD_SOURCES) \
	userland/desktop/wayland/seat.c userland/desktop/wayland/input.c userland/desktop/wayland/tablet.c userland/desktop/wayland/touch.c \
	userland/desktop/wayland/compose.c userland/desktop/wayland/import.c userland/desktop/wayland/shm.c \
	userland/desktop/wayland/glass.c userland/desktop/wayland/shell.c userland/desktop/wayland/home.c userland/desktop/wayland/corner.c userland/desktop/wayland/keyboard.c userland/desktop/wayland/keyboard-layout.c userland/desktop/wayland/keyboard-hand.c \
	userland/desktop/wayland/menu.c userland/desktop/wayland/menu-shell.c userland/desktop/wayland/titlebar.c \
	userland/desktop/wayland/popup.c userland/desktop/wayland/toplevel.c userland/desktop/wayland/sheet.c userland/desktop/wayland/subsurface.c userland/desktop/wayland/keymap.c userland/desktop/wayland/data.c \
	userland/desktop/wayland/icons.c userland/desktop/wayland/titlebar-shell.c \
	userland/desktop/wayland/decoration.c userland/desktop/wayland/cursor.c userland/desktop/wayland/viewport.c \
	userland/desktop/wayland/panels.c userland/desktop/wayland/backdrop.c userland/desktop/wayland/damage.c userland/desktop/wayland/greeter.c userland/desktop/wayland/handoff.c userland/desktop/wayland/backend-host.c userland/desktop/wayland/primary.c \
	userland/desktop/wayland/network.c userland/desktop/wayland/preferences.c userland/desktop/wayland/volume.c \
	userland/desktop/wayland/text-input.c userland/desktop/wayland/input-method.c \
	userland/desktop/wayland/desktop.c userland/desktop/wayland/inset.c userland/desktop/wayland/edit.c userland/desktop/wayland/clipboard.c \
	userland/tests/vkdemo/display.c userland/desktop/artwork/mark.c userland/desktop/picture/color-glyph.c
# App Home starts X11 applications through zdesktop-x11, which starts xserver when it is needed.
ZEDBSD_PACKAGE_INPUTS += userland/desktop/wayland/keiland-x11
ZEDBSD_PACKAGE_FILES += $(if $(filter wayland,$(ZEDBSD_USER_PROGRAMS)),--file /usr/libexec/keiland-x11=userland/desktop/wayland/keiland-x11)
# App Home's default list (ws129-p010), unless the image brings its own (a test's or the demonstration's extra files,
# which would otherwise name the same destination twice).
ZEDBSD_PACKAGE_INPUTS += userland/desktop/wayland/apps.conf
ZEDBSD_PACKAGE_FILES += $(if $(filter wayland,$(ZEDBSD_USER_PROGRAMS)),$(if $(findstring /etc/keiland/apps.conf=,$(ZEDBSD_TEST_EXTRA_FILES)),,\
	--file /etc/keiland/apps.conf=userland/desktop/wayland/apps.conf --mode /etc/keiland/apps.conf=0644))
KEILAND_FONT_DATA := /usr/share/fonts/keiland.ttf=userland/desktop/fonts/Inter.ttf \
	/usr/share/fonts/keiland-mono.ttf=userland/desktop/fonts/JetBrainsMono-Regular.ttf \
	/usr/share/fonts/keiland-fallback.ttf=userland/desktop/fonts/DroidSansFallbackFull.ttf \
	/usr/share/licenses/keiland-fonts/Inter-OFL.txt=userland/desktop/fonts/Inter-OFL.txt \
	/usr/share/licenses/keiland-fonts/JetBrainsMono-OFL.txt=userland/desktop/fonts/JetBrainsMono-OFL.txt \
	/usr/share/licenses/keiland-fonts/DroidSansFallback-LICENSE.txt=userland/desktop/fonts/DroidSansFallback-LICENSE.txt \
	/usr/share/licenses/keiland-fonts/Apache-2.0.txt=userland/desktop/fonts/Apache-2.0.txt
$(eval $(call ZEDBSD_USERLAND_PACKAGE,wayland,Wayland compositor,amd64,y,basic,$(KEILAND_SOURCES),y,desktop,desktop/wayland,desktop/libvulkan desktop/libtruetype desktop/libkeiland desktop/sessiond base/libpng-compat base/libz-compat fonts/noto-color-emoji,0755,program,$(KEILAND_FONT_DATA),,bin))
