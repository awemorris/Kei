include $(dir $(lastword $(MAKEFILE_LIST)))../../base/package.mk

# GLX test (WS069 p004): an X window drawn with OpenGL through GLX (egltest's scene; --gl3: an OpenGL 3.0 context, WS068 p013; --gl31: 3.1, p031; --gl32: 3.2, p033).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
GLXTEST_SOURCES := userland/retro/glxtest/main.c userland/retro/glxtest/gl3.c userland/retro/glxtest/gl31.c userland/retro/glxtest/gl32.c userland/tests/egltest/scene.c userland/retro/libX11/xlib.c
$(eval $(call ZEDBSD_USERLAND_PACKAGE,glxtest,GLX test,amd64,y,basic,$(GLXTEST_SOURCES),y,x11-applications,X11/glxtest,X11/libGL,0755,program,,,bin))
