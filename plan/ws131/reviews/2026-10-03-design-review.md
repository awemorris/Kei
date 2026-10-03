# WS131 移行計画の design-reviewer の review（2026-10-03、照合した版 e3fda7d7b）

Q1 が design-reviewer に依頼した敵対的な review の結果の要約。P3 の q629 の改訂の入力。全文は Q1 の session の記録。

## 先に直すべき（重大・中）

1. **p006 と p007 の間の退行（重大）**: p006 で compositor の毎秒の監視を除くと、Settings は p007 まで desktop.conf を直接書くので、変更が compositor の再起動まで効かない。書き手が二つになる。`plan/ws089/tests/settings-p007.sh`（手で書いた desktop.conf を数秒で適用）も FAIL。→ 監視の除去を p007 に移すか、p006 と p007 の look を同じ merge の単位に。settings-p007.sh の書き換えを範囲に。
2. **Files の libkeiui の利用の取りこぼし**: `files/ui-scrollbar.c:22` が `keiui.h`、Files の 3 本の Makefile が `libkeiui/scroll-bar.c` を compile。config の package 名 `libkeiui`（config/ci/config-amd64.mk:37 ほか 4 か所）、FreeBSD の header の install の表（keiland-freebsd.mk:184-187）に keiland-ui.h・keiland-motion.h が無い。→ p008 の範囲と受け入れに。
3. **Guardrail の改訂案が app を縛りすぎる**: Terminal の pty（`terminal/main.c:42`・`:825` の ioctl TIOCSWINSZ）、xserver の keymap（`<uapi/input.h>`）、Files の xattr（freebsd-compat）が C1・C2 で FAIL。→ 対象を desktop の OS の抽象化（電源・network・音声・PnP・設定・seat・GPU）に限るか、例外として明記（**ユーザーの判断**）。
4. **D12 の前提が source と合わない**: session の control の socket で sessiond が受けるのは UNLOCK と LOGOUT だけ（`sessiond/session.c:309-324`）、POWER は greeter の socket だけ（`greeter.c:517`）。lock 中に POWER を送ると UNLOCK の答えと取り違える（`handoff-zedbsd.c:294-298`）。→ zedBSD は unsupported、sessiond の拡張は別の WS、など。
5. **session の interface に lock 画面（UNLOCK）が無い**、答えの多重化の規則も無い。→ `session_unlock()`、答えに要求の識別、待ちは一つの規則、p004 の回帰に lock/unlock（ws035-p102 系）。
6. **p007 で旧 API を除くと壊れる道具**: `plan/tools/keiland-linux/network-probe.c`・`audio-probe.c`・`lib-smoke.c`、`plan/ws089/tests/host-network.c`・`host-slot.c`。p007 の受け入れ自身が network-probe を使う。→ 向け直しを p007 の範囲に。
7. **compositor の event loop で disk と ioctl を待つ**（BUG-125 と同じ種類）: `wayland/network.c:1164`・`:1283` の同期の読み書き、新しい protocol の query_details・save_key・get_saved。→ worker の thread へ。
8. **network の要求の多重化が未定**: backend は一つの要求の pull 型（`keiland.h:689-698`）、PROFILES の段（`keiland.h:627`）が protocol に無い。→ 一度に一つ・他は busy、PROFILES は save_key の中で compositor が行う。
9. **認可（D5）**: uid の照合は実質制限にならない（socket は 0700 の XDG_RUNTIME_DIR）。registry の時点で隠す（`protocol.c` の `zwl_ime_global_visible` の仕組み）。WS113 p005 の「active session の同じ UID」と揃える。zedBSD の peer の uid は既にある（`include/uapi/socket.h:84`、`src/libc/openbsd.c:261` の getpeereid、networkd の `main.c:152`）。
10. **書き込みの確実さと WS113 との約束**: 250 ms でまとめる間の終了・log out・電源断の flush、結果を applied と saved に分ける WS113 p005 と揃える（D13）。
11. **D10 の例外**: 機械的な改名と path の修正を「変えない移動」とみなすと明示しないと例外が効かない。
12. **subagent の所有 path**: `plan/tools/keiland-os-boundary/`・`gpu-boundary/`、他の WS の試験、`config/ci` は main の明示の委任が要る。
13. **Guardrail の適用の時期**: 配置の規則は p005、protocol と書き手の規則は p007 の後に段階的に。
14. **rollback**: 他の WS の後続の変更があると revert できない。forward fix を基本に。
15. **見積もり**: 約 50h は楽観的、80〜120h の幅。p008 を「移動と改名」と「互換と回帰」に割る。
16. **backend の callback の約束**: logind の PauseDeviceComplete の応答の順、ResumeDevice の新しい fd、callback の中からの再入。

## 軽

17. protocol の名前（`keiland_network_v1` ほか）が旧 API と被り、interface の表が glob で外に出る → `keiland_system_*_v1` に、表は static。
18. B4 が改名表と矛盾（`kui_color_mix` は外に出ている）。
19. p015 で keiui.h を消すと壊れる host 試験（ws090・tools/keiui・ws127 の scroll-bar-test、§2.7 の漏れ）。
20. 公開の header に Vulkan の型（`keiland_window_vulkan_surface`）→ 全利用者が Vulkan の header に依存。
21. zedBSD の置き場は Makefile の名前にしてはいけない（top の `Makefile:262-266` が `userland/*/*/Makefile` を自動 include）。
22. その他: p006 の probe の試し方、Keiland 以外の compositor での Settings、WS132 の範囲 2、ws.md の題、phase の実行者の書き方。

## 事実の照合

約 40 か所が正しい。誤り: `KEILAND_VERSION` は `keiland.h:49`、chooser の版の根拠は `:48`・`:1069`、`KEILAND_LINUX_PROGRAM` は `userland/desktop/keiland-linux.mk:57`、libkeiland.so は既に libtruetype に依存、「shader 3」は shader 2 と regenerate.py、§2.6 の Files の分類、§10 の peer uid、D12 の確かめ、rename-map の `KUI_VERSION` の行番号。

## この review の後のユーザーの決定で変わった点

- D2 は G2 をやめ GPU の buffer の protocol も backend へ（指摘の「D2 の明示」は解消）。
- ABI は変えてよく版も上げない（指摘 10・15 の版の部分、17 の glob の外への漏れの重みが下がる）。
- 名前は `KL_`・`kl_`・`KWL_`・`kwl_`（rename-map の作り直し）。
