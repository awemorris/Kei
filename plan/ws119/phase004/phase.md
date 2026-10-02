<!-- awesome-plan project=zedbsd record=ws119-p004 -->
# ws119-p004: live の image への組込みと QEMU の通し

Status: planning（p002・p003 を待つ）
Disposition: normal
Parent: [WS119](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 2〜3h

## 範囲

インストーラを image に入れ（user program の登録、App Home の tile または p001 で決めた入口）、USB の image で起動 → インストーラ → NVMe へ導入 → USB を外して NVMe から起動 → 作った利用者で
graphical login、を QEMU で通す（受け入れ I1〜I3）。release の config（WS129）に入れる差分は main に依頼する。

## 受け入れ

I1〜I3 の PNG（QMP の screendump）と log、`plan/tools/boot-test.sh` の PNG をユーザーに見せる。QEMU の証拠と書く。

## 所有 path

インストーラの directory、`plan/ws119/`。image の config・App Home の設定の共有の file は main に依頼する。

## 依存

p002、p003。

## 未決の判断

なし。
