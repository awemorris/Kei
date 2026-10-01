# q546 native package / fresh install evidence

Both distro native builds: make keiland-linux-debian / make keiland-linux-ubuntu2604,
clean QEMU build overlays, Debian GCC14.2 / Ubuntu GCC15.2. No source compiler
warnings. Package 40 payload files, 19 runtime ELF files. 27 dpkg-shlibdeps
warnings each are existing private unversioned SONAME name/version inference;
no missing-info suppression, external libc computed and dlopen system Vulkan
explicit. libdrm headers are build input; direct DRM uses kernel UAPI, not a
runtime libdrm link. Vulkan driver packages supply their own libdrm dependencies.

Initial attempts stopped for snapshot base/common omission, then an incorrect
runner socket name; corrected allowlist and Linux wayland-keiland contract.
Input runner also had an SSH key-path/method collision, fixed by key_path.
Production source unchanged. Final focused fresh overlays passed installed public
Vulkan client, compositor, Terminal QMP real input/touch, reinstallation,
genuine Version+smoke1 upgrade/conffile preservation, remove/purge/user data.
Python byte compile, target dry-run, snapshot dependency closure and checksum
verification passed. Corrupt image cache rejected before QEMU.

Native candidates are commit 301a1cc6025b plus uncommitted packaging code; exact
metadata attached. Full final make targets on committed packaging source, including
md5sums/correct maintainer/8GiB guests/source_dirty, are required in p004.
These exploratory guests used 4GiB; final helper honors Master 8GiB policy.
Physical GPU/display-manager session not newly executed, registration checked.
No host installation, toolchain change, push or remote Actions/Issue publication.
