<!-- awesome-plan project=zedbsd record=ws106p001 -->

# ws106p001: 対象・参照・build 契約を確定

Status: cleared
Disposition: normal
Parent: [WS106](../ws.md)
Queue / Attempt: q539 / q539-i01

## 目的・範囲

対象表の 30 件を全数照合し、package ID/category/config 互換、grouping と package.mk の設計、変更する runner、WS095 との所有を確定する。移動は行わない。

## 完了条件

台帳と移動手順、実際の build/config/check command を固定。所有が未調整なら ime-probe の実行を選定しない。

## 前提・未決・実行手順

依存: なし。WS の scope と acceptance、設計の未決を確認する。
技術的な細部は委任範囲で決める。対象/受け入れ/外部契約を変える結果は実装前に計画と承認範囲へ反映する。
調査→変更表と手順の確定→有限 Queue の承認→実装→指定検証→結果・WS・Master・Queue の照合。
最初の p001 は調査の案（1 session / 最大60分、満たせない点と再開条件を残す）。後続の timebox/command は設計後に選定する。

## 適用規則・影響する部品・検証

[WS の制約/部品/受け入れ](../ws.md)、[Guardrail](../../guardrail.md)、
[C 規約全文](../../coding-style.md)、[自動化](../../standards/automation.md)を適用。
新規/変更 C は全文該当節を読み、clang-format-19 と style-check の限界を補う。
最終 conformance は全 WS の source を全文で review。build warning 0、必要な契約検証、diff-check を記録する。
具体的な build/config/tool version と script は p001 の結果で固定する。`make check` は禁止。
zedBSD の起動は boot-test.sh の PNG。Linux の既存検証は WS105 の手順と許可範囲、FreeBSD は検証環境の確定が必要。

## 証拠・結果・再開条件

未実行。command/result/commit/artifact/skipped checks はまだ無い。計画の作成を clearance としない。
再開: prerequisite の実 output と変更の所有、scope snapshot、Queue 承認を確認する。

## イベント

2026-10-01 / review-20261001-planning: 新設した Phase 案。親 WS の目標への寄与と依存を記録。実装の選定は未実施。
GitHub の Phase 作成/comment/Project の projection は公開保留、local outbox に保持する。


## 2026-10-01 調査による scope 補完（q539）

ユーザー回答「13 ファイルも追加して移動する」を採用。base/tests直下の syscall-smoke.c、posix-r2.c、posix-r2-remaining.c、
susv4-xsi.c、posix-phase5-helper.c、smp-resource-stress.c、dyntest.c、tlstest.c、rpathdep.c、rpathtest.c、versiontest.c、versionuse.c、versiontest.map を userland/tests/ 直下へ。
これに付く grouping Makefile と platform の source/object/map/runner参照も更新する。
対象30 package は保持。T1/T2 の移動/参照台帳は追加13filesを含む。T3/T4もこの追加範囲のbuild/hash/規約reviewへ適用。
[移動手順と検証](../design.md)、[台帳](../survey.json)。p001→p002→p003の依存は変更しない。
Event ws106-q539-scope-update: 調査で初期inventoryの漏れを発見し、ユーザーの具体的追加指示と全変更Phase/WSへの影響を記録。
GitHubの各Phase/WS event deliveryはoutbox pending。ime-probe所有確認と既存style扱いの回答を待つ。

2026-10-01 / ws106-q539-style-decision: 上記style質問は解決。既存implementation style維持の[限定例外](../../standards/ws106-relocation.md)をユーザーが許可。ime-probe所有確認はpendingで、未調整の間は実行対象から外す。

## 実行結果 / q539-i01（2026-10-01T14:19:02.675047+00:00）

cleared。30 package＋承認追加13files、全166 tracked fileのhash/mode/source→destinationと参照217fileを棚卸し。menu/package/config/install互換とbuild/boot手順をdesign.mdへ確定。既存styleはユーザーの限定例外を記録。ime-probeは人間作業との非競合回答まで選定から外す条件で、他29件と13filesは実行可能。証拠: plan/ws106/survey.json、programs-before.txt、style-before.txt、design.md。

Event: ws106-q539-cleared。Phaseの結果と意図したclosureをlocal記録。GitHub comment/closeはdeferred、remote closure未確認。
