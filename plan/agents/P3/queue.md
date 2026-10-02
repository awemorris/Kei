# P3 Queue lane

| Queue / attempt | Phase | Scope | Approval | Timebox | State |
| --- | --- | --- | --- | --- | --- |
| q592 / q592-i01 | [ws114-p007](../../ws114/phase007/phase.md) | CSD/SSD の残り5点（新 attempt） | 2026-10-02 user「作業を開始しましょう。」 | 3h | finished / cleared |

| q597 / q597-i01 | [ws115-p001](../../ws115/phase001/phase.md) | 素の GTK4 移植の契約 | user 2026-10-02「まずは素のGTK4を移植してください」 | 4h | finished / cleared |
| q600 / q600-i01 | [ws115-p004](../../ws115/phase004/phase.md) | meson の cross 契約と host 道具 | 継続 dispatch | 4h | finished / cleared |
| q602 / q602-i01 | [ws115-p005](../../ws115/phase005/phase.md) | libffi・pcre2・glib | 継続 dispatch | 4h | in-progress |

Next（予約）: ws115-p006（libpng・freetype・harfbuzz・fontconfig）〜

## Merge requests

| P3-001 | q592 | 8856edf21（base 901037f9f） | plan/ws114 evidence/tests のみ | integrated debba7e9e |
| P3-002 | q592 | b2ebbfee6（前回 8856edf21） | plan/ws114 evidence/結果 | integrated d10ad2cf3 |
| P3-003 | q592 | 597841d2a（前回 b2ebbfee6） | plan/ws114 | integrated 3b985ae4c |
| P3-004 | q597 | 33e061c3b..be587f8ae（前回 597841d2a） | plan/ws115 | integrated 408a31586 |
| P3-005 | q600 | 503b94c80（前回 be587f8ae） | external.mk・packages/tools/gen-meson-cross.sh・devel/gperf・plan/ws115 | integrated 81f77738d |
