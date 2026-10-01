<!-- awesome-plan project=zedbsd record=ws108 -->

# WS108: Debian13・Ubuntu26.04 の native QEMU deb / CI release

Status: completed
Primary Milestone: MG007
Related Milestones: MG001, MG006
Parent: [Master](../master.md)
Queue: q549 finished
Resume point: P1〜P5 verified（2026-10-02 JST）。GitHub publication/close/Project pending。

## 目標と結果

Debian13は `make keiland-linux-debian`、Ubuntu26.04は `make keiland-linux-ubuntu2604`。
それぞれのpinned amd64 OSをQEMUで起動してnative build/dpkgを実行し、別fresh guestへ導入して動作検証する。
既存CIに2 target matrixを追加し、両方の検証をrelease作成の条件にした。deb/checksum/manifest/Keiland JSON buildinfoをnightly release filesへ添付する定義。

最終source b0e1eaf972e1で両make exit0（KVM）、同じ最終debのfresh TCG両OS全smokeもPASS。
TCG内のnative buildも2OSで実施し、最終KVMの40payloadファイルのhash/modeと全一致。
[全文規約とWS acceptance](../history/ws108/conformance.md)でP1〜P5を照合。19ELF、root ownership/md5/conffile/session/依存/ライセンス、Vulkan/実GUI/keyboard command/reinstall/genuine upgrade/remove/purge/user dataを確認。
production C/元Linux build規則/zedBSD image/toolchainは変更無し。runtime test app5/development headers/model dataをprivate stagingで除外。

## 制限・残務・同期

remote GitHub Actions/upload/release/Issue/Projectは未実施。最終full TCG make単独invocationは重ねず、actual native TCG buildと最終TCG runtimeを分割検証、全40payload一致を証拠とする。
CIホストのaction実行結果・実機GPU/WiFi/audio・新display-manager session起動は主張しない。QEMU direct GUIとsession登録を検証。
TCG画面は入力処理から遅れて表示することがあるため、端末コマンドの実file効果をSSHでも検証。
compiler warning0、dpkg-shlibdepsのprivate unversioned SONAME警告27件ずつは分類済み（抑制無し）。buildinfoはDebian標準形式でなくKeiland JSON、bit reproducibilityは主張しない。
GitHub publication/closure/outbox pending。commit WIP / pushなし。MG007全体のacceptanceは未充足、Supporting MG001/MG006へ検証/配布出力を提供。
WS106 ime-probe所有回答待ちと既存resize bugsは別に保持。後続WS/Queueを自動開始しない。

## Phase / Queue

| ID / Phase | Goal | Status / attempts | Dependencies / evidence |
| --- | --- | --- | --- |
| [ws108p001](../history/ws108/q545/phase.md) | P1 pins/manifest/design/native guest readiness | cleared / q545 | WS105 output/WS106確定配置（context）、301a1cc6 |
| [ws108p002](../history/ws108/q546/phase.md) | P2/P3 native packaging/install/runtime | cleared / q546 | p001、ac04182a、[evidence](../history/ws108/q546/result.md) |
| [ws108p003](../history/ws108/q548/phase.md) | P4 CI/release gates | cleared / q548; [q547 uncleared](../history/queue-q547.md)維持 | p002、[retry evidence](../history/ws108/q548/result.md)、cc7ab9ac |
| [ws108p004](../history/ws108/q549/phase.md) | P1〜P5 final full conformance | cleared / q549 | p003、WS106確定配置（scoped output、ime-probe対象外）、[conformance](../history/ws108/conformance.md) |

[設計](design.md)、[inputs](inputs.json)、[実行・判断履歴](../history/ws108/ws-at-acceptance.md)、[再利用driver](../../tools/release/keiland-linux-deb/README.md)。
受け入れ時snapshot内の元Phaseリンクは、上のPhase表にarchive先を保持。
Phase directoriesはarchive確認後に削除。失敗を後の成功に書き換えず、各Queue履歴/ログへ保存。

## 2026-10-02 / ws108-user-targets

ユーザーの指定make targets、両OS QEMU native build/導入・動作確認、既存CI/release filesへの組込み指示。
「現在のQueueを完了したら、WS108を実行してください。」を有限1Phase Queueの実行承認として保存。2026-10-01レビュー第3項を具体化し、旧container案とrelease除外を置換。push禁止を保持。

Event ws108-completed-20261002: 全Phase結果に加えWS自身のP1〜P5を検証してcompleted。remote publication/closureは別のpending状態として保持。
