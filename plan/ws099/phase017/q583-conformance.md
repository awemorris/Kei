# q583 source conformance

Scope: new [p017-popup-timeline.py](../tests/p017-popup-timeline.py), its ten generated
stage1–4 runners, result/diagnosis records and unchanged raw evidence.
AGENTS.md, Guardrail, protocol/session-b, C coding-style.md全文 and automation.md
were loaded. New non-C code follows the established Python/shell diagnosis
conventions; C-specific formatting/static checking and product builds do not
apply to this source scope. No exception or production diagnostic switch added.

Python in-memory `compile()` PASS. `sh -n` PASS for both preparation variants
and all ten actual guest runners. Scoped source/Markdown `git diff --check` PASS.
Full manual review confirms license, fresh output directories, shared source
boundary checks, preserved original expectations, first-verdict failure retention,
six bounded observations, separate host/guest clock domains, fresh count polling,
ten-second transport timeout and whole-run termination of only owned shell children.
Guest runtime cleanup is separate and confirmed by the receipt.

Host negative controls: original capture FAIL remains FAIL after six later PASS;
the expected pixel/coordinates remain unchanged; stale/timeout readiness never
sends the second click. Stubbed transport/capture checks do not prove guest behavior.
Actual guest original5/handshake5 partial runs all PASS; each snapshot returned0,
and each handshake returned fresh readiness. No actual guest timeout was exercised.
Product source, shared regression and hidden production behavior are unchanged.

Shared p076 SHA256 before/after:
`ad8c7369946f95f0baac15cdccda33e58a355d5d3aecf1ce159a71b60ae4ceef`.
Original q577 evidence was retained. New raw PNG/log bytes were copied unchanged
and hashed. Whitespace in raw application logs is original evidence and is
excluded from source/Markdown formatting claims. No credentials or serial/console
logs are included. Product build/C9/physical/full-WS conformance remain unexecuted
under this partial diagnosis scope. Whole p017 remains uncleared.
