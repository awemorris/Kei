# P1 Queue lane

| Queue / attempt | Phase | Scope | Approval | Timebox | State |
| --- | --- | --- | --- | --- | --- |
| q590 / q590-i01 | [ws004-p051](../../ws004/phase051/phase.md) | BUG-134: AX211 の passthrough での再現・解析・driver の修正と確認（phase.md の範囲） | 2026-10-02 user「PCIパススルーでQEMUを起動してデバッグしておいてほしいです。これは最初に取り組みましょう！」 | 4 時間 | finished / uncleared（実機試験待ち） |
| q594 / q594-i01 | [ws129-p009](../../ws129/phase009/phase.md) | デモの image を CI 設定の土台に、boot-test の timeout | 2026-10-02 user「デモのイメージはCI設定をベースに変更しましょう。」 | 3 時間 | finished / uncleared（full image 待ち） |

| q596 / q596-i01 | [ws005-p018](../../ws005/phase018/phase.md) | WiFi の利用者の流れの調査と契約 | 継続 dispatch（user 2026-10-02） | 3 時間 | finished / cleared |

| q598 / q598-i01 | [ws033-p001](../../ws033/phase001/phase.md) | USB の LAN の hotplug を QEMU で | 継続 dispatch | 3 時間 | in-progress |

Next（予約）: ws118-p001 → ws005-p019（方式の判断後）

## Merge requests

| P1-001 | q590 | 5a9da8080（base 257b2bd9f） | plan/ws004・BUG-134・Bug Board | integrated 487372080（BUG-134 の衝突は P1 側を採用） |
| P1-002 | q594 | 22efda2be（base c4ed68f12、前回 5a9da8080） | plan/ws075/demo・plan/tools/boot-test.*・plan/ws129/phase009 | integrated eeecec752 |
| P1-003 | q596 | 79126012c..3019d674b（前回 22efda2be） | plan/ws005/phase018 | integrated 7cbbd3fc5 |
