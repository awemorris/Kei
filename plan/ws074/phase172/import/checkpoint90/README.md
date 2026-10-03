# q598 checkpoint90 / Array helper and transient roots

Partial `js/builtin_array.c` review continued. Shared element helpers now retain receiver and transient values during VM property operations. `flat/flatMap` recursion retains mapped values, and `reverse` retains both fetched values through a reentrant swap. Existing C string literal contents and public ABI are unchanged. All209 inventory hashes match; the file remains pending full review, so progress is204/209 with five full files left.

GCC14.2 whole/scoped warning0, JS14/14, DOM23/23. Style checker reports only33 permitted forward cleanup jumps; clang-format19.1.7 ran on a copy. Generic primitive receiver lifetime and additional transient paths remain to review; forced GC through all callbacks and ENOMEM injection remain untested. Original whole p172 gates also remain.
