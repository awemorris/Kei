<!-- awesome-plan project=zedbsd record=ws107 -->

# WS107: libbrowser の source 所有と独立コンポーネントの整備

Status: completed
Primary Milestone: MG006
Related Milestones: MG002
Parent: [Master](../master.md)
Queue: q544 finished
Resume point: B1〜B5 verified（2026-10-02 JST）。GitHub publication/Issue close/Projectはdeferred。

## 目標と結果

ユーザーの2026-10-01レビュー第2項と2026-10-02「WS107を実行してください。」を実行。
engine165filesをdesktop/libbrowserへ移し、browserはmain/shell/app data14filesを所有。browserと独立clientは公開browser.h経由でlibbrowser.soへ動的link。
libbrowserは標準Vulkanを使い、Wayland public/private source/include/link/APIを持たない。shellがwindow/inputを公開抽象入力へ渡す。
API v2を維持し、寸法/stride/target入力、navigation allocation rollback、async history commit/失敗・cancel、INT_MINを修正。
callback中の同じview変更/破棄を外側call後に行うユーザー契約を文書化。

## WS acceptance / 制限

B1 source ownership、B2 dynamic component、B3 Wayland-free/public-only client、B4 view/input/callback/GPU/failure ownership、B5必要回帰/全文規約/bootを[最終conformance](../history/ws107/conformance.md)で照合。
plain/ASan+UBSan各83、host-view59、goldens81、Acid2 pixel差0、native Venus shell status0、boot login PNG。WS107 build warning0。full imageの既存外部warning343行は記録、toolchain/無関係packageを変更せず。

実機/production Linux browser/Qt/GTK/Acid3/CSS2/amazon改善は範囲外・未実施。WS074 p100→p101を保持し、今後は新rootを使う。WS106のime-probe所有回答待ちは別の未達で保存。
Primary MG006へ独立componentの成果、Related MG002へ検証出力を提供。milestone全体完了の証拠ではない。
内容不変moveの既存style維持はユーザーの[今回限りの例外](../standards/ws107-relocation.md)、今回の適用終了。quality/new clientはC全文。

## Phase / Queue

| ID | Result | Goal / evidence |
| --- | --- | --- |
| [ws107p001](../history/ws107/q541/phase.md) | cleared / q541 | 所有台帳、有限quality、callback契約と設計。commit efcc8d6f |
| [ws107p002](../history/ws107/q542/phase.md) | cleared / q542 | engine165 move/build/runner references。commit2770e481 |
| [ws107p003](../history/ws107/q543/phase.md) | cleared / q543 | API v2/component品質/独立動的client。commit0283edb0 |
| [ws107p004](../history/ws107/q544/phase.md) | cleared / q544 | 全文規約/B1〜B5/回帰/最終boot。final WIP commitの親0283edb0 |

[設計](design.md)、[移動台帳](../history/ws107/inventory.json)、[結果・判断履歴](../history/ws107/ws-at-acceptance.md)、[boot PNG](../history/ws107/q544/evidence/login.png)。再利用試験は[plan/tools/browser-component](../tools/browser-component/README.md)。

Event ws107-completed-20261002: 全Phase clearedに加えWS自身のB1〜B5を検証してcompleted。Phase directoriesを証拠archive照合後に削除、GitHubのcomment/close deliveryは未実施。ユーザーの後続WS108実行指示へ移る。
