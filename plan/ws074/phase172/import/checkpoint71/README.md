# q597 checkpoint71 / DOM query, classList and dataset full source review

Fully reviewed original1529 and current1778 lines of `bind/query.c` against full C/component standards and Guardrail. Reviewed190/209,19 production C/header pending; all209 inventory hashes match. Original C literal instances and public ABI are unchanged. The review covered selector conversion/error handling, live engine and owner selection, static result arrays, class token validation/reading/mutation and dataset native accessors.

Only table-referenced callback declarations precede constant tables. Temporary results now have explicit GC roots through selector parsing, query result construction, dataset publication, class token arrays/conversions and accessor construction. Each root is removed on success and failure. A pressure page exercised 3000 selector matches, 400 class add/remove cycles and dataset access; it did not force GC at a selected allocation.

GCC14.2 scoped compile and whole host build exit0/warning0. Style checker lists25 permitted forward cleanup jumps and no other findings; clang-format19.1.7 ran on a copy. DOM23/23, child-style page30 PASS and pressure page assertions pass.

Resume from [remaining19](remaining.json) and original whole p172 gates. p100 remains blocked/unselected. GitHub Issue/Project publication remains deferred.
