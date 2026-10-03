<!-- awesome-plan project=zedbsd record=ws131-p025 -->

# ws131-p025: browser の shell の窓を新しい API へ（D7）

Status: planning（2026-10-03 user「D7,browserのshellもlibkeilandで書きましょう。」で新設）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p020 cleared（app の移行で `kl_app` の API が揃っている）。実行の順は p020 の後・p023 の前。**WS074（browser）は Codex が作業中なので、この Phase の開始の前に Q1 がユーザーに衝突を確かめる**
目安: 4〜5h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/browser/`（`shell/`・`main.c` の窓の結線、`Makefile`）、API の不足の補い（`userland/desktop/libkeiland/app/`・`ui/`）、`plan/ws131/`。**libbrowser（`userland/desktop/libbrowser/`）は触らない**。Q1 の委任が要る: WS074 の browser の試験（`plan/ws074/tests/browser-*.sh`）の手順の調整

## 目的と結果

browser の shell（`userland/desktop/browser/shell/`、4,232 行。そのうち自前の窓 `window.c` 1,176 行・Vulkan の present `present.c` 648 行・titlebar の結線 `titlebar.c` 349 行・touch の結線 `touch.c` 601 行・key `keys.c` 227 行）の自前の窓を `kl_app`・`kl_window` と宣言的な titlebar（URL の field）へ移す。libbrowser の描画は今のとおり shell の Vulkan の present で画面に出し、窓は `kl_window_vulkan_surface`（見せ方 NONE）で作る。Guardrail の「browser / libbrowser」（libbrowser は Wayland を使わない、shell が Wayland の event を public input interface に変換する、[browser component](../../standards/browser-component.md)）は変えない。

## 範囲

1. 最初の 30 分で WS074 の作業の状態（Q1 の確認の結果）と、shell の `window.c`・`present.c` が Wayland のどの object を持つか（xdg toplevel・seat・touch・keyboard・titlebar・gesture・scroller）を表にして phase.md に書く。
2. 窓・入力（pointer・key・touch・repeat）・main loop を `kl_app` へ。shell の input の変換（Wayland の event → libbrowser の public input interface）は `kl_app_take` の event からの変換に置き換える。
3. titlebar（URL の field・戻る・進む・再読み込み）を宣言的な control に。System Menu は今は無い（study §1.3）ので足さない。
4. touch の scroll・gesture は今の呼び方（scroller・gesture）を保ち、新名に。
5. 新名と `<keiland.h>`。browser は zedBSD だけの package（Linux・FreeBSD の Makefile は無い）。

## 受け入れ

- zedBSD の amd64 の build（exit 0・自前の warning 0）と `plan/tools/keiland-os-boundary/check.sh` PASS。Linux・FreeBSD の `make keiland-linux`・FreeBSD の native build が壊れていない（browser は入らないが libkeiland の変更の確認）。
- browser の shell に自前の `xdg_wm_base`・`wl_registry`・swapchain の code が無い（grep）。libbrowser の diff が 0。libbrowser が Wayland の header を include しない（browser component の規則の check）。
- 回帰: `plan/ws074/tests/browser-guest.sh` と WS074 の代表の試験（Q1 と決める）、`plan/ws081/tests/run-browsertouch.sh`・`run-browser-scroll.sh`、boot-test。QEMU の console・serial の log で判定しない。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)。build は自分の `BUILD=build/ws131-p025/`、共有の `build/` を消さない。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: WS074（Codex が作業中、`userland/desktop/browser/` と libbrowser）。開始の前に Q1 がユーザーに確かめる。
- 危険: shell の input の変換の順（key の repeat、IME の無い field の入力、touch の時刻）の退行。browser の試験を前後で流す。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

p020 の cleared と main への統合、WS074 との衝突の確認の後に、Q1 が Queue を作る。
