# Native FreeBSD Keiland build, independent of the Linux and zedBSD rules.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Builds real native libraries and the compositor with FreeBSD seat authority.
.DEFAULT_GOAL := all

KEILAND_FREEBSD_BUILD ?= build/keiland-freebsd
KEILAND_PREFIX ?= /opt/keiland
DESTDIR ?=
KEILAND_FREEBSD_OPT ?= -O2 -g
KEILAND_FREEBSD_LOCALBASE ?= /usr/local
KEILAND_FREEBSD_EXTRA_CPPFLAGS ?=
KEILAND_FREEBSD_CPPFLAGS := \
	-DKEILAND_BINDIR='"$(KEILAND_PREFIX)/bin"' -DKEILAND_LIBEXECDIR='"$(KEILAND_PREFIX)/libexec"' \
	-DKEILAND_DATADIR='"$(KEILAND_PREFIX)/share"' -DKEILAND_SYSCONFDIR='"$(KEILAND_PREFIX)/etc"' \
	$(KEILAND_FREEBSD_EXTRA_CPPFLAGS) -I. -Iuserland/desktop/keiland \
	-I$(KEILAND_FREEBSD_BUILD)/include -I$(KEILAND_FREEBSD_LOCALBASE)/include
KEILAND_FREEBSD_CFLAGS := $(KEILAND_FREEBSD_OPT) -std=gnu17 -Wall -Wextra -Werror -fPIC
KEILAND_FREEBSD_LDFLAGS := -Wl,-rpath,$(KEILAND_PREFIX)/lib -Wl,--enable-new-dtags \
	-Wl,-rpath-link,$(KEILAND_FREEBSD_BUILD)/lib -L$(KEILAND_FREEBSD_BUILD)/lib

# Copy only the selected compatibility headers; native libc headers keep their standard names.
KEILAND_FREEBSD_HEADERS := $(shell find include/libc/compat -type f -name '*.h') \
	include/libc/pdf.h include/libc/sha2.h include/libc/md5.h include/libc/sha1.h
KEILAND_FREEBSD_HEADER_COPIES := $(patsubst include/libc/%,$(KEILAND_FREEBSD_BUILD)/include/%,$(KEILAND_FREEBSD_HEADERS))
KEILAND_FREEBSD_NATIVE_HEADERS := $(KEILAND_FREEBSD_BUILD)/include/pty.h $(KEILAND_FREEBSD_BUILD)/include/sys/xattr.h
$(KEILAND_FREEBSD_BUILD)/include/%: include/libc/%
	@mkdir -p $(dir $@)
	cp $< $@

# Use the installed native DRM UAPI under the common source's include spelling.
$(KEILAND_FREEBSD_BUILD)/include/drm:
	@mkdir -p $(dir $@)
	test -f $(KEILAND_FREEBSD_LOCALBASE)/include/libdrm/drm.h
	ln -s $(KEILAND_FREEBSD_LOCALBASE)/include/libdrm $@

$(KEILAND_FREEBSD_BUILD)/obj/%.o: %.c | $(KEILAND_FREEBSD_HEADER_COPIES) $(KEILAND_FREEBSD_NATIVE_HEADERS) $(KEILAND_FREEBSD_BUILD)/include/drm
	@mkdir -p $(dir $@)
	$(CC) $(KEILAND_FREEBSD_CFLAGS) $(KEILAND_FREEBSD_CPPFLAGS) $(KEILAND_FREEBSD_CPPFLAGS_$(subst /,_,$(dir $<))) -MMD -MP -c $< -o $@

KEILAND_FREEBSD_SOURCES :=
KEILAND_FREEBSD_ALL :=
KEILAND_FREEBSD_INSTALL :=

# $(1) name, $(2) SONAME, $(3) sources, $(4) our libraries it links (SONAMEs or a .a), $(5) system libraries,
# $(6) the version script or empty, $(7) more link flags
define KEILAND_FREEBSD_LIBRARY
KEILAND_FREEBSD_SOURCES += $(3)
KEILAND_FREEBSD_LIBRARY_OBJS_$(1) := $$(patsubst %.c,$(KEILAND_FREEBSD_BUILD)/obj/%.o,$(3))
-include $$(KEILAND_FREEBSD_LIBRARY_OBJS_$(1):.o=.d)
$(KEILAND_FREEBSD_BUILD)/lib/$(2): $$(KEILAND_FREEBSD_LIBRARY_OBJS_$(1)) $$(addprefix $(KEILAND_FREEBSD_BUILD)/lib/,$(4)) $(6)
	@mkdir -p $$(dir $$@)
	$$(CC) -shared -Wl,-soname,$(2) -Wl,-z,defs $$(if $(6),-Wl$$(comma)--version-script=$(6)) $(7) $$(KEILAND_FREEBSD_LDFLAGS) \
		$$(KEILAND_FREEBSD_LIBRARY_OBJS_$(1)) $$(addprefix -l:,$(4)) $(5) -o $$@
KEILAND_FREEBSD_ALL += $(KEILAND_FREEBSD_BUILD)/lib/$(2)
KEILAND_FREEBSD_INSTALL += lib/$(2)
endef

# $(1) name, $(2) bin or libexec, $(3) sources, $(4) own libraries, $(5) system libraries
define KEILAND_FREEBSD_PROGRAM
KEILAND_FREEBSD_SOURCES += $(3)
KEILAND_FREEBSD_PROGRAM_OBJS_$(1) := $$(patsubst %.c,$(KEILAND_FREEBSD_BUILD)/obj/%.o,$(3))
-include $$(KEILAND_FREEBSD_PROGRAM_OBJS_$(1):.o=.d)
$(KEILAND_FREEBSD_BUILD)/$(2)/$(1): $$(KEILAND_FREEBSD_PROGRAM_OBJS_$(1)) $$(addprefix $(KEILAND_FREEBSD_BUILD)/lib/,$(4))
	@mkdir -p $$(dir $$@)
	$$(CC) -pie $$(KEILAND_FREEBSD_LDFLAGS) $$(KEILAND_FREEBSD_PROGRAM_OBJS_$(1)) $$(addprefix -l:,$(4)) $(5) -o $$@
KEILAND_FREEBSD_ALL += $(KEILAND_FREEBSD_BUILD)/$(2)/$(1)
KEILAND_FREEBSD_INSTALL += $(2)/$(1)
endef

# $(1) path under the prefix, $(2) source file
define KEILAND_FREEBSD_DATA
$(KEILAND_FREEBSD_BUILD)/$(1): $(2)
	@mkdir -p $$(dir $$@)
	cp $$< $$@
KEILAND_FREEBSD_ALL += $(KEILAND_FREEBSD_BUILD)/$(1)
KEILAND_FREEBSD_INSTALL += $(1)
endef

# $(1) name, $(2) the archive's file name, $(3) sources
define KEILAND_FREEBSD_STATIC
KEILAND_FREEBSD_SOURCES += $(3)
KEILAND_FREEBSD_STATIC_OBJS_$(1) := $$(patsubst %.c,$(KEILAND_FREEBSD_BUILD)/obj/%.o,$(3))
-include $$(KEILAND_FREEBSD_STATIC_OBJS_$(1):.o=.d)
$(KEILAND_FREEBSD_BUILD)/lib/$(2): $$(KEILAND_FREEBSD_STATIC_OBJS_$(1))
	@mkdir -p $$(dir $$@)
	rm -f $$@
	ar rcs $$@ $$(KEILAND_FREEBSD_STATIC_OBJS_$(1))
KEILAND_FREEBSD_ALL += $(KEILAND_FREEBSD_BUILD)/lib/$(2)
endef

comma := ,

KEILAND_FREEBSD_PACKAGES ?= userland/base/libz-compat/Makefile.freebsd \
	userland/base/libpng-compat/Makefile.freebsd \
	userland/base/libjpeg-compat/Makefile.freebsd \
	userland/base/libgif-compat/Makefile.freebsd \
	userland/desktop/freebsd-compat/Makefile.freebsd \
	userland/desktop/libwayland/Makefile.freebsd \
	userland/desktop/libtruetype/Makefile.freebsd \
	userland/desktop/libvulkan-compat/Makefile.freebsd \
	userland/desktop/libkeiland/Makefile.freebsd \
	userland/desktop/libkeiui/Makefile.freebsd \
	userland/base/libpdf/Makefile.freebsd \
	userland/packages/libseat/Makefile.freebsd \
	userland/desktop/wayland/Makefile.freebsd
include $(KEILAND_FREEBSD_PACKAGES)

.PHONY: all libraries install install-headers print-sources header-dependencies
all: $(KEILAND_FREEBSD_ALL)
libraries: $(filter %.a %.so %.so.1,$(KEILAND_FREEBSD_ALL))

# FreeBSD install has no GNU -D; create each destination directory explicitly.
install: all install-headers
	@set -e; for f in $(KEILAND_FREEBSD_INSTALL); do \
		mode=0644; case $$f in lib/*|bin/*|libexec/*) mode=0755 ;; esac; \
		mkdir -p "$(DESTDIR)$(KEILAND_PREFIX)/$$(dirname "$$f")"; \
		install -m $$mode "$(KEILAND_FREEBSD_BUILD)/$$f" "$(DESTDIR)$(KEILAND_PREFIX)/$$f"; \
	done

KEILAND_FREEBSD_PUBLIC_HEADERS := $(shell find userland/desktop/keiland/wayland -type f -name '*.h') \
	$(addprefix userland/desktop/keiland/,wayland-client.h wayland-client-core.h wayland-client-protocol.h \
	wayland-util.h xdg-shell-client-protocol.h primary-selection-unstable-v1-client-protocol.h \
	tablet-unstable-v2-client-protocol.h truetype.h keiland.h keiui.h)
install-headers:
	@mkdir -p "$(DESTDIR)$(KEILAND_PREFIX)/include"
	install -m 0644 include/libc/pdf.h "$(DESTDIR)$(KEILAND_PREFIX)/include/pdf.h"
	@set -e; for f in $(KEILAND_FREEBSD_PUBLIC_HEADERS); do \
		rel=$${f#userland/desktop/keiland/}; \
		mkdir -p "$(DESTDIR)$(KEILAND_PREFIX)/include/$$(dirname "$$rel")"; \
		install -m 0644 "$$f" "$(DESTDIR)$(KEILAND_PREFIX)/include/$$rel"; \
	done

print-sources:
	@printf '%s\n' $(KEILAND_FREEBSD_SOURCES)

# Include native system headers in audit output, unlike ordinary -MMD dependency files.
KEILAND_FREEBSD_HEADER_DEPS := $(patsubst %.c,$(KEILAND_FREEBSD_BUILD)/header-check/%.d,$(KEILAND_FREEBSD_SOURCES))
header-dependencies: $(KEILAND_FREEBSD_HEADER_DEPS)
$(KEILAND_FREEBSD_BUILD)/header-check/%.d: %.c | $(KEILAND_FREEBSD_HEADER_COPIES) $(KEILAND_FREEBSD_NATIVE_HEADERS) $(KEILAND_FREEBSD_BUILD)/include/drm
	@mkdir -p $(dir $@)
	$(CC) $(KEILAND_FREEBSD_CFLAGS) $(KEILAND_FREEBSD_CPPFLAGS) $(KEILAND_FREEBSD_CPPFLAGS_$(subst /,_,$(dir $<))) -M $< -o $@
