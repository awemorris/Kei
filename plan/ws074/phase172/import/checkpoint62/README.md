# q597 checkpoint62 / DOM node partial source review

Partial review of `userland/desktop/libbrowser/dom/node.c`: original1069 and current1104 lines, source SHA256 `0959155814f7d5c168d441778ab2fc528b9378b197aadfdc582a2877f5bb9549`. All209 inventory hashes checked, but this file **remains pending**: reviewed181/209,28 production C/header remain. All6 C string literals and public ABI are unchanged.

DocumentType creation now retains its three input strings during collectible node allocation. Attribute array growth now refuses multiplication overflow before realloc. Full caller ownership is not yet resolved: `bind/implementation.c` converts three potentially user-defined strings before construction, and `html/modes.c` constructs three strings in sequence. Revisit those callers and revalidate any changed previously reviewed source before marking this DOM file complete. This is a scoped ownership finding, without an isolated forced-GC reproduction claim.

GCC14.2 scoped `-std=gnu11 -Wall -Wextra -Werror` compile and whole host build exit0/warning0. Style checker exit1 lists exactly two permitted forward jumps to one cleanup label, other findings0; `clang-format-19` v19.1.7 ran on a copy. Native cloning26, attribute namespaces25, XML DOM54, XML binding34, native properties65 and DOM23/23 pass.

Resume from [remaining28](remaining.json) and the identified caller paths. Original whole p172 gates remain; p100 is blocked/unselected. GitHub Issue/Project publication remains deferred.
