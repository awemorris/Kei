# P4 Queue lane

| Queue / attempt | Phase | Scope | Approval | Timebox | State |
| --- | --- | --- | --- | --- | --- |
| q593 / q593-i01 | [ws095-p012](../../ws095/phase012/phase.md) | IME の辞書を千語に拡張、held-out で測る | 2026-10-02 user「作業を開始しましょう。」 | 4h | finished / cleared |

| q595 / q595-i01 | [ws127-p001](../../ws127/phase001/phase.md) | Files の棚卸し（source 不変） | 継続 dispatch（user 2026-10-02） | 3h | finished / cleared |
| q603 / q603-i01 | [ws095-p013](../../ws095/phase013/phase.md) | BUG-139 preedit の大きさ | user 2026-10-02 | 4h | finished / cleared |
| q604 / q604-i01 | [ws095-p005](../../ws095/phase005/phase.md) | 候補の窓と右上の IME の status | user 2026-10-02 | 4h | finished / cleared |
| q606 / q606-i01 | [ws129-p010](../../ws129/phase010/phase.md) | 全 desktop app と base の config | user 2026-10-02 | 3h | in-progress |

Next（予約）: ws127-p002（Files の改善）→ ws089-p010

## Merge requests
| P4-001 | q593 | 1c049e599..c9d9187ad（base 901037f9f） | userland/desktop/ime・plan/ws095 | integrated c7bbbf06a |
| P4-002 | q595 | 7f5e0199f..2142b1fc6（前回 c9d9187ad） | plan/ws127 | integrated 3381b12ab |
| P4-003 | q603 | 12bc714ea（前回 2142b1fc6） | textedit・plan/ws095・BUG-139 | integrated c0449523a |
| P4-004 | q604 | 4ca0b3e80（前回 12bc714ea） | wayland/{input-method,ime.h,compose,protocol,shell}・ime/・vmunix.mk の keiland-ime の link・plan/ws095 | integrated 5634eea24 |
