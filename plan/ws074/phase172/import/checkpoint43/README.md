# p172 checkpoint43 / q596-i01

Full C/component review of form-owner308, native clone354, table-mutation354 and
table-row436 original lines. Review158/209, remaining51 C/header; all209 source
hashes reconcile with checkpoint42. q596/p172 whole remains in-progress, WS
incomplete, p100 unselected.

Actual calls check execution before successful teardown or source release.
Form runtime uses one forward cleanup while preserving its existing installation/
script error diagnostic. Native clone cleanup removes only still-acquired
construction/result registrations; construction ownership ends **before** the
original direct clone call and output ownership starts only **after** publication.
Before field reads, clone input must still exist, output must be distinct/non-null,
Text/Element kinds and character/attribute buffers must be real. Table receiver,
borrowed host, native parent/anchor and conversion object are checked before reads.
Callback observer state remains integer-only; all original tree edges are severed
and C stacks excluded during actual GC. Infrastructure cleanup restores the
embedding stack boundary; no new root retains tested native participants.

Full semantic review covers actual DOM form/listed/controls predicates and Element
branding through unchanged ordinary script-created trees; native detached clone
source/copy/Document ownership, independent exact UTF16/surrogate buffers,
namespace URI/prefix/attribute identity, default pressure and real capped ENOMEM;
native table caption/body/header/row/old-child/insertion-parent/anchor/new-child
lifetimes, borrowed child host selection, numeric valueOf once, real host EIO
unwind and eventual reclamation. Original assertions, C strings, native scripts,
6000/512 clone corpus, limits/default pressure and caller-root exclusions remain.
No production edits or synthetic engine switch.

GCC14.2 -Wall/-Wextra/-Werror final scoped compile/link exit0/empty log against
actual current own engine objects: after-commands.json/compile-link.log/
build-result.json. Actual form-owner56, clone26, mutation42 and row75 =199,
timeout30s each/exit0/empty stderr: tests.json/individual stdout/stderr.
clang-format19.1.7 plus tabbed definitions, split pointer loops/compound clauses/
one-argument-per-line calls: format.json/versions.txt. Supporting checker exit1
solely37 forward shared-cleanup jumps, all allowed by full C§14, no other findings:
manual-goto-review.json/style detail/summary. Full actual authoritative source
coverage: full-standards-review.json. Original/final source/object-text hashes and
equal ordered C literals: object-code.json. Text changes are expected from guard/
unwind improvements. git diff --check passes; all209 hashes reconcile.

Whole final boundary/API/client/ASan/target/boot and51 remaining full reviews are
not claimed. q596 original finite3h/whole criteria remain; main only/all WIP/normal
origin/browser3 push, Issue/Project publication deferred. Prior cleanup complete.
