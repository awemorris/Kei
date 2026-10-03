# ws005-p020: the Venus desktop guest of ws005-p024 (plan/ws005/phase024/config-venus.mk: the real networkd and
# sessiond, Settings, kei logged in at boot) with the RTL8822B firmware (/lib/firmware/rtw88/rtw8822b_fw.bin), for
# the TP-Link 802.11ac NIC (2357:0138, RTL8822BU) of the development host passed through by usb-host.  No key is in
# the image.  Build:
#   make ZEDBSD_CONFIG=plan/ws005/phase020/config-venus-rtl.mk BUILD=... disk-image   (or build-rtl-image.sh)
include plan/ws005/phase024/config-venus.mk
ZEDBSD_USER_PROGRAMS += rtl8822b-firmware
