# Reserved q593 / WS099 p019 background asset resume notes

2026-10-02 / read-only B2 investigation. **No implementation started.**
Main created p019 at `eea8b913`, then recorded default decisions at `706d2460`.
A reserved q593 for B2, next global ID q594. Later user direction requires B2
to wrap up after q588 and exit; q593 remains unexecuted pending future dispatch.

## User decisions to preserve

- Put the old blurred birch/lake background in the source tree for common use
  by zedBSD, Linux and FreeBSD; also include the linear abstract version if found.
- Use the blurred lake as the common initial startup default. Keep existing
  bundled pictures available through desktop background selection.
- Existing saved wallpaper preference remains higher priority; no instruction
  authorized overwriting user settings.
- Copy existing bytes, no image generation/editing. Keep user provenance and
  signature; do not assert a new license without evidence. Old WS035 p061
  completed acceptance stays preserved; the new instruction replaces its former
  git-outside asset storage policy through new WS099 p019.

## Confirmed source candidates

Read-only source directory: `/home/awe/zedBSD-claude1/build/ws035-wallpaper/`.
Main visually confirmed v2-soft-b.png is the tested blurred lake; B2 viewed the
original/blur comparison sheets. No image was changed.

| Existing asset | Dimensions / bytes / SHA256 |
| --- | --- |
| wallpaper.ppm | 1280x800 / 3072016 / 3616eb2147caa717dbd53b5e151283f8942fbd2b1b2aa5cf54f47cad6639b687 |
| v2-soft-b.png | 1280x800 / 721373 / 6ed9573292dc0004401d434e18d32eede24d9acda72e079566033a245ea6f4e6 |
| wallpaper-1080.ppm | 1920x1080 / 6220817 / 5f9f70c1bedf387e78d1d1edbc169b9ba7a15b50e5f70855a909075db78dbb1d |

The 1280x800 PNG and PPM have identical RGB pixels. The 1080 version has different
blur strength; do not silently substitute it. WS035 p061 records user-provided
birch/mountain/lake artwork signed LEEKING 26, the later 1672x941 replacement,
central 1506x941 crop, 1280x800 version and chosen existing processing. There is
no independently verified distribution license; keep provenance and the current
user's explicit source-inclusion direction without applying code's Zlib label
to the image.

Suggested exact new source paths (not approved/created):
`userland/desktop/wallpapers/Birch-Lake.ppm`, `Birch-Lake.png`, and `README.md`
with provenance, source hashes, dimensions and PPM/PNG relationship. PNG archives
the existing viewable asset; runtime uses the existing PPM. A found abstract
candidate needs main/view_image confirmation, original bytes/provenance and an
explicit source path before formal dispatch.

## Current startup/catalogue and minimal recipe proposal

- `userland/desktop/keiland-linux.mk:143` currently prefers an untracked
  build/ws035 wallpaper if present, otherwise generated Aurora.
  `keiland-freebsd.mk:144` defaults to generated Aurora. Both already install
  share/keiland/wallpaper.ppm and five generated catalogue pictures.
- Change each native `*_WALLPAPER ?=` default to the common checked-in PPM;
  retain caller overrides. Add a Birch-Lake catalogue copy. Preserve the existing
  generator and all five Aurora/Dawn/Lagoon/Meadow/Twilight entries.
- zedBSD `userland/desktop/wayland/Makefile` package DATA currently carries fonts,
  without wallpaper. Its existing `ZEDBSD_USERLAND_PACKAGE` DATA argument feeds
  root Makefile's ZEDBSD_USERLAND_DATA_INPUTS/FILES/MODES, so adding the default
  and catalogue data here can avoid root/build/OS source edits. Existing five
  generated pictures should be registered with a dedicated own BUILD generation
  target here, retaining the unchanged generator. Review standalone package
  BUILD/root resolution before choosing that rule.
- `sessiond/session.sh:16`, sessiond greeter.c/SESSIOND_WALLPAPER and native
  `wayland/keiland-desktop.in` already use installed wallpaper.ppm. The Linux
  display-manager .desktop does likewise. Saved absolute wallpaper preferences
  override the startup path in wayland/preferences.c; startup C changes appear
  unnecessary.
- `settings/look.c` adds the default first, then sorted catalogue `*.ppm`, up to
  seven after the default (SE_WALLPAPERS=8). Five existing entries plus lake and
  one abstract fit the current limit. Selection index 0 clears the wallpaper key;
  other entries save their path. Settings source edits appear unnecessary.
- Existing test/demo image scripts add untracked wallpaper and generated five
  entries by ZEDBSD_TEST_EXTRA_FILES. A future scope must check duplicates/source
  precedence and move relevant owned WS099 criteria builder to the checked-in
  default; do not silently assume only package DATA controls every test image.

## Finite validation proposal

Own build/staging paths only, safe dry-runs before actual targets. Verify exact
asset copies by hashes, dimensions and RGB match; visually confirm the copied
PNG. Verify native data targets and staged install membership/default bytes for
both OS recipes, zedBSD expanded release data membership/default/catalogue
paths/modes and existing five entries. Capture current launcher argv and check
actual preference priority/Settings catalogue selection via relevant existing
host fixtures or a bounded asset check. Shell/Make/Python/full-rule review and
diff checks as applicable. Guest startup/default switching confirmation needs
formal Queue scope, a staged independent image and resource grant.

Do not report data-target/install staging as full native release build, a frozen
manifest check as fresh image construction, or host contract evidence as all
three OS runtime success. Shared packaging source ownership needs main/A
coordination before implementation.

## Abstract asset search outcome and bounds

Not identified. Searched both checkout build inventories for wallpaper/abstract/
birch/geometric names, home nonbuild image paths by wallpaper/abstract/birch/lake/
background names, related WS035/WS089/WS099 plan references, and bounded /tmp image
paths. The v2 sheets and 1080 previews depict blurred lake versions, not a linear
abstract picture. Old generated/fallback landscapes do not establish identity
as the requested second asset. The search does not prove no unnamed attachment
or unavailable historical file exists. Candidate path lists remain under
`build/b2-ws094-p007/{wallpaper-candidates,nonbuild-image-paths}.txt`.
