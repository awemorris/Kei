include $(dir $(lastword $(MAKEFILE_LIST)))../package.mk

# The file manager of the desktop: a Wayland window drawn on the CPU and shown with Vulkan; text from libtruetype.
# The overlay scroll bar is libkeiui's scroll-bar.c, built into the program (it needs no canvas; ws127-p002).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
KEILAND_FILES_SOURCES := userland/desktop/files/main.c \
	userland/desktop/files/window.c userland/desktop/files/dnd.c userland/desktop/files/present.c \
	userland/desktop/files/canvas.c userland/desktop/files/text.c \
	userland/desktop/files/icons.c userland/desktop/files/ui.c \
	userland/desktop/files/ui-tabs.c \
	userland/desktop/files/ui-grid.c userland/desktop/files/dir.c \
	userland/desktop/files/mime.c userland/desktop/files/places.c userland/desktop/files/mntent/mounts-mntent.c \
	userland/desktop/files/ui-input.c userland/desktop/files/ui-list.c \
	userland/desktop/files/ui-scrollbar.c userland/desktop/libkeiui/scroll-bar.c \
	userland/desktop/files/ui-field.c userland/desktop/files/select.c \
	userland/desktop/files/task.c userland/desktop/files/trash.c \
	userland/desktop/files/undo.c userland/desktop/files/clip.c \
	userland/desktop/files/actions.c userland/desktop/files/ui-overlay.c \
	userland/desktop/files/tags.c userland/desktop/files/search.c \
	userland/desktop/files/ui-search.c userland/desktop/files/ui-home.c \
	userland/desktop/files/thumb.c userland/desktop/files/thumb-cache.c userland/desktop/files/peek.c \
	userland/desktop/files/ui-preview.c userland/desktop/files/apps.c \
	userland/desktop/files/info.c userland/desktop/files/ui-info.c \
	userland/desktop/files/ui-menu.c userland/desktop/files/ui-help.c \
	userland/desktop/files/menu.c userland/desktop/files/titlebar.c \
	userland/desktop/files/ui-titlebar.c userland/desktop/files/glass.c \
	userland/desktop/files/ui-context.c userland/desktop/files/ui-drag.c \
	userland/desktop/files/ui-desktop.c userland/desktop/files/ui-desktop-actions.c \
	userland/desktop/files/ui-desktop-drag.c \
	userland/desktop/files/desktop-layout.c \
	userland/desktop/files/touch.c userland/desktop/artwork/mark.c \
	userland/desktop/picture/picture.c
$(eval $(call ZEDBSD_USERLAND_PACKAGE,files,File manager,amd64,y,basic,$(KEILAND_FILES_SOURCES),y,desktop,desktop/files,desktop/libvulkan desktop/libwayland desktop/libkeiland desktop/libtruetype base/libz-compat base/libpng-compat base/libjpeg-compat base/libgif-compat,0755,program,,,bin))
