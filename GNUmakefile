# Native FreeBSD goals avoid parsing the unrelated zedBSD cross toolchain rules.
KEILAND_FREEBSD_ENTRY_GOALS := keiland-freebsd keiland-freebsd-install
ifneq ($(strip $(MAKECMDGOALS)),)
ifeq ($(filter-out $(KEILAND_FREEBSD_ENTRY_GOALS),$(MAKECMDGOALS)),)
.PHONY: keiland-freebsd keiland-freebsd-install
keiland-freebsd:
	$(MAKE) -f userland/desktop/keiland-freebsd.mk all
keiland-freebsd-install:
	$(MAKE) -f userland/desktop/keiland-freebsd.mk install
else
include Makefile
endif
else
include Makefile
endif
