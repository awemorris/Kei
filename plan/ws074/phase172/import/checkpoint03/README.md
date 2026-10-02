# Main recovery checkpoint03 / 2026-10-02

Recovered after P10 model usage-limit cessation; committed b5c4875db and merged by main. [C token comparison](main-token-review.json): all32 modified C files equal to checkpoint02 tokens. Changes are comments, split-call layout and one tab-only whitespace fix. Production semantics unchanged by this checkpoint. Source/test format records preserve canonical style, not blind formatter output.

Python regression runner review: explicit argv allowlist remains, no shell execution;900-second inventory bound,120-second individual bound, incremental reports and OSError classification prevent interrupted groups disappearing. Python AST/JSON parsing and git diff whitespace check pass. This does not prove its unexecuted error paths with tests.

[Boundary](boundary.json):167engine sources/377headers, no project Wayland/shell/UAPI dependency,39exports unchanged/SONAME libbrowser.so,public C89/C++11 clients. [Goldens](goldens.json):81/81 per plain and ASan. [Recovered ASan groups](regressions-asan-recovered.json):98 executable groups pass, golden group separately verified. Earlier concurrent builder resource error is preserved, not an assertion failure silently retried. No further builds while executables are running.

Main separately verified native target build warning0, boot login PNG and p014 shell rendering/input/window move/close, [evidence](../../main-target.md). No source other than comment/whitespace changed since those native tests.

Full manual C-standard review of remaining imported changed source is outstanding. style-check zero and token equality are supporting checks, not a waiver or completed full review. p172 remains in-progress; downstream browser work stays blocked. Acid3 score100 is verified but pixel37.04% is not the current p100 goal. No push/GitHub publication.
