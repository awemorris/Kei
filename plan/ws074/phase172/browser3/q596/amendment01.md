# q596 / internal p172 function ownership amendment01

2026-10-03 JST, q596-i01. The selected full function/factory review reproduced
actual default-GC input and unpublished-object losses. This is an internal fix
under existing selected paths/acceptance, not an additional Phase or changed API.

Original bca821b4a producer: default parent environment probe exit2 with missing
actual parent; unpublished native output probe exit2; bytecode and closure probes
both signal11. Exact fixture, compile/run argv, hashes and stdout/stderr are retained
in import/checkpoint41/baseline-functions.json and baseline-function-probe.txt.
The first factory-only repair passed all four under the same real 8MiB accounting.
The next independent actual reentrant native collector lost all five otherwise
unowned participants on the still-original dispatcher: callable, receiver,
argument, saved callee and saved new.target. baseline-native.json/probe.txt retain
that failure and the exact factory-only producer patch/source hash. Dispatcher
repair retained all five in the same probe. A numeric address is only an
observation, never an added collector root. Missing cells are not dereferenced.

Factories now retain managed realm/code/environment inputs before VM allocation,
new function/name/prototype cells before subsequent allocations, and release only
successfully registered roots. Parent allocation size is checked for host-size
overflow. Closure prototype helpers rely on the enclosing closure root and own
their unpublished private prototype. Direct call/construction/prototype lookup
scopes retain cell inputs and collectible caller realms through actual getters,
reentrant execution, exception transfer and publication. Native entry separately
retains suspended callee/new.target while realm fields are replaced, then restores
them before dropping its temporary roots. No collector threshold, test-only
production switch, bytecode/corpus/native-root exclusion, public ABI or component
boundary changes. C-array/root registration allocation errors unwind normally.

The existing default host-vm-factory fixture receives the four actual allocation
regressions and the real reentrant collector test, including post-return state,
name/length, bytecode/capture result and eventual collection after caller retirement.
Verification: stable-source own plain warning-as-error build, focused real native
ownership/realm/interpreter checks, fixed checked-in JS expected outputs, final
format/full C/component semantic review and all209 source-hash reconciliation.
Whole p172 target/boot/API/ASan and remaining full-source gates stay required.
Fault injection is not performed; exact acquired-root unwinds are manually reviewed.

Resume/output: import/checkpoint41/README.md after bounded final verification.
Phase remains in-progress, WS incomplete, p100 unselected. Original whole acceptance
and q596 finite timebox remain unchanged. Issue/Project publication deferred.
