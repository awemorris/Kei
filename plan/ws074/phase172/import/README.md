# q579-i01 / P10 import checkpoint 01

Source base `41aac4fc76d0038f9024b0b966d8ae93eb4145fa`, branch
`e53ef03b80113aec959deb67f828cba21d68d4be`, common ancestor
`493b6eea90c45b3c1f393c0c62a0f7882b43621c`.

`classify.py` reads every branch diff entry, applies the checked-in WS107
before→after inventory, classifies new engine files into libbrowser and compares
common/current/branch content. It changes no production source. Staged three-way
outputs are disposable files under `plan/ws074/temp/p172/staged`.

All 569 changed entries are preserved in `manifest.json`: source 92, library
build registration 1, WS074 tests 116, WS074 plan/evidence 264, historical evidence
85, bug evidence 7, excluded shared boards/config 4. Source/build preliminary
classification is clean 55, new 36, clean three-way 2; no textual conflict. Tests
are new 114 and clean 2. These classifications are applicability evidence,
not semantic acceptance; every entry remains unresolved until import/review.

The two three-way merges retain the existing Makefile engine ownership and
main.c's current path/header changes while adding branch sources and bounded
`--settle-ms`. Public browser.h, exports.map, SONAME and view.c were not changed
by browser2. No Wayland API is introduced by path mapping.

Checks: fixed branch/HEAD read-back, 179 inventory entries, full diff 569 entries,
SHA-256 per source version, `git merge-file -p` for diverged sources, Python syntax,
`git diff --check`. Production build/host/guest/boot/full C semantic conformance
remain unperformed at this checkpoint. p172 remains in-progress and uncleared.
GitHub publication is pending; no push or shared record changes.
