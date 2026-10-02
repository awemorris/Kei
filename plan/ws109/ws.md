<!-- awesome-plan project=zedbsd record=ws109 -->

# WS109: Linux版 Keiland を FreeBSD 15 へ移植

Status: completed
Primary Milestone: MG006
Related Milestones: MG007
Parent: [Master](../master.md)
Queue: なし（q572 finished）
Resume point: 完了。別version/GPU/実radio・driver修理は新しい有限scopeを選定する。
Completed: 2026-10-02 JST /current user指示のF1〜F5を検証。

## 目標と承認

Linuxの共通描画を再利用し、FreeBSD15のnative compositor/主要app、audio/network/WiFi
backendを用意する。[レビュー](../reviews/2026-10-01-review.md)とユーザーのWS108後の
WS109実行・drm-kmod/FreeBSD SSH/QMP例外承認に従った。Linuxulatorやrendererの複製は無い。

その後、ユーザーは実機/WiFi実機を免除しQEMU Venusを選択。native Venusの前提が欠けたため、
最新指示「awe@10.0.10.25 でi915をPCIパススルーして、FreeBSDゲストを実行してみましょう。」
により、指定hostの既存vfio-pci/IrisXeを使う専用guestで確認した。
[承認の全文と境界](../standards/ws109-native.md)、[設計](design.md)。Venus成功とは報告しない。

## WS自身の受け入れ

| 条件 | 検証した成果 / 証拠 |
| --- | --- |
| F1 環境・native ABI・license | FreeBSD15.1-p4/amd64/baseClang19.1.7、QEMU10.0.11/Q35/KVM4CPU4GiB、IntelIrisXe8086:46a8、drm66/Mesa26.1.3/seatd。Keilandの寛容licenseと許可されたsystem drm-kmodを分けて確認。[環境](../history/ws109/q551/environment.md)、[実GPU](../history/ws109/q568/result.md) |
| F2 native build/install/公開ABI/後段Vulkan | 独立gmake/DESTDIR/opt、native libc/header、private11libraries/SONAME/25ELF、標準公開headerと後段system Vulkan chain。warning0。[q565](../history/ws109/q565/result.md)、[最終](../history/ws109/q572/native-final-build-audit.txt) |
| F3 共通描画/入力/同期/session | 実IntelGPUのfill/copy/readback、非特権compositor/shm/正しいxdgVulkanwindow、liveDMA/closedfd/output/borrowedownership、VT1→2→1のlease停止/再取得、実USB入力、93frames/error0/cleanup0。[q569](../history/ws109/q569/result.md)、[q570](../history/ws109/q570/result.md) |
| F4 native audio/network/WiFi backend | 実OSS列挙/stereo音量/mute/外部変更/拒否/再接続、有線metadata/carrier down→up/権限/native net80211 ABI。WPA scan/profile/credential/join/disconnect/timeoutは独立peerのwire検証。実radioはユーザー免除。[q571](../history/ws109/q571/result.md) |
| F5 主要app/全文規約/3OS/文書 | Terminal実keyboard→PTY→shell、Files列挙/thumbnail、Settings実音量/有線、Notes/PDFViewer実PDF、TextEdit実text、ImageView実PNG、AppHomeの認証desktop表示。全文C最終修正/レビュー、nativeheaderELF、Linux/zedBSD finalbuild、最後のboot-test.sh loginPNGと運用文書。[最終結果](../history/ws109/q572/result.md)、[source hashes](../history/ws109/q572/source-inventory.md) |

## Phase一覧（記録は履歴へ保存）

| ID | 目的 | 最終Status / Queue | 証拠 |
| --- | --- | --- | --- |
| ws109p001 | 環境・ABI・依存の確定 | cleared / q551 | [Phase](../history/ws109/q551/phase.md)、[環境](../history/ws109/q551/environment.md) |
| ws109p002 | native build/library/app基盤 | cleared / q565 | [Phase](../history/ws109/q565/phase.md)、[結果](../history/ws109/q565/result.md) |
| ws109p003 | 共有描画/native seat/input/sync | cleared / q570 | [Phase](../history/ws109/q570/phase.md)、[結果](../history/ws109/q570/result.md) |
| ws109p004 | audio/network/WiFi backend | cleared / q571 | [Phase](../history/ws109/q571/phase.md)、[結果](../history/ws109/q571/result.md) |
| ws109p005 | 全文規約/主要app/回帰/文書 | cleared / q572 | [Phase](../history/ws109/q572/phase.md)、[結果](../history/ws109/q572/result.md) |

依存: p001のverified出力→p002→p003/p004→p005。途中のpartial/unclearedと判断を履歴に保持。
近final conformance q566を最終q572で補完し、残ったpublic function comment形式も修正した。

## 制限・移管・終了時の状態

- [BUG-130](../bugs/BUG-130.md)はreproduced/tracking。Keilandのnative能力分類と既存CPU完了fallbackを実検証したが、drm-kmodのzeroaccess DMA-BUF自体は未修理。upgrade時に再検証する。
- Venus未実行、FreeBSD15.1 amd64/指定IntelGPU以外は未実施。実機radio/physical audio・PCM・system WPA保存/全GUI編集の網羅を主張しない。
- [運用手順](../../userland/desktop/README.freebsd.md)、[再利用native probes](../tools/keiland-freebsd/README.md)。PhaseとWS固有testsはAGENTS.mdに従い削除、git/historyに保存した。
- 専用FreeBSD VMを正常停止、hostGPUは元のvfio-pciのまま、専用diskを保存。[終了readback](../history/ws109/q572/host-restored.json)。hostのdriver/kernel/networkや他のVMを変更していない。
- commitはWIP、push無し。GitHub publication/Issue close/Projectは保留でoutboxに保存。local完了とremote同期を区別する。
- fg016達成。Primary MG006/Related MG007への寄与を確認したが、milestone全体や別WSの完了にはしない。

## 完了イベント

2026-10-02 / ws109-completed-20261002: F1〜F5の独立acceptanceを照合しcompleted。
[全履歴](../history/index.md)、[完了直前のWS記録](../history/ws109/q572/ws-before-closure.md)。
p001〜p005のclearance/close intentとWScompletion eventはlocal保存済み、remote公開/closeは未実施。
