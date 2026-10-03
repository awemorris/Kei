# zedBSD userland package: wayland-protocols (external release tarball).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# wayland-protocols is MIT-licensed protocol descriptions (XML) and stays
# outside the source tree: only this Makefile and the recorded archive
# identity live here.  It is build-time data: the GTK port's build (WS115)
# finds the XML through wayland-protocols.pc and generates its own protocol
# code from it, so nothing of it goes into the image.  Provenance is in
# plan/ws034/package-inventory.md and plan/ws115/port-contract.md.
ZEDBSD_WAYLAND_PROTOCOLS_MAKEFILE := $(lastword $(MAKEFILE_LIST))
ZEDBSD_WAYLAND_PROTOCOLS_DIR := $(patsubst %/,%,$(dir $(ZEDBSD_WAYLAND_PROTOCOLS_MAKEFILE)))
include $(ZEDBSD_WAYLAND_PROTOCOLS_DIR)/../../../base/package.mk
include $(ZEDBSD_WAYLAND_PROTOCOLS_DIR)/../../external.mk

# 1.49 was the latest release when recorded.  Size and digest are the
# upstream bytes as measured; they match the published SHA-256 and the
# archive's signature was checked (WS034 p001).
ZEDBSD_EXT_wayland-protocols_VERSION := 1.49
ZEDBSD_EXT_wayland-protocols_ROOT := wayland-protocols-1.49
ZEDBSD_EXT_wayland-protocols_ARCHIVE := wayland-protocols-1.49.tar.xz
ZEDBSD_EXT_wayland-protocols_URL := https://gitlab.freedesktop.org/-/project/2891/uploads/7ed597f0cad076a17fe36f8860596f8c/wayland-protocols-1.49.tar.xz
ZEDBSD_EXT_wayland-protocols_SIZE := 150612
ZEDBSD_EXT_wayland-protocols_SHA256 := ec4c8f74942d6dff7ace8b4ce4764f0ef9ff618a935d974ea77edee2ad240b14
ZEDBSD_EXT_wayland-protocols_PATCH_LEVEL := zedbsd1
ZEDBSD_EXT_wayland-protocols_PATCHES := \
	$(ZEDBSD_WAYLAND_PROTOCOLS_DIR)/patches/0001-make-enum-headers-only-with-scanner-1-24.patch

$(eval $(call ZEDBSD_EXTERNAL_SOURCE,wayland-protocols))

# ---------------------------------------------------------------- build

# Meson, through the common rule; nothing is compiled.  The tests, which
# would compile every protocol's generated code, are off, and patch 0001
# leaves out the enum headers the build machine's wayland-scanner 1.23 cannot
# make from this release's XML (GTK does not use them).  The stage holds
# the XML under usr/share/wayland-protocols and the .pc file under
# usr/share/pkgconfig, which a package that depends on this one finds
# through its view.
ZEDBSD_EXT_wayland-protocols_MAKEFILE := $(ZEDBSD_WAYLAND_PROTOCOLS_MAKEFILE)
ZEDBSD_EXT_wayland-protocols_DEPENDS :=
ZEDBSD_EXT_wayland-protocols_HOST_TOOLS :=
ZEDBSD_EXT_wayland-protocols_MESON_OPTIONS := -Dtests=false

$(eval $(call ZEDBSD_EXTERNAL_MESON,wayland-protocols))

ZEDBSD_WAYLAND_PROTOCOLS_STAGED := $(ZEDBSD_EXT_wayland-protocols_STAGEDIR)/.zedbsd-staged
ZEDBSD_WAYLAND_PROTOCOLS_CHECKED := $(ZEDBSD_EXT_wayland-protocols_STAGEDIR)/.zedbsd-checked
ZEDBSD_WAYLAND_PROTOCOLS_PC := $(ZEDBSD_EXT_wayland-protocols_STAGEDIR)/usr/share/pkgconfig/wayland-protocols.pc
ZEDBSD_WAYLAND_PROTOCOLS_XDG_SHELL := \
	$(ZEDBSD_EXT_wayland-protocols_STAGEDIR)/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml

# Checks that the stage has the .pc file of the release and the XML GTK's
# build asks for first.
$(ZEDBSD_WAYLAND_PROTOCOLS_CHECKED): $(ZEDBSD_WAYLAND_PROTOCOLS_STAGED)
	@grep -qx 'Version: $(ZEDBSD_EXT_wayland-protocols_VERSION)' '$(ZEDBSD_WAYLAND_PROTOCOLS_PC)'
	@test -f '$(ZEDBSD_WAYLAND_PROTOCOLS_XDG_SHELL)'
	@touch '$@'

.PHONY: wayland-protocols
wayland-protocols: $(ZEDBSD_WAYLAND_PROTOCOLS_CHECKED)
	@:
