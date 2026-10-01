include $(dir $(lastword $(MAKEFILE_LIST)))../package.mk

# The widgets' sampler (WS090 ws090-p005): libkeiui's widgets on three pages for the pointer, the keyboard
# and the fingers, logging what they report.  In the test image only (plan/ws035/tests/config-amd64-zdesktop.mk).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
KUIDEMO_SOURCES := userland/tests/kuidemo/main.c
$(eval $(call ZEDBSD_USERLAND_PACKAGE,kuidemo,Widgets sampler,amd64,y,basic,$(KUIDEMO_SOURCES),y,tests,tests/kuidemo,desktop/libvulkan desktop/libwayland desktop/libkeiland desktop/libkeiui desktop/libtruetype,0755,program,,,bin))
