# ws005-p024: the lean zdesktop guest for the Venus QEMU (plan/ws035/tests/zdesktop-guest.sh), with the real
# networkd and sessiond's greeter, for the login and logout notices of the Wi-Fi stores (no radio).  The
# Settings image (plan/ws089/tests/config-amd64-settings.mk) without the networkd stand-in.
include plan/tools/files/config-amd64-files.mk
ZEDBSD_USER_PROGRAMS += settings
