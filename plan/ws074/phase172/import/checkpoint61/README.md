# q597 checkpoint61 / browser shell main full source review

Fully reviewed original906 and final931 lines of `userland/desktop/browser/main.c` against full C/component standards and Guardrail. Source SHA256 `75ad356ede3d8a5d29ccd2fa3e224aea44ab49e805936f8096123d3aee2b9470`; all209 inventory hashes checked, reviewed181/209,28 production C/header remain. All65 original C string literals and public ABI remain unchanged; one console write-error diagnostic was added. The shell includes only its shell header, public browser API and desktop paths.

Reviewed mode parsing, headless view callback lifetime, load/settle, CPU/GPU rendering and device release, native font/URL options, DOM/style dumps, script tools and PPM output. Dump, --run console and PPM writes now report output failures as nonzero exits. Normal DOM/style dumps match checked-in sheets goldens, --run prints94 expected PASS lines, and a32x32 CPU PPM is valid. The same dump, --run and render modes exit1 on /dev/full with diagnostics. The first output-probe assertion expected the word 'write' for dump's existing 'cannot dump' diagnostic; adjusted assertion passed with unchanged production behavior.

GCC14.2 scoped `-std=gnu11 -Wall -Wextra -Werror` compile and whole host build exit0/warning0. Style checker exit0/findings0, `clang-format-19` v19.1.7 on copy and manually retained canonical source. DOM23/23 and JS14/14 pass.

Resume with [remaining28](remaining.json) and original whole p172 manifest/semantic/API/ABI/client/native/ASan/target/boot gates. p100 remains blocked/unselected until whole p172 clearance and actual output. GitHub Issue/Project publication remains deferred.
