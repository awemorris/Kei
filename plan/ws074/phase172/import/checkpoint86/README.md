# q597 checkpoint86 / Array helpers partial review

Partial review of `js/builtin_array.c` across its 2427 original lines. Fixed error output read, transient GC roots for sort and join, and scratch allocation failure. Existing string literals and public ABI remain unchanged. All209 source hashes match checkpoint86 inventory; reviewed count stays204/209, five files pending.

GCC14.2 whole and scoped builds exit0/warning0. JS tests14/14 pass. Style checker reports only five permitted forward cleanup jumps; clang-format19.1.7 ran on a copy. Forced GC within all getter/comparator paths and allocation failure injection were not run.

This is partial: newly produced arrays in other methods still require reentrant GC root review. Keep this source pending with the other four and original whole p172 gates. p100 remains blocked/unselected. GitHub Issue/Project publication deferred.
