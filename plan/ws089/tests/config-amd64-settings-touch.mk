# ws089-p012: the Settings image with the test touch screen (/dev/input-inject, CONFIG_INPUT_TEST_INJECT=y, and
# touchinject, as plan/ws079/tests/config-amd64-pen.mk has them) for the drag that scrolls a pane.  A test image only.
# Build:
#   SETTINGS_CONFIG=plan/ws089/tests/config-amd64-settings-touch.mk plan/ws089/tests/build-settings-image.sh BUILD
include plan/ws089/tests/config-amd64-settings.mk
CONFIG_INPUT_TEST_INJECT := y
ZEDBSD_USER_PROGRAMS += touchinject
