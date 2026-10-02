# q577 source conformance review

Reviewed 2026-10-02, P8 generation 2. The final attempt is uncleared: original 20/5 acceptance is unmet and the
bounded C9 supplement was stopped on user wrap-up. This review supplies no clearance.

## Authority and final source

Loaded AGENTS.md, Guardrail, the full coding-style.md (all sections),
standards/automation.md, agent protocol, approved phase017 snapshot and actual
nearby shell/Python/Venus capture/compositor/probe implementation. C-specific
rules apply to no changed C implementation here. Non-C code follows established
local conventions and approved Phase requirements. No new global standards,
formatter configuration, test-only production behavior, build, or exception.

Reviewed q577's changed executable sources from base41aac4fc7:

| Source | SHA256 | Role |
| --- | --- | --- |
| `plan/ws035/tests/zdesktop-p076.sh` | `ad8c7369946f95f0baac15cdccda33e58a355d5d3aecf1ce159a71b60ae4ceef` | Shared accepted-request handshake; original expectations retained |
| `plan/ws099/phase017/p076-framed.sh` | `b907de2f69ce317fc0009dd34f6ec46f7949a0a212f079093250fc61d0bf3f2f` | Existing framed observation of the original whole test |
| `plan/ws099/tests/p017-popup-observe.py` | `bf896c0985d4a82ee638a06eb5d33cce7620005d9a06ea411a0c8fc428b664ab` | Original stages1–4 with immutable first pixel verdict and six later observations |

The generated observation shell in evidence retains its own copy and SHA256
receipt. Operational recovery scripts in ignored build/ are diagnostic process
controls, not changes to the production or shared test source. The inherited
resume script is preserved as historical evidence despite its missing environment.

## Semantic review

The shared handshake counts accepted requests before down, accepts only a newly
increased count while held, excludes refused move lines, and stops before motion
when count is invalid or the bounded wait fails. Timeout releases the held button,
saves ordinary logs/errors, and exits FAIL. The stale-count negative observation
in generation1 verifies count3 does not falsely pass and only release is sent.
The moved log and all original 550/200/800 dimensions, right-edge anchors, settled
logs and expected pixels remain required. A prior request cannot replace a new one.
Other popup/ping/maximize/minimize expectations remain unchanged.

The new popup observer guards every source substitution with a unique pattern
check, quotes the actual working directory, parses the generated shell before
execution, keeps each output isolated, and records subprocess status. Initial
pixel failure always propagates even if a later image matches. The observed first
failure instead remains a FAIL for every image; no production change or shared
pixel retry was applied. Observation is shortened and uses concurrent host load,
so it is not a whole-p076 count or a proof of historical failure identity.

No source in compositor/HAL/toolchain/Noct/LLVM/shared build was edited or built.
No serial or console log was read for any verdict. Guest application logs, process
environment for the named renderer variables, and Venus VNC screenshots provided
evidence. Existing boot-test PASS/login PNG is retained; no new boot was needed.

## Commands and limits

- Python3.13.5: `python3 -m py_compile plan/ws099/tests/p017-popup-observe.py` PASS.
- `/bin/sh` is dash0.5.12-12: `sh -n` on shared p076, p076-framed and generated observe.sh PASS.
- `git diff 41aac4fc7..HEAD --check --` the three named executable sources PASS.
- New Python/Markdown staged whitespace check PASS. Full raw evidence whitespace
  check is not clean: twelve original C9 result lines at MR05 retain trailing spaces; new supplemental
  raw results also retain their original trailing spaces.
  Evidence hashes and original contents are preserved; that is not a source PASS.
- Artifact SHA256 read-back: 35 original/observation evidence files PASS at MR05.
- QEMU10.0.11, existing strict-queue Venus renderer, imageSHA256
  `992e83f498cda2a1d506bb6ad7975b3c4592069fcb6493eca707e0d84403ac37`.
- C formatter/style-check and product build are not applicable to the changed
  shell/Python source. Aggregate make check, toolchain build and physical tests
  were not run. Full WS099 standards Phase is separate, outside this Queue.

Single20/fiveC9 zero-failure acceptance is unmet. Historical failed geometry and
C2 q538 remain unresolved outside this limited harness synchronization evidence.
