<!-- awesome-plan project=zedbsd record=ws124 -->

# WS124: GNU Emacs の package

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG002
Objectives: O2
Parent: [Master](../master.md)
Focused goal: fg019（ベータ1）
Queue: none
Resume point: p001（取得・検証・license 監査と cross build の方針）が planned。すぐ Queue にできる。p004 は ws125-p002（staged tree の image への導入）の成果を待つ。GUI 版の要否はユーザーの判断（下の D1、既定案は端末版だけ）。
<!-- awesome-plan-current:end -->

## 目標（2026-10-02 ユーザー（ベータ1、リリース目標 10/17））

「userland/packages/emacs (GNU Emacs)を追加」

- 外部 package として GNU Emacs を取得・検証・patch して build/install する（tarball、license 監査。GPL は package の境界の中、[設計方針 §2.1](../master-design-policy.md)）。置き場所は `userland/packages/editors/emacs`（2026-10-02 user 承認）。
- 版の提案: **31.1**（[WS034 の台帳](../ws034/package-inventory.md) §1.2 で取得・署名検証済みの最新安定版）。実装 Phase で 31.x の新しい bugfix 版の有無を確かめ、あればそれを取得して実測し直す。

## ベータ1 の到達目標と受け入れ条件（案）

端末版（`emacs -nw`）を zedBSD amd64 の image に入れ、guest で編集できる。GUI（pgtk/X11）はベータ1 の範囲外（D1）。

| # | 基準 | 確かめ方 |
| --- | --- | --- |
| E1 | guest で `emacs -nw`（TERM=xterm）が起動し、`*scratch*` をエラー無しで出す。起動の時間を記録する（目安 QEMU で 3 秒以内、p005 で短縮） | SSH（`plan/tools/guest/guest.sh`）の pty と、Keiland の terminal の画面（QMP の screendump ではなく boot-test と同じ撮影） |
| E2 | file を開き、文字を足し、`C-x C-s` で保存し、`C-x C-c` で終了する。保存した内容を byte で照合し、終了後に端末の状態（stty）が戻る | SSH の自動試験（`plan/ws124/tests/`） |
| E3 | UTF-8 の日本語を含む file を表示・編集・保存し、編集していない部分が byte で変わらない | 同上 |
| E4 | `emacs --batch --eval '(princ emacs-version)'` が `31.1` を出す。`c-mode`・`dired`・`M-x shell`（pty 経由の `/bin/sh`）が動く | 同上 |
| E5 | `/usr/share/licenses/emacs/COPYING`、provenance（版・size・SHA-256・署名・patch 一覧）、license の機械監査の結果がある | 文書と image の中身 |
| E6 | Emacs を選んだ image でも `plan/tools/boot-test.sh` の login prompt が出る | boot-test の PNG |

QEMU の証拠と実機の証拠を分けて書く。実機（Latitude 5330）は未実施なら未実施と書く。

## 依存（既存と不足）

| 依存 | 状態 | 扱い |
| --- | --- | --- |
| termcap API（`tgetent`・`tputs` 等） | base の curses にある（ws034-p041 cleared、`/lib/libcurses.a` は PIC） | `--with-terminfo`。base の terminfo（`xterm.zti` 等、独自形式）を `tgetent` が読む |
| pty（`posix_openpt`・`grantpt`・`openpty`）、termios、`select`、`sigaltstack`、`dlopen` | libc にある | 実行時の確認は E4 |
| zlib | `userland/packages/libs/zlib`（1.3.2）がある。base の `libz-compat` は inflate だけ | 任意（圧縮 file を開く）。使うなら package の zlib |
| GnuTLS、libxml2、gmp、tree-sitter、native-comp（libgccjit）、modules | 無い | `--without-gnutls --without-xml2 --without-libgmp`（同梱 mini-gmp）`--without-tree-sitter --without-native-compilation --without-modules` |
| `getloadavg`、`timerfd` | libc に無い | Emacs は無くても動く（configure の代替）。要れば libc の不足として WS034 へ |
| host の Emacs 31.1 | 無い | p002 で同じ tarball から host 用を作る（lisp の byte-compile と DOC の生成） |
| config.sub の zedbsd | 未対応（[台帳](../ws034/package-inventory.md) §3.1） | OpenSSH と同じ package ごとの patch（共通化 ws034-p025 は待たない） |
| 多数の file の image への導入 | 今の `ZEDBSD_PACKAGE_FILES`（file ごとの `--file`）は 1 つの shell の引数に入り、lisp の数千の file では上限（128 KiB）を超える | [ws125-p002](../ws125/phase002/phase.md) の staged tree の仕組みを使う |

## 危険

- Emacs は一般の cross build を想定しない（Android だけ）。build の途中で `temacs` を走らせて dump と byte-compile をするので、host 用の Emacs と target の `temacs` の分担を p001 で決める。既定案: lisp の `.elc`・DOC・charsets は host の Emacs で作り（`.elc` は機種に依らない）、target の binary は `--with-dumping=none`（毎回 loadup）か、guest で一度 `--temacs=pdump` を走らせて `emacs.pdmp` を作る（p005）。
- image の容量（root 1024 MiB、inode 65536）。`.el` を入れず `.elc` だけにするなど、p004 で実測して決める。
- WS034 の旧 Phase ws034-p010（emacs 端末版、planning）とこの WS が重なる。所有の移管（p010 をこの WS へ移し canceled にする）は main に依頼する。

## ユーザーの判断が要る点

- **D1（GUI 版の要否）**: 既定案はベータ1 は端末版だけ。GUI（pgtk）は GTK の移植（WS115）の後の別 WS。
- **D2（image の既定）**: ベータ1 の release image に既定で入れるか（menuconfig の既定 y/n）。容量と WS129 の release 内容に関わる。既定案は menuconfig で選べる（既定 n）にし、release image に入れるかは WS129 で決める。

## Phase

| Phase | 目的 | Status | 依存 | 目安 |
| --- | --- | --- | --- | --- |
| [ws124-p001](phase001/phase.md) | 取得・検証・license 監査、zedbsd 向けの configure の試行、cross build の方針（dump の方式、host の道具の分担） | planned | — | 2〜3h |
| [ws124-p002](phase002/phase.md) | host 用 Emacs 31.1 の build（lisp と DOC の道具）と、package の Makefile の骨組み（取得・展開・patch） | planned | p001 | 2〜3h |
| [ws124-p003](phase003/phase.md) | target の cross build（lib-src、`emacs` の binary）、ELF の確認 | planning（p001 の方式の確定を待つ） | p002 | 3〜4h |
| [ws124-p004](phase004/phase.md) | lisp・etc・DOC の stage、menuconfig への登録、image への導入、guest の受け入れ E1〜E6 | planning | p003、ws125-p002 | 3〜4h |
| [ws124-p005](phase005/phase.md) | pdump による起動の短縮（p004 で `--with-dumping=none` にした場合） | planning（p004 の起動の時間を見て要否を決める） | p004 | 2〜3h |
| [ws124-p006](phase006/phase.md) | 全文規約（Makefile・patch・試験）と回帰、制限の整理（必須の最終確認） | planning | p004（p005 を行うならそれも） | 2h |

Graph: p001 → p002 → p003 → p004 → (p005) → p006。ws125-p002 → p004。

## Event history

2026-10-02 / ws124-plan-detail-20261002: 計画担当が WS034 の台帳・既存 package（OpenSSH・expat・remacs）・libc を調べて受け入れ条件と Phase を詳細化。p001・p002 planned、Queue none。実装・取得はしていない。
