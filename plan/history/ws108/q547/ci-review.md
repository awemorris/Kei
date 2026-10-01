# q547 CI integration review

Source 3a5b5b16 WIP. PyYAML6.0.2 parsed matrix/job/release structure;
all embedded run scripts passed bash -n. The build job is deep-equal to its
pre-WS108 YAML (checkout/dependencies/toolchain/image/optional zip/upload),
and push/PR triggers + permissions remain unchanged. Release retains its
main-push condition, image/zip files and description, adding native deb files.

Two matrix entries call the exact requested make targets. QEMU TCG is explicit,
so nested KVM is unnecessary. Each image is pinned and verified; build and test
use distinct fresh overlays with native target identity checks. The host script
failure fails the job; no packaging continue-on-error. Upload requires output,
distro names are distinct. Release needs build + the entire package matrix.
Download v4 pattern/merge-multiple is supported by official v4 README:
https://raw.githubusercontent.com/actions/download-artifact/v4/README.md
GitHub matrix/fail-fast behavior:
https://docs.github.com/en/actions/how-tos/write-workflows/choose-what-workflows-do/run-job-variations

verify-artifacts checks exactly one distro package, complete hashes, metadata,
manifest/runtime filters, matching runtime package hash + all smoke checks,
PNG signatures and clean-source status. Missing package and corrupted actual
candidate metadata were rejected locally. It is an operational release gate,
not a replacement for actual native QEMU/runtime tests. Final TCG runs on this
committed source are in progress; positive evidence is not yet claimed.

No remote workflow, upload or release has been executed. Local host QEMU10.0.11/
Python3.13 differs from a hosted runner; target guest images/native OS/procedure
are identical, with TCG explicitly selected. Remote Actions runner/service/action
execution is an unexecuted limitation. CI timeout45min bounds the entire job;
individual boot/apt/build/SSH timeouts and Guest cleanup bound local calls.
