<!-- awesome-plan project=zedbsd record=past-log -->

# Past Log

Last finished Queue: [q576](queue-q576.md)（WS111 p002 cleared）

## 最新: q576 /WS111 completed

L1〜L3: 両OS shell/native build/install、FreeBSD/opt script0755、LinuxGDM direct entry unchanged、全source review PASS。[結果](/home/awe/zedBSD-claude1/plan/history/ws111/q576/result.md)。--loginは本人確認/PIN交換案のみ、WS110/testing未実装。

WIP commit、今回のlauncher git push/実機pullは既存指定環境の承認を利用。Issue/Project公開保留。WS110/testing/--loginは検討のみ。

## Queue history（直近30、古い順）

| Queue | Outcome |
| --- | --- |
| [q547](queue-q547.md) | WS108 p003 uncleared |
| [q548](queue-q548.md) | WS108 p003 cleared |
| [q549](queue-q549.md) | WS108 p004 cleared |
| [q550](queue-q550.md) | WS109 p001 uncleared / driver license・起動方法・realdevice回答待ち |

| [q551](queue-q551.md) | WS109 p001 item cleared /Phase cleared |
| [q552](queue-q552.md) | WS109 p002 item uncleared /Phase uncleared |
| [q553](queue-q553.md) | WS109 p002 item cleared /Phase uncleared |
| [q554](queue-q554.md) | WS109 p004 item cleared /Phase uncleared |
| [q555](queue-q555.md) | WS109 p004 item cleared /Phase uncleared |
| [q556](queue-q556.md) | WS109 p002 item cleared /Phase uncleared |
| [q557](queue-q557.md) | WS109 p003 item cleared /Phase uncleared |
| [q558](queue-q558.md) | WS109 p003 item cleared /Phase uncleared |
| [q559](queue-q559.md) | WS109 p003 item cleared /Phase uncleared |
| [q560](queue-q560.md) | WS109 p002 item cleared /Phase uncleared |
| [q561](queue-q561.md) | WS109 p002 item cleared /Phase uncleared |
| [q562](queue-q562.md) | WS109 p002 item uncleared /Phase uncleared |
| [q563](queue-q563.md) | WS109 p002 item uncleared /Phase uncleared |
| [q564](queue-q564.md) | WS109 p002 item cleared /Phase uncleared |
| [q565](queue-q565.md) | WS109 p002 item cleared /Phase cleared |
| [q566](queue-q566.md) | WS109 p005 item cleared /Phase uncleared |
| [q567](queue-q567.md) | WS109 p003 item uncleared /Phase uncleared |
| [q568](queue-q568.md) | WS109 p003 item uncleared /Phase uncleared |
| [q569](queue-q569.md) | WS109 p003 item cleared /Phase uncleared |
| [q570](queue-q570.md) | WS109 p003 item cleared /Phase cleared |
| [q571](queue-q571.md) | WS109 p004 item cleared /Phase cleared |
| [q572](queue-q572.md) | WS109 p005 item cleared /Phase cleared |
| [q573](queue-q573.md) | WS109 p006 cleared |
| [q574](queue-q574.md) | WS109 p007 cleared |
| [q575](queue-q575.md) | WS111 p001 cleared |
| [q576](queue-q576.md) | WS111 p002 cleared |

前回全文は[保存済みindex](ws109/q551/previous-past-log.md)。以前の全summary/判断/bugリンクは[through q548](past-log-through-q548.md)、[through q537](past-log-through-q537.md)、[through q522](past-log-through-q522.md)。各承認scope/結果はQueue archiveを参照。

2026-10-02 /ws111-completed-20261002: p001/p002に加えWS L1〜L3を照合、completed。userのGDM直接entry/pw本人確認→PIN案を保持。--login実装は検討のみ。

## 計画の追記 / 2026-10-02

Event ws112-package-plan-20261002: userの「あとで実装」に従い[WS112](../ws112/ws.md)（5OS package作成/CI全5種類release）をplannedで追加。RPi OS arm64、ad hoc生成可、CI runtime不要、FreeBSD source-only。
実装・build・Queue実行は無し。q576 finishedと旧WS108/WS111の完了証拠は保持。新実装候補はOutlookのみ、Issue/Projectとrepository pushは未実施。

2026-10-02 / ws112-rpi-build-only-20261002: 追加user決定: RPiはbuildが通ればOK。GPU/GUI確認を要求しないことをWS112とaffected Phase/方針へ反映、計画のみ。
