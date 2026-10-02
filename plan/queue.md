<!-- awesome-plan project=zedbsd record=queue -->

# Queue / all-agent index

Active Queues: q590（P1）。
Status: active
Main executor / plan writer: Q1（単一 Claude Code セッション、[protocol](agents/protocol.md)）。サブエージェント P1〜P8、N=2。
Last reconciled Queues: [q584](history/queue-q584.md)〜[q589](history/queue-q589.md)。過去の Queue は [Past Log](history/index.md)。次の未予約 ID は q591。

| Queue / attempt | Agent | Phase | Exact scope | State | Approval / checkpoint |
| --- | --- | --- | --- | --- | --- |
| q590 / q590-i01 | P1 | [ws004-p051](ws004/phase051/phase.md) | BUG-134 AX211 の passthrough 再現・解析・修正、4h | in-progress | user 2026-10-02「PCIパススルーでQEMUを起動してデバッグ…最初に取り組みましょう！」、[lane](agents/P1/queue.md) |

## 直近の終了 Queue の残り（再開の候補、承認ではない）

| Queue | Phase | 結果 | 残り |
| --- | --- | --- | --- |
| [q584](history/queue-q584.md) | [WS074 p172](ws074/phase172/phase.md) | uncleared | browser 全文 review 97/209、残 112。p172 が後続 browser Phase の前提 |
| [q585](history/queue-q585.md) | [WS112 p001](ws112/phase001/phase.md) | uncleared | D1（Fedora/Arch の boot 適用）のユーザー回答待ち |
| [q586](history/queue-q586.md) | [WS113 p001](ws113/phase001/phase.md) | uncleared | D-ATOMIC 未決 |
| [q587](history/queue-q587.md) | [WS114 p007](ws114/phase007/phase.md) | uncleared | CSD/SSD 実装済み、最終 runtime・clipboard・Textedit・boot 等 |
| [q588](history/queue-q588.md) | [WS094 p007](ws094/phase007/phase.md) | 部分 cleared / whole uncleared | 実機・guest・boot 等 |
| [q589](history/queue-q589.md) | [WS099 p017](ws099/phase017/phase.md) / BUG-125 | uncleared | 低 overhead 診断の準備のみ、guest 未実施 |

## Upcoming Work Outlook

fg019（ベータ1のリリース）の内容をユーザーと議論して決めてから、優先作業と P1/P2 の最初の Queue を選ぶ。候補・順序は Master の [Upcoming Work Outlook](master.md#upcoming-work-outlook) と上の残り。どれも承認ではない。
