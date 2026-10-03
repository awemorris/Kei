include $(dir $(lastword $(MAKEFILE_LIST)))../package.mk

# The desktop's shared widgets and controls (WS090, plan/ws090/design.md): so far the drawing layer,
# a CPU canvas, TrueType text, icons and the Kei theme (KUI_VERSION 1), and the scroll, the input and the
# text view's touch (KUI_VERSION 2), on libkeiland's scroller and gestures, and the window (KUI_VERSION 3):
# Wayland, Vulkan or shared memory, the clipboard and the primary selection, and the widgets (KUI_VERSION 4):
# buttons, switches, sliders, fields, lists, the sidebar, panels, cards, dialogs, chips and progress, and the
# file chooser moved from libkeiland (KUI_VERSION 5).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
LIBKEIUI_SOURCES := userland/desktop/libkeiui/version.c userland/desktop/libkeiui/canvas.c \
	userland/desktop/libkeiui/text.c userland/desktop/libkeiui/icons.c \
	userland/desktop/libkeiui/icons-line.c userland/desktop/libkeiui/theme.c \
	userland/desktop/libkeiui/input.c userland/desktop/libkeiui/scroll.c \
	userland/desktop/libkeiui/scroll-bar.c \
	userland/desktop/libkeiui/text-touch.c userland/desktop/libkeiui/ui.c \
	userland/desktop/libkeiui/window.c userland/desktop/libkeiui/present.c \
	userland/desktop/libkeiui/present-shm.c userland/desktop/libkeiui/clipboard.c \
	userland/desktop/libkeiui/primary.c userland/desktop/libkeiui/text-input.c userland/desktop/libkeiui/edit.c userland/desktop/libkeiui/widgets.c \
	userland/desktop/libkeiui/field.c userland/desktop/libkeiui/list.c \
	userland/desktop/libkeiui/cards.c userland/desktop/libkeiui/chooser.c \
	userland/desktop/libkeiui/chooser-model.c userland/desktop/libkeiui/chooser-view.c \
	userland/desktop/picture/color-glyph.c

LIBKEIUI_IMAGE_DATA := $(if $(ZEDBSD_TOPLEVEL_BUILD),/lib/libkeiui.so=$(BUILD)/dynamic/libkeiui.so)
$(eval $(call ZEDBSD_USERLAND_PACKAGE,libkeiui,Desktop widgets,amd64,y,library,$(LIBKEIUI_SOURCES),y,desktop,desktop/libkeiui,desktop/libtruetype desktop/libkeiland desktop/libwayland desktop/libvulkan base/libpng-compat base/libz-compat,,shared-library,$(LIBKEIUI_IMAGE_DATA),,lib))
