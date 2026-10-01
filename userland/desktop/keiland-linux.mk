# The Linux build of the Keiland desktop (WS105, plan/ws105/design.md §3).  It shares no rule with the zedBSD build:
# the host's compiler, headers and C library build every package listed in KEILAND_LINUX_PACKAGES, each with its
# own Makefile.linux, into $(KEILAND_LINUX_BUILD), and install copies the result under $(DESTDIR)$(KEILAND_PREFIX).
#
#   make keiland-linux                       (from the top-level Makefile: make -f userland/desktop/keiland-linux.mk all)
#   make keiland-linux-install DESTDIR=...   (install)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib

KEILAND_LINUX_BUILD ?= build/keiland-linux
KEILAND_PREFIX ?= /opt/keiland
DESTDIR ?=
KEILAND_LINUX_OPT ?= -O2 -g
# The Debian multiarch name of the host (clang has no -print-multiarch on Debian 19).
KEILAND_LINUX_MULTIARCH ?= $(shell gcc -print-multiarch 2>/dev/null || dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null)
KEILAND_LINUX_EXTRA_CPPFLAGS ?=

KEILAND_LINUX_CPPFLAGS := -D_GNU_SOURCE \
	-DKEILAND_BINDIR='"$(KEILAND_PREFIX)/bin"' -DKEILAND_LIBEXECDIR='"$(KEILAND_PREFIX)/libexec"' \
	-DKEILAND_DATADIR='"$(KEILAND_PREFIX)/share"' -DKEILAND_SYSCONFDIR='"$(KEILAND_PREFIX)/etc"' \
	$(KEILAND_LINUX_EXTRA_CPPFLAGS) -I. -Iuserland/desktop/keiland -I$(KEILAND_LINUX_BUILD)/include
# -Wno-format-truncation: gcc's guess that a display string may be cut short (the strings are cut on purpose; D24).
KEILAND_LINUX_CFLAGS := $(KEILAND_LINUX_OPT) -std=gnu17 -Wall -Wextra -Werror -Wno-format-truncation -fPIC
KEILAND_LINUX_LDFLAGS := -Wl,-rpath,$(KEILAND_PREFIX)/lib -Wl,--enable-new-dtags \
	-Wl,-rpath-link,$(KEILAND_LINUX_BUILD)/lib -L$(KEILAND_LINUX_BUILD)/lib

# The headers the Linux build takes from zedBSD's C library side: copies, never -Iinclude/libc (it would hide glibc's).
KEILAND_LINUX_HEADERS := $(shell find include/libc/compat -type f -name '*.h') \
	include/libc/pdf.h include/libc/sha2.h include/libc/md5.h include/libc/sha1.h
KEILAND_LINUX_HEADER_COPIES := $(patsubst include/libc/%,$(KEILAND_LINUX_BUILD)/include/%,$(KEILAND_LINUX_HEADERS))

$(KEILAND_LINUX_BUILD)/include/%: include/libc/%
	@mkdir -p $(dir $@)
	cp $< $@

$(KEILAND_LINUX_BUILD)/obj/%.o: %.c | $(KEILAND_LINUX_HEADER_COPIES)
	@mkdir -p $(dir $@)
	$(CC) $(KEILAND_LINUX_CFLAGS) $(KEILAND_LINUX_CPPFLAGS) $(KEILAND_LINUX_CPPFLAGS_$(subst /,_,$(dir $<))) -MMD -MP -c $< -o $@

KEILAND_LINUX_SOURCES :=
KEILAND_LINUX_ALL :=
KEILAND_LINUX_INSTALL :=

# $(1) name, $(2) SONAME, $(3) sources, $(4) our libraries it links (SONAMEs or a .a), $(5) system libraries,
# $(6) the version script or empty, $(7) more link flags
define KEILAND_LINUX_LIBRARY
KEILAND_LINUX_SOURCES += $(3)
KEILAND_LINUX_OBJS_$(1) := $$(patsubst %.c,$(KEILAND_LINUX_BUILD)/obj/%.o,$(3))
-include $$(KEILAND_LINUX_OBJS_$(1):.o=.d)
$(KEILAND_LINUX_BUILD)/lib/$(2): $$(KEILAND_LINUX_OBJS_$(1)) $$(addprefix $(KEILAND_LINUX_BUILD)/lib/,$(4)) $(6)
	@mkdir -p $$(dir $$@)
	$$(CC) -shared -Wl,-soname,$(2) -Wl,-z,defs $$(if $(6),-Wl$$(comma)--version-script=$(6)) $(7) $$(KEILAND_LINUX_LDFLAGS) \
		$$(KEILAND_LINUX_OBJS_$(1)) $$(addprefix -l:,$(4)) $(5) -o $$@
KEILAND_LINUX_ALL += $(KEILAND_LINUX_BUILD)/lib/$(2)
KEILAND_LINUX_INSTALL += lib/$(2)
endef

# $(1) name, $(2) bin or libexec, $(3) sources, $(4) our libraries it links, $(5) system libraries
define KEILAND_LINUX_PROGRAM
KEILAND_LINUX_SOURCES += $(3)
KEILAND_LINUX_OBJS_$(1) := $$(patsubst %.c,$(KEILAND_LINUX_BUILD)/obj/%.o,$(3))
-include $$(KEILAND_LINUX_OBJS_$(1):.o=.d)
$(KEILAND_LINUX_BUILD)/$(2)/$(1): $$(KEILAND_LINUX_OBJS_$(1)) $$(addprefix $(KEILAND_LINUX_BUILD)/lib/,$(4))
	@mkdir -p $$(dir $$@)
	$$(CC) -pie $$(KEILAND_LINUX_LDFLAGS) $$(KEILAND_LINUX_OBJS_$(1)) $$(addprefix -l:,$(4)) $(5) -o $$@
KEILAND_LINUX_ALL += $(KEILAND_LINUX_BUILD)/$(2)/$(1)
KEILAND_LINUX_INSTALL += $(2)/$(1)
endef

# $(1) the path under the prefix, $(2) the source file
define KEILAND_LINUX_DATA
$(KEILAND_LINUX_BUILD)/$(1): $(2)
	@mkdir -p $$(dir $$@)
	cp $$< $$@
KEILAND_LINUX_ALL += $(KEILAND_LINUX_BUILD)/$(1)
KEILAND_LINUX_INSTALL += $(1)
endef

# $(1) name, $(2) the archive's file name, $(3) sources
define KEILAND_LINUX_STATIC
KEILAND_LINUX_SOURCES += $(3)
KEILAND_LINUX_OBJS_$(1) := $$(patsubst %.c,$(KEILAND_LINUX_BUILD)/obj/%.o,$(3))
-include $$(KEILAND_LINUX_OBJS_$(1):.o=.d)
$(KEILAND_LINUX_BUILD)/lib/$(2): $$(KEILAND_LINUX_OBJS_$(1))
	@mkdir -p $$(dir $$@)
	rm -f $$@
	ar rcs $$@ $$(KEILAND_LINUX_OBJS_$(1))
KEILAND_LINUX_ALL += $(KEILAND_LINUX_BUILD)/lib/$(2)
endef

comma := ,

KEILAND_LINUX_PACKAGES ?= userland/base/libz-compat/Makefile.linux \
	userland/base/libpng-compat/Makefile.linux \
	userland/base/libjpeg-compat/Makefile.linux \
	userland/base/libgif-compat/Makefile.linux \
	userland/desktop/linux-compat/Makefile.linux \
	userland/desktop/libwayland/Makefile.linux \
	userland/desktop/libtruetype/Makefile.linux \
	userland/desktop/libvulkan-compat/Makefile.linux \
	userland/desktop/libkeiland/Makefile.linux
include $(KEILAND_LINUX_PACKAGES)

.PHONY: all install clean
all: $(KEILAND_LINUX_ALL)

install: all
	@set -e; for f in $(KEILAND_LINUX_INSTALL); do \
		mode=0644; case $$f in lib/*|bin/*|libexec/*) mode=0755 ;; esac; \
		install -D -m $$mode $(KEILAND_LINUX_BUILD)/$$f $(DESTDIR)$(KEILAND_PREFIX)/$$f; \
	done

clean:
	rm -rf $(KEILAND_LINUX_BUILD)/obj $(KEILAND_LINUX_BUILD)/lib $(KEILAND_LINUX_BUILD)/bin $(KEILAND_LINUX_BUILD)/libexec \
		$(KEILAND_LINUX_BUILD)/include $(KEILAND_LINUX_BUILD)/share $(KEILAND_LINUX_BUILD)/etc $(KEILAND_LINUX_BUILD)/gen \
		$(KEILAND_LINUX_BUILD)/stage

.PHONY: install-session print-sources header-dependencies
install-session:
	@:
print-sources:
	@printf '%s\n' $(KEILAND_LINUX_SOURCES)

# Include system headers in the audit; -MMD deliberately omits them from ordinary build dependencies.
KEILAND_LINUX_HEADER_DEPS := $(patsubst %.c,$(KEILAND_LINUX_BUILD)/header-check/%.d,$(KEILAND_LINUX_SOURCES))
header-dependencies: $(KEILAND_LINUX_HEADER_DEPS)
$(KEILAND_LINUX_BUILD)/header-check/%.d: %.c | $(KEILAND_LINUX_HEADER_COPIES)
	@mkdir -p $(dir $@)
	$(CC) $(KEILAND_LINUX_CFLAGS) $(KEILAND_LINUX_CPPFLAGS) $(KEILAND_LINUX_CPPFLAGS_$(subst /,_,$(dir $<))) -M $< -o $@
