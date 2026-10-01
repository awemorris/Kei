<!-- awesome-plan project=zedbsd record=ws108p003 -->

# ws108p003: CI の2 distro job と artifact

Status: cleared
Disposition: normal
Parent: [WS108](/home/awe/zedBSD-claude1/plan/ws108/ws.md)
Queue / Attempt: q548 / q548-i01

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

## q547 の具体手順

最大60分。既存CIへ2 distro matrix追加、両指定target/nativeQEMU手順、artifact distinct names、missing/checksum/runtime fail gate、release needs/download/添付を実装。GitHub公式matrix/download-artifact v4仕様確認済み。remote Actions/push/publish未実行、q546の同driver actual guest成果とlocal YAML/shell/release input検証を用い、最終committed両targetはp004。TCG選択をrelease driverの普通の実行設定として明示可能にし、CIと同じ非KVM手順をp004で実行。既存build jobを構造比較。

2026-10-02 / ws108-q547-ci-checkpoint: source 3a5b5b16 WIP。CI2distro matrix/TCG/make、missing inputs・hash・smoke package identity・clean source fail gates、distinct upload、release needs両job/merge download/4種類添付。PyYAML6.0.2構造・embedded bash -n PASS、既存build job deep equality PASS。missing package/corrupt metadataの拒否を確認。両committed source makeをTCGで実行中、remote Actions未実行。

2026-10-02 / ws108-q547-tcg-input: 3a5b5b16の両TCG native build/19ELF/clean source metadata生成済み。Debian fresh Vulkan/reinstall/upgrade PASS後、Terminal commandのfile確認が失敗しmake exit2。証拠はfailed-tcg-debian/へ保存。TCG software compositionはframe約1.4秒、1秒のpointer-focus待機＋短いkeypress列では入力完了を保証できなかったため、明示click/TCG待機/keypress間隔/実fileの最大30秒観測と失敗PNG/journalを追加。Debian root systemd serviceのHOME欠落によるdesktop Files ENOENTは通常HOME/WAYLAND_DISPLAYをfixtureへ明示し修正。production source/guest build.pyは不変、失敗をclear扱いせずnative候補で再現確認と最終両makeを続ける。

2026-10-02 / ws108-q547-tcg-final-retry: Ubuntu旧3a5b5b16試験も同じinput file未作成でmake exit2、target logをfailed-tcg-ubuntuへ保存。input/environment runner修正10362bd3 WIPでcodeを固定、両指定makeをTCGで新たに実行（build/test各8GiB）。前の失敗を成功へ書き換えない。native build.py/production Cは不変だが、CIのend-to-end exit0を最後に確認するため両native buildを再実行する。

## 結果 / q547-i01 / 2026-10-01T17:46:28.529984+00:00

uncleared。CI定義/構造/negative gates実装済み、2OS native TCG build/19ELF/clean metadata成功。旧両TCG input失敗を修正しfocused Debian runtime PASS。10362bd3最終Debianはupgrade用deb再圧縮を含む180秒blockでtimeout/exit2、CI検証要件未clear。Ubuntuの同時試験は終了待ち、p004開始不可。次attemptはupgrade圧縮とboundsを修正、2OS actual make＋affected TCG試験へ。

Event ws108-q547-uncleared: Phase結果/closure意図をlocal保存、remote comment/closeはdeferred。

ws108-q547-cessation: Ubuntu実行workerをSIGTERMで終了（Guest contextがown QEMUを停止）、native成果物/manifest/buildinfoを保存。Queueのuncleared結果は変更しない。新Queue前にold worker停止を確認。

## q547 uncleared 後の再設計 / ws108-q548-redesign

upgrade fixtureのみdpkg-deb -Znoneで正規debを再構築し、変更Version/conffileを実update。配布debの圧縮は不変。ldd/public client/reinstall/raw extract/upgradeのSSH blockを600秒へ明示し、全CI45分は維持。最終両makeを既定auto/KVMで実行しexit0を確認、その最終debをfresh TCG guest両方で全smoke再実行。TCG内のnative buildはq547で2OS成功しており、guest build.py/production Cの不変hashとnative TCG/KVMの全payload40file SHA/mode一致を確認する。これにより未知の圧縮性能を反復測定せず、変えたruntime/upgrade checksを実証。remote jobは未実行、加速器差と実行分割を明記。最大60分、scope/P4/CI default TCG/2OS/prerequisiteは不変。再開: oldworker停止、修正source固定、両make/TCG runtime/positive release gatesが成功。

ws108-q548-stage-race: Ubuntu KVM nativecompile成功後、並列install/install-sessionのmkdir失敗（cannot create directory）でexit2。staging親を先に作り両goalsを直列にするguest build.py修正。runtime内容/compiler/production sourceは不変、必要な2OS再実行を続ける。sourceのworkflow以外の重大判断無し、p004はfinal build.pyも全文確認する。

## 結果 / q548-i01 / 2026-10-01T18:13:59.373022+00:00

cleared。最終b0e1eaf9の両make exit0、両fresh TCG全smoke PASS、native TCG/KVM payload40全一致、19ELF/manifest/md5/control/root/session audit PASS。CI構造/negative/positive gates、既存build job維持。q547uncleared保持、remote Actions/publish未実行。plan/history/ws108/q548/result.md。

Event ws108-q548-cleared: Phase結果/closure意図をlocal保存、remote comment/closeはdeferred。
