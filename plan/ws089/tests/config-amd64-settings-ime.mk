# ws089-p010 (BUG-146): the Settings image with the input method, as a desktop image has it.  zdesktop starts
# keiland-ime itself and it connects first, so Settings is client 2: the tests find its window by find_window
# (settings-wait.sh), not by the client number.  Build:
#   SETTINGS_CONFIG=plan/ws089/tests/config-amd64-settings-ime.mk plan/ws089/tests/build-settings-image.sh BUILD
include plan/ws089/tests/config-amd64-settings.mk
ZEDBSD_USER_PROGRAMS += keiland-ime ime-dict-ja
