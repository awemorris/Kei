# q597 checkpoint84 / CSS parser full source review

Fully reviewed original2489 and current2541 lines of `css/parser.c` against full C/component standards and Guardrail. Reviewed203/209,6 production C/header pending; all209 inventory hashes match. Existing C string literal contents and public ABI are unchanged; the `s` attribute flag literal was added.

Unclosed attribute and functional selectors now fail before range arithmetic; attribute selectors reject trailing tokens. `:nth-*()` rejects fractional/out-of-range numbers and bounds signed arithmetic. `@supports` preserves ENOMEM as an error instead of silently treating it as unsupported. Added malformed-selector regression cases to the DOM query fixture.

GCC14.2 whole host build and scoped compile exit0/warning0. Style checker0 findings; clang-format19.1.7 ran on a copy. CSSOM12/12, CSS rule model44/44, CSSOM mutation12/12, JS14/14 and DOM23/23 pass. ENOMEM injection in `@supports` was not available; normal paths and error propagation were reviewed.

Resume from [remaining6](remaining.json) and original whole p172 gates. p100 remains blocked/unselected. GitHub Issue/Project publication remains deferred.
