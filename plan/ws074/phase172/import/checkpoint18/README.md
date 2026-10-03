# Browser3 / q590-i01 checkpoint18

Complete full C/component review of page/link.c438, bind/svg-text.c451 and
host-resource.c428 original lines. [Inventory](review-inventory.json):109/209
reviewed; [remaining100 C/header files](remaining.json), zero other files.
q590 and whole p172 remain in-progress; downstream gates remain closed.

## Reproduced cleanup defect

Legacy page_fetch leaked independently decoded data MIME/body storage when
appending into the caller's byte buffer failed. Existing host-resource now
exercises the actual wb_buffer overflow guard after genuine data decoding,
four times, with synthetic length restored before ordinary caller teardown.
No engine replacement or production test switch. Baseline actual production
objects linked with `-fsanitize=leak` exit23:512 leaked bytes in8 allocations.
[Before result](resource-before.stdout), [provenance/fixture patch](lsan-before.json).
The [internal amendment](../../browser3/q590-amendment-02.md) records the narrow
repair: release decoded fields on both append outcomes and propagate the
original error. Final same focused LeakSanitizer link exits0,105/105 resource
checks and no leak diagnostic. [After](resource-after.stdout).

## Full review and conformance

Link review covers borrowed hit-tested nodes/attribute atoms, independent
URL representations, file/data/HTTP acquisition, redirects, declared MIME,
owned response fields, failed-output reset and generic decoded extensions.
All fallible operations have immediate guarded cleanup and explicit outcomes;
successful bytes/metadata and original error conventions remain unchanged
apart from the corrected leak. Response cleanup uses one shared forward label.

SVG review follows canonical local-name brands/inheritance, stable receiver
and traversal root slots across CSS callbacks, display:none ancestors and
subtrees, retirement/disconnection, normalized UTF16 whitespace, CRLF and
surrogate counts, bounded arithmetic and iterative preorder. Only its table
callback declaration precedes immutable tables; other prototypes follow them.
Failed style operations now stop before unrelated traversal queries, preserving
their original error/zero-output contracts. No metric API or placeholder added.

Resource fixture reviews actual local/loopback/data responses and MIME sentinel
matrix, preserving the six local cases' order and their original expectations
in an explicit table. Fallible setup/independent URL checks unwind immediately;
void observers and byte comparison have explicit outcomes. Four new overflow
observations are the leak regression; earlier normal observations remain.

## Finite verification

- Own GCC14.2.0 plain host rebuild: exit0/warnings0. [Build log](plain-build.log).
- Final resource105/105 passes with both plain and focused LeakSanitizer links;
  SVG text42/42, viewport12/12 and link22/22 pass, exit0/empty stderr.
  [Commands/results](tests.json). Initial host-link invocation omitted required
  fixture arguments and exited2/usage; retained and corrected, not a product failure.
- clang-format19.1.7 then mandated argument tabs/compound layout:
  [receipt](format.json). [Style checker](style-summary.txt) exits1 with only12
  goto findings. Each is a single forward jump to its function's one shared
  cleanup label, permitted by full C §14: [manual classification](goto-review.json).
  No rule exception is introduced. `git diff --check` passes.
- [Object hashes](object-code.json) disclose all three differing texts after
  cleanup/control-flow changes. No executable equality is claimed.

LeakSanitizer observes allocation leaks through the real libc allocator; this
focused link is not an ASan memory-boundary campaign. No target/guest/downstream
suite ran; whole final conformance remains outstanding. GitHub publication
remains deferred; own build and finite test processes have exited.
