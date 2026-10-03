# ws063: the SSH guest image (plan/tools/guest) without clang and libcxx,
# for the crash and regression tests of the UFS journal (plan/tools/ufs).
# Build with plan/tools/guest/build-ssh-image.sh, or
#   eval "make -j16 ZEDBSD_CONFIG=plan/tools/guest/config-amd64-ssh.mk \
#     BUILD=build/amd64 $(python3 plan/tools/guest/guest.py extra-files) disk-image"
include plan/ws035/tests/config-amd64-userland.mk
CONFIG_PCAT_SERIAL_MIRROR := y
ZEDBSD_USER_PROGRAMS += openssl openssh
