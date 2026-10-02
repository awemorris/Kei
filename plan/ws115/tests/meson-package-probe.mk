# ws115-p004: exercise the ZEDBSD_EXTERNAL_MESON rule with a real upstream
# Meson release, outside the image: graphene 1.10.8 without its GObject types
# (so it needs no other package).  Not a userland package and never
# installed.  Run from the repository root:
#
#   make -f plan/ws115/tests/meson-package-probe.mk \
#       ZEDBSD_STANDALONE_CONFIG=<config.mk> meson-package-probe
#
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
ZEDBSD_PROBE_MAKEFILE := $(lastword $(MAKEFILE_LIST))
include userland/base/package.mk
include userland/packages/external.mk

ZEDBSD_EXT_graphene_probe_VERSION := 1.10.8
ZEDBSD_EXT_graphene_probe_ROOT := graphene-1.10.8
ZEDBSD_EXT_graphene_probe_ARCHIVE := graphene-1.10.8.tar.xz
ZEDBSD_EXT_graphene_probe_URL := https://download.gnome.org/sources/graphene/1.10/graphene-1.10.8.tar.xz
ZEDBSD_EXT_graphene_probe_SIZE := 333924
ZEDBSD_EXT_graphene_probe_SHA256 := a37bb0e78a419dcbeaa9c7027bcff52f5ec2367c25ec859da31dfde2928f279a
ZEDBSD_EXT_graphene_probe_PATCH_LEVEL := zedbsd0
ZEDBSD_EXT_graphene_probe_PATCHES :=
$(eval $(call ZEDBSD_EXTERNAL_SOURCE,graphene_probe))

ZEDBSD_EXT_graphene_probe_MAKEFILE := $(ZEDBSD_PROBE_MAKEFILE)
ZEDBSD_EXT_graphene_probe_DEPENDS :=
ZEDBSD_EXT_graphene_probe_HOST_TOOLS :=
ZEDBSD_EXT_graphene_probe_MESON_OPTIONS := -Dgobject_types=false \
	-Dintrospection=disabled -Dtests=false -Dinstalled_tests=false -Dgtk_doc=false
$(eval $(call ZEDBSD_EXTERNAL_MESON,graphene_probe))

# The library graphene installs, checked as an installed shared library.
.PHONY: meson-package-probe
meson-package-probe: $(ZEDBSD_EXT_graphene_probe_STAGED)
	python3 tools/build/check-dynamic-elf.py --machine $(ZEDBSD_ARCHITECTURE) \
		--role shared-library --soname libgraphene-1.0.so.0 --needed libc.so \
		'$(ZEDBSD_EXT_graphene_probe_STAGEDIR)/usr/lib/libgraphene-1.0.so.0.1000.8'
	@echo 'meson-package-probe: PASS'
