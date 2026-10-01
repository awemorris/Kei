# q548 retry cleared / final source b0e1eaf972e1

make keiland-linux-debian and make keiland-linux-ubuntu2604: both exit0,
QEMU native build + separate fresh QEMU install overlay, auto/KVM, 8GiB.
Final packages under build/keiland-deb/artifacts/{debian13,ubuntu2604}.
Native GCC14.2 / GCC15.2; source compiler warnings0. Private unversioned SO
inference warnings27 per dpkg-shlibdeps are classified, not suppressed.

Final b0 packages also passed the entire smoke in independent fresh TCG guests:
public Vulkan, direct KMS compositor + desktop Files, Terminal map/click/actual
keyboard touch command, reinstall, genuine +smoke1 upgrade/config preservation,
remove/purge/user data. TCG screenshots can lag terminal processing; the actual
file is independently checked via SSH. No physical GPU/performance assertion.

q547 native TCG built both OS packages. All40 final native KVM payload paths,
hashes and modes exactly match their TCG native builds; C/compiler flags are
unchanged. build.py change is serial staging + parent mkdir, not payload generation.
Thus native TCG build and final TCG runtime were verified as separate executions;
a single final complete TCG make and remote Actions were not executed. Actual
final complete make commands above used KVM. CI uses the same final functions,
explicit TCG, timeout45min and release fail gates. Remote runner/action execution
remains unexecuted due to no push/publication.

q547 stays uncleared. q548 initially found parallel staging mkdir failure on
Ubuntu; serial install/install-session and parent mkdir fixed it. Upgrade fixture
now uses a normal uncompressed deb (Version+smoke1); released deb compression
unchanged, block timeout600sec. No production C/toolchain/host install changes.

Independent payload audit: exact40 files, hashes/modes/root ownership19 ELF,
control/md5sums/conffile/session/test-app exclusions/licenses PASS. CI/YAML/
negative checksum gates were tested in q547; final positive verifier runs follow.
