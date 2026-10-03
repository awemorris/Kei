# q598 checkpoint88 / input constructor caller roots

Selected adjacent `bind/input.c` repair: KeyboardEvent and WheelEvent wrappers are rooted from `bind_event_construct` return through the additional reentrant getters and conversions, then released on success and error. The source is outside the fixed209-file import review inventory and is not added to its reviewed count. All209 inventory hashes still match;204 full reviews, five pending. C string literal contents and public ABI unchanged.

GCC14.2 whole/scoped warning0, JS14/14, DOM23/23. Style checker reports only seven permitted forward cleanup jumps; clang-format19.1.7 ran on a copy. Forced collection through every init getter branch was not run. Original whole p172 gates and p100 prerequisite remain.
