# q597 checkpoint69 / Window and realm full source review

Fully reviewed original1411 and current1455 lines of `bind/window.c` against full C/component standards and Guardrail. All209 inventory hashes match source; reviewed188/209,21 production C/header pending. All original C string literal instances and public ABI are unchanged. The review covered managed/unmanaged window creation, ownership transfer and destruction, interface/global publication, console/script/checkpoint, Window methods, event delivery, and tracing.

Moved only table-referenced callback declarations ahead of the constant Window tables; other forward declarations now follow the tables. `postMessage` now roots its converted options/origin, event and delivery callback across allocations until the timer retains them, with one cleanup path for all outcomes.

GCC14.2 scoped compile and whole host build exit0/warning0. Style checker lists eight permitted forward cleanup jumps and no other findings; `clang-format-19` v19.1.7 ran on a copy. Native frame lifetime148/148 and realm30/30, DOM23/23, settle14 runtime +8 diagnostic pass. A separate postMessage probe accepted wildcard, same-context slash, exact-origin and options overload, and rejected mismatched origin; it delivered four messages in order. No isolated forced-GC injection inside postMessage was performed.

Resume from [remaining21](remaining.json) and original whole p172 gates. p100 is blocked/unselected until whole p172 clearance and actual output. GitHub Issue/Project publication remains deferred.
