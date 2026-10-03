# p172 checkpoint35 / q594-i01

Complete full C/component reviews: host-xml-binding.c288,
host-document-parser.c292, host-document-stream.c303 original lines.
Inventory138/209 reviewed; remaining71 C/header, zero other entries.

XML binding review covers29 actual native XML assertions, genuine CDATA/PI
brands/prototype snapshots, CharacterData/Range/Text/traversal/serialization/
layout/style/script behavior, real8MiB threshold collection with native input
unrooted, nullable caller root, saved wrapper-only Document/target retention and
last-root reclamation. Fallible conversion/construction/script/global publication
is immediately guarded; borrowed input releases on each outcome, caller root
removes before Page teardown. Original table/script/assertion strings unchanged.

Parser review covers managed child/window-owned parser stacks, detached open
subtree traced solely by parser, continued parse after GC, parser destruction,
unfinished child stream finalization and independent standalone tracer lifetime.
Required generated frame/host/tree observations are checked before dereference.
A single forward cleanup point releases acquired fixture roots and an unfinished
standalone parser on error; owner-published parsers retain normal owner teardown.
No additional fixture retention root or disabled production GC path.

Stream review covers factory Document replacement refusal, borrowed operations
on actual child Document, native open/identity/loading parser publication,
inline execution with real GC, nested write and reentrant close depth, actual
retirement during callback and eventual context/unfinished parser collection.
Expected exception is checked immediately; frame/host/wrapper checked before
publication. Callback's obsolete frame pointer clears before final collection.
Normal scripts/expected observations/production behavior unchanged.

## Finite verification

- Scoped GCC14.2.0 -Wall/-Wextra/-Werror fixture compiles/links against actual
  current own production objects, exit0/warnings0: compile-link.log.
- Actual XML34/34, parser8/8, stream9/9, all exit0/empty stderr:
  tests.json and individual results. Real GC pressure and C-stack exclusion
  remain; no allocation failure injection or final whole acceptance claimed.
- clang-format19.1.7 plus mandatory definition/semantic restoration and original
  constant table layout: format.json. All C strings equal, actual object-text
  changes recorded: object-code.json. git diff --check and full final diff read.
- Supporting style checker exits1 solely for24 forward jumps to parser_case's
  one shared cleanup point: style-summary.txt/manual-goto-review.json. No other
  findings; allowed by full C section14.

All209 source hashes reconcile. q594 scoped five-file item remains in-progress,
whole p172 not cleared, WS incomplete. Two selected files still to review;
whole remaining71/final conformance gates retain their original criteria.
No target/boot/ASan/downstream campaign. All fixture processes ended;
Issue/Project publication deferred. Next real WIP commit triggers requested
unused build scratch cleanup after archived evidence/read-back checks.
