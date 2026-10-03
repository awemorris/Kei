# q590-i01 / direct p172 continuation

Current user explicitly requested browser WS continuation without subagents,
WIP commits and sequential pushes to the same named remote `browser3` branch.
This supersedes the former A1-only assignment, wrap-up and no-push limit for this
browser execution. It introduces no new browser feature or acceptance waiver.

Base: origin/main `261903952`, fast-forwarded into the user's clean browser3.
Prerequisites: q584 is finished/uncleared, A1 has stopped; all 209 hashes from
checkpoint14 match actual current source. Full review97/209, other60 finished.
Reserved q590 is used for this first new attempt; q591–q593 reservations are
preserved and no global ID is cosmetically renumbered.

## Exact implementation boundary and acceptance

The remaining112 C/header paths are exactly
[checkpoint14/remaining.json](../import/checkpoint14/remaining.json), SHA256
`06a3d626b5b6a71135652d7505e86845f0c3626778f5d7dddbaaf6702d244d53`.
Review their complete current files against full C and browser component rules,
repair in-scope conformance defects while preserving normal behavior, public
ABI, engine/shell ownership, GC/lifetime, callbacks and error conventions.
Necessary fixture failure checks follow the already recorded q584 policy.
Unrelated implementations, new product/architecture choices, HAL/toolchain and
downstream browser Phases are outside this attempt. Material behavior defects
are recorded separately rather than disguised as formatting.

Keep complete per-file evidence and current hashes. Work in finite coherent
checkpoints; each push includes verified changes and accurately retains pending
review. Token/object equality supplements manual semantic review when changes
are mechanical; it does not prove full-rule conformance. Fully read each file
before claiming a completed review; mechanical scans alone are insufficient.

Use the already loaded full C standard, Guardrail, automation and component
standard. Apply clang-format19 to the scoped source, restore mandatory definition
argument/condition lines, run scoped style/diff checks and the private existing
host builder with warnings0, then focused tests for changed ownership/behavior.
No shared toolchain or external installation is needed. The existing p172 full
boundary/public-client/target/boot and import reconciliation criteria remain
required before whole clearance; retained results are revalidated when affected.

At most three hours from 09:41:27 to12:41:27 UTC, as in the existing p172 finite
attempt policy. If the review or a required gate is incomplete at the bound,
record uncleared with exact remaining paths and resume inputs. Prior q579/q584
outcomes stay unchanged. No whole-Phase clearance or downstream execution is
implied by a checkpoint commit/push.
