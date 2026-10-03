# Browser3 / q590-i01 checkpoint22

Complete full C/component review of host-xml-node.c285, host-xml-document.c342
and host-attribute-namespace.c301 original lines. [Inventory](review-inventory.json):
118/209 reviewed; [remaining91 C/header files](remaining.json), zero other files.
q590/p172 remain in-progress and downstream gates stay closed.

Node fixture review follows actual PI/CDATA factory collection, immutable target,
exact lone-surrogate data, interval mutations, invalid inputs, clone roots and
final reclamation. Native calls are individually checked before observation;
partial root-registration failure removes all successful slots, and test failure
also releases every registered slot/realm/heap. Explicit success/void exits and
semantic paragraphs satisfy full rules.

Document fixture review follows connected/detached/implementation-only roots,
retired primary host, prototype snapshot, fresh caller identity and final owner
reclamation. Binding results are checked before native dereference; the Document
is found by observation address after collection before dereference. Temporary
units/unrelated caller release occurs after immediate failure checks, with
complete failure cleanup. Original three graph cases and expected values remain.

Attribute fixture review follows exact URI/local-name identity independent of
atoms, alias duplicate rejection, ordered native array growth/removal, clone
URI ownership and last-root collection. Each initial allocation is checked
immediately. The existing post-GC URI observation must survive before another
lookup uses it; otherwise fixture returns EINVAL through owner cleanup. This is
necessary fixture failure detection, not a production behavior change.

## Finite verification

- Three final GCC14.2.0 fixture compiles/links use actual unchanged engine objects
  and exact isolated host flags: exit0/warnings0. [Commands](compiles.json).
- Native XML node32/32, XML document16/16 and attribute namespace25/25:
  73 checks pass, exit0/empty stderr. [Commands/results](tests.json).
- All original script/assertion/C string literals exactly equal. All three actual
  fixture object texts differ after failure guards/rearrangement; no executable
  equality claim. [Hashes](object-code.json).
- Scoped clang-format19.1.7 then definition tabs/compound layout [receipt](format.json),
  full manual review, supporting [style checker](style-summary.txt)0 and
  successful git diff --check.

No production changes or unnecessary full rebuild; no new target/guest/ASan or
downstream campaign. Remaining91 full reviews/final conformance required;
GitHub publication deferred. All owned compile/test processes completed.
