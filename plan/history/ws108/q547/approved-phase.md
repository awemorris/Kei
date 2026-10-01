<!-- awesome-plan project=zedbsd record=ws108p003 -->

# ws108p003: CI の2 distro job と artifact

Status: planning
Disposition: normal
Parent: [WS108](../ws.md)
Queue / Attempt: なし（未承認）

## 目的・範囲

固定 target ごとに p002 の package build・検証を走らせ、.deb/buildinfo/checksum を upload。既存 nightly job と権限を維持。

## 完了条件

P4。実 job の結果または同じ環境/手順の検証を記録し、remote CI 未実行は区別する。

## 前提・未決・実行手順

依存: p002。WS の scope と acceptance、設計の未決を確認する。
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

## 2026-10-02 / ws108-user-targets

ユーザー（このchat）: make keiland-linux-debianでDebian13のdpkg、make keiland-linux-ubuntu2604でUbuntu26.04のdpkg。ビルド自体もそれぞれQEMU guest内、dpkgを各guestへ導入して動作試験、既存CIに組込みnightly release filesにする。「現在のQueueを完了したら、WS108を実行してください。」をp001〜p004 finite1Phase Queueの実行承認として保存。
旧container案をQEMU nativeへ置換、旧release対象外をrelease filesへ置換。既存/opt layoutとWS105のLinux対応済production内容を維持、test appはruntime package外。amd64は既存対象の具体化。実release publication/pushはこのsessionでは行わずCI定義を作成・local同手順検証、remote runは未実施と分ける。

2026-10-02 / ws108-q545-design: [確定設計](../design.md)へ具体化、p001 inputs/manifest/readiness→p002指定targets/2OS native guest/deb/runtime→p003既存CI/release→p004full conformance。各Phase自身の内容/verify/time boundsは設計該当節。container/release除外の旧案はユーザー指示で置換。dependency順は維持、WS106未確定ime-probeは対象外で待たない。GitHub Phase/WS delivery pending。

## q547 の具体手順

最大60分。既存CIへ2 distro matrix追加、両指定target/nativeQEMU手順、artifact distinct names、missing/checksum/runtime fail gate、release needs/download/添付を実装。GitHub公式matrix/download-artifact v4仕様確認済み。remote Actions/push/publish未実行、q546の同driver actual guest成果とlocal YAML/shell/release input検証を用い、最終committed両targetはp004。TCG選択をrelease driverの普通の実行設定として明示可能にし、CIと同じ非KVM手順をp004で実行。既存build jobを構造比較。
