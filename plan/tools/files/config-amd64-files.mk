# ws071 (File Manager): a lean zdesktop guest image for the Venus tests, the ws070
# System Menu image (plan/tools/titlebar/config-amd64-menu.mk) with files.
# Build:
#   plan/tools/files/build-files-image.sh [BUILD]
include plan/tools/titlebar/config-amd64-menu.mk
ZEDBSD_USER_PROGRAMS += libz-compat libpng-compat files
# ws079-p006: PDF Viewer (Files opens PDFs with it) and libpdf (with libjpeg-compat).
ZEDBSD_USER_PROGRAMS += libjpeg-compat libpdf pdfviewer
# ws127-p002: without the input method.  zdesktop starts keiland-ime itself (WS095) when it is installed, and
# it connects first, so the window under test would be client 2 where every guest test of files looks for
# client 1 (all of files-regress.sh failed so on 2026-10-03).  The tests do not type through an input method.
ZEDBSD_USER_PROGRAMS := $(filter-out keiland-ime ime-dict-ja,$(ZEDBSD_USER_PROGRAMS))
