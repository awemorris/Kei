# A1 / q584-i01 checkpoint13

Complete full C/component review of bind/submit.c covers every constructor,
getter, EventInit conversion and native dispatch operation. [Inventory]
(review-inventory.json):reviewed92/209; remaining117 C/header. p172/q584 remain
in-progress; the whole-Phase/dependent-browser gate remains closed.

Only submit_construct and submit_getter declarations precede the immutable
process-lifetime tables because their initializers require them. read_init and
release declarations occupy the normal post-variable block. Both initializer
bodies remain exactly equal after whitespace removal. No API/ABI/table identity,
native receiver brand, event flags or ownership contract changes.

Review checks genuine actual-owner view selection, type-before-dictionary order,
private native Event/HTMLElement brands and nullable/undefined dictionary semantics.
Each original form/input/event or converted type/dictionary/submitter stack slot
is registered before allocation/user getters. Every failure unwinds only the
successfully registered slots. The event traces its submitter; the form's nested
submission guard opens again after dispatch even on failure. Getter/dictionary
exceptions preserve their actual VM_THROWN/ENOMEM conventions, not errno-only
assumptions. Standard/internal DOM dispatch remains distinct from public view
callback mutation restrictions; no host same-view callback permission changes.

Repairs add precise allocation/root/flag/conversion paragraphs, meaningful error
names, saved callee outcomes and explicit final failure/success returns. Stored
cleanup results equal the prior direct returns; splitting error-versus-zero
identity leaves call/order/root lifetime unchanged. [Verification](verification.json):
GCC14.2.0 independently compiles before/final warning0 and final .text is exactly
identical. The final owned engine object was compiled warning0 and the actual
native submit fixture linked against it plus existing own engine dependencies;
[13/13 native lifetime checks](native-submit.log) pass with empty stderr.

[Formatter](format.json):clang-format19.1.7 then required tabs/initializer rows.
[Style checker](style-summary.txt):zero. Source/managed-document diff-check passes;
raw checkpoint10 disassembly bytes remain excluded. No unnecessary complete
rebuild/runtime-suite repetition; the own shared library still has the prior
identical executable operations, with no repeated final-lib/guest assertion.
Existing host/ASan/target/boot receipts remain retained. No toolchain build,
install, push, aggregate make-check or serial-log judgment.
