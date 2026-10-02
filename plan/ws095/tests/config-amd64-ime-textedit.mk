# ws095-p013: the input method's lean Venus guest image (config-amd64-ime.mk) with the Text Editor and the
# Japanese fallback font, for the preedit in the editor's body (BUG-139).
# Build:
#   plan/ws095/tests/build-ime-image.sh [BUILD]   with ZEDBSD_CONFIG=plan/ws095/tests/config-amd64-ime-textedit.mk
include plan/ws095/tests/config-amd64-ime.mk
ZEDBSD_USER_PROGRAMS += textedit
