<!-- awesome-plan project=zedbsd record=ws108p004 -->

# ws108p004: 全文規約・両 distro の最終 install/session 回帰

Status: cleared
Disposition: normal
Parent: [WS108](/home/awe/zedBSD-claude1/plan/ws108/ws.md)
Queue / Attempt: q549 / q549-i01

## 目的・範囲

全 source/CI/packaging 変更を標準で review し、fresh target 両方で最終 package を検証。

## 完了条件

P1〜P5。必要なQEMU GPU/session証拠とCI定義/local同手順検証を照合する。remote Actions/publishは未実行と区別（q545確定設計/p003契約）。

## 前提・未決・実行手順

依存: p003、WS106 の確定配置（scoped output）。WS の scope と acceptance、設計の未決を確認する。
技術的な細部は委任範囲で決める。対象/受け入れ/外部契約を変える結果は実装前に計画と承認範囲へ反映する。
調査→変更表と手順の確定→有限 Queue の承認→実装→指定検証→結果・WS・Master・Queue の照合。
最初の p001 は調査の案（1 session / 最大60分、満たせない点と再開条件を残す）。後続の timebox/command は設計後に選定する。

## 適用規則・影響する部品・検証

[WS の制約/部品/受け入れ](/home/awe/zedBSD-claude1/plan/ws108/ws.md)、[Guardrail](/home/awe/zedBSD-claude1/plan/guardrail.md)、
[C 規約全文](/home/awe/zedBSD-claude1/plan/coding-style.md)、[自動化](/home/awe/zedBSD-claude1/plan/standards/automation.md)を適用。
新規/変更 C は全文該当節を読み、clang-format-19 と style-check の限界を補う。
最終 conformance は全 WS の source を全文で review。build warning 0、必要な契約検証、diff-check を記録する。
具体的な build/config/tool version と script は p001 の結果で固定する。`make check` は禁止。
zedBSD の起動は boot-test.sh の PNG。Linux の既存検証は WS105 の手順と許可範囲、FreeBSD は検証環境の確定が必要。

## 初期計画の証拠・再開条件（2026-10-01の履歴）

未実行。command/result/commit/artifact/skipped checks はまだ無い。計画の作成を clearance としない。
再開: prerequisite の実 output と変更の所有、scope snapshot、Queue 承認を確認する。

## イベント

2026-10-01 / review-20261001-planning: 新設した Phase 案。親 WS の目標への寄与と依存を記録。実装の選定は未実施。
GitHub の Phase 作成/comment/Project の projection は公開保留、local outbox に保持する。

## 2026-10-02 / ws108-user-targets

ユーザー（このchat）: make keiland-linux-debianでDebian13のdpkg、make keiland-linux-ubuntu2604でUbuntu26.04のdpkg。ビルド自体もそれぞれQEMU guest内、dpkgを各guestへ導入して動作試験、既存CIに組込みnightly release filesにする。「現在のQueueを完了したら、WS108を実行してください。」をp001〜p004 finite1Phase Queueの実行承認として保存。
旧container案をQEMU nativeへ置換、旧release対象外をrelease filesへ置換。既存/opt layoutとWS105のLinux対応済production内容を維持、test appはruntime package外。amd64は既存対象の具体化。実release publication/pushはこのsessionでは行わずCI定義を作成・local同手順検証、remote runは未実施と分ける。

2026-10-02 / ws108-q545-design: [確定設計](/home/awe/zedBSD-claude1/plan/ws108/design.md)へ具体化、p001 inputs/manifest/readiness→p002指定targets/2OS native guest/deb/runtime→p003既存CI/release→p004full conformance。各Phase自身の内容/verify/time boundsは設計該当節。container/release除外の旧案はユーザー指示で置換。dependency順は維持、WS106未確定ime-probeは対象外で待たない。GitHub Phase/WS delivery pending。

## 最終検証の具体化（q547設計）

p003のcommitted最終source3a5b5b16からの両make target/TCG native build＋fresh install/session結果をp004でcode hashを照合して最終証拠として再確認。コードが変わればaffected build/runtimeを再実行。全WS108 Python/JSON/Makefile/YAML/規則差分を全文/manual review、元production C/reused fixtureは変更無し。remote CI未実行はp003の承認されたlocal同driver/ゲスト手順の実証と区別。CI定義とlocal fail gatesがP4、remote Actions/release実行をしたとは言わない。最終guestはMaster通り8GiB。shell/YAML/byte compile/diff-check、payload hashes/modes/19ELF/control/root-owner/session/conffile/md5sum/licenses/ユーザーデータを照合。zedBSD production/APIは不変で既存CI build構造を比較、WS107の最新boot証拠を参考contextにし新boot試験を主張しない。

2026-10-02 / ws108-q547-final-source: p003のTCG input失敗による検証runner修正を10362bd3で固定。最終native/runtime証拠はこのcommitの両make exit0を用いる。旧3a5b5b16のsource hashを最終sourceとはせず、失敗ログを保存。p004のfinal-code再照合・affected checks義務を維持。

ws108-q548-redesign: p003 q547はuncleared。次q548のp003 retryのactual両make/TCG runtime＋全payload parityを最後のsource証拠とする。p003 clearedまでp004は開始しない。own acceptance/full rules scope不変、[origin](/home/awe/zedBSD-claude1/plan/history/ws108/q548/phase.md)。

## q549 最終scope

最大60分。final production/tool source b0e1eaf9をgit hashで照合、q548の両make exit0＋両fresh TCG全smoke＋native TCG/KVM全40payload一致を最終証拠として確認。全WS108 source8filesとAGENTS/Guardrail/C全文の適用範囲、licenses/private roots/native dependencies/user data/failure/CI/既存build維持をmanual full review。byte compile/YAML/bash syntax/combined release verifier/independentpayload audit/git diff-check/停止processを確認。後続code変更無しならnative buildを重ねない。WS ownP1〜P5、Master/Outlook/PastLog/archive/syncpendingを照合、Phase dirsはverified archive後削除。

## 結果 / q549-i01 / 2026-10-01T18:21:56.494202+00:00

cleared。P1〜P5 verified。最終b0両make exit0/native QEMU、fresh KVM/TCG各OSのpublic Vulkan/GUI/実input/upgrade/remove PASS、TCG native/KVM全40payload一致、19ELF/control/md5/root/hash audit、CI/release正負gate/YAML/既存build維持/全source規約レビュー。plan/history/ws108/conformance.md。remote Actions/publish未実施、outbox pending。

Event ws108-q549-cleared: Phase結果/closure意図をlocal保存、remote comment/closeはdeferred。
