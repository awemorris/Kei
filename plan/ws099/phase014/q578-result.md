# q578-i01 result / 2026-10-02 06:24 UTC

Outcome: Queue item cleared; ws099-p014 cleared. Main recovered final verification after P9 stopped at the model usage limit. No replacement model was used.

- Command: `sh plan/ws099/tests/c10-hw.sh build/ws099-p014/fresh-5ac9b753d.img build/ws099-p014/c10-full-fresh 60` in P9 worktree.
- Actual continuous operations: **3602 seconds / 278 rounds**, errors=0, restarts=0, exit.status=0. The suggested 300 rounds is a guide, not the acceptance threshold; the required hour was measured directly.
- Retained ten apps; repeatedly opened/closed another Terminal, moved windows, opened Wiseview and App Home. Main inspected final actual live-plane PNG: Terminal completed the fresh receipt/log-copy/sync command and returned to its prompt. Disk log contains `C10_HW_SAVED_1790918269`; sessiond records initial autologin and no greeter retry/failed.
- Hardware: 5330 i915 PCI passthrough at solaris10-man/10.0.10.25. Source image 5ac9b753d, SHA256 `72003343313d85b5e0950f5659ca6af5a6183ef3c0776c68a1e7ecdfba0b3e7e`. Later browser import is not in this image; compositor binary unchanged. This is **passthrough**, not USB/native bare-metal verification.
- Fresh short validation: 14 rounds / 187 seconds / errors0 / restarts0 / exit0, [evidence](evidence/fresh-short/). Old interrupted short is retained separately and does not count as PASS.
- Ownership cleanup read back: /tmp/i915-h4-owner absent, flock /tmp/i915-hw.lock free, original remote QEMU24995 absent, no remote QEMU, GPU remains vfio-pci. No rebind/reboot was performed.
- Final script conformance: full shell source reviewed for bounded timing, actual request/receipt checks and owned resource cleanup; `sh -n plan/ws099/tests/c10-hw.sh`, code diff whitespace checks PASS. No compositor C changes. Script commits4bdd9224f/21042cf4e integrated at7fc178474/3d1acf0cf. Subsequent commits add evidence only.
- Supplementary framebuffer boot for the passthrough config timed out twice without a PNG; [preflight](main-image-preflight.md) records this limitation. It is not substituted for the required actual i915 screenshots and hour receipt.
- [Full evidence](evidence/fresh-full/) includes periodic live-plane PNGs, final screenshot, disk session/sessiond, output, exit and hashes. Console/serial logs excluded from acceptance.

All p014 criteria verified. WS099 remains incomplete: other criteria/Phases retain their own acceptance. GitHub intended Phase closure and progress comment remain pending publication by configured hold; no remote closure claimed. No push.
