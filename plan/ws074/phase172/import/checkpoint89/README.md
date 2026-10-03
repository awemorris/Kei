# q598 checkpoint89 / Array result and accumulator roots

Partial `js/builtin_array.c` review continued. Constructor prototype/new Array, produced copy/deleted arrays, removed values and reduce accumulators now have explicit GC roots across reentrant property operations. Existing C string literal contents and public ABI are unchanged. All209 inventory hashes match; the file remains pending full review, so progress is204/209 with five full files left.

GCC14.2 whole/scoped warning0, JS14/14, DOM23/23. Style checker reports only29 permitted forward cleanup jumps; clang-format19.1.7 ran on a copy. Forced GC through all callbacks and ENOMEM injection remain untested. Continue transient value and generic receiver review before marking this file full; original p172 whole gates also remain.
