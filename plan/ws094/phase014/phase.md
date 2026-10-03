<!-- awesome-plan project=zedbsd record=ws094-p014 -->

# ws094-p014: q588 の review の所有外の規約の指摘を直す

Status: cleared（2026-10-03 Q1 判定（user「Q1の判断で閉じられるものは閉じてください。」）: P2 の提案を受け入れ。規約の指摘 0、host・build・boot PASS）。元の記載: in-progress（q622-i01 は 2026-10-03 07:20 に終了。P2 は cleared を提案、判定は Q1）
Disposition: normal
Parent: [WS094](../ws.md)
Queue: q622 / q622-i01（自走の指示による Q1 の dispatch、時限 2h）
依存: [p007 の q588 の review](../phase007/q588-review.md)（指摘の表、76〜88 行）。compositor の file は WS099 p020・p021 の後（同じ directory、直列）
目安: 2h（1 Queue）。実行者の目安: phase-runner（compositor と Files の両方）
所有 path: `userland/desktop/wayland/display.c`・`menu.c`・`desktop.c`・`desktop.h` の WS094 の部分、`userland/desktop/files/thumb.c`・`ui-context.c`、`userland/desktop/imageview/image.c` の p013 の部分、`plan/ws094/`

## 背景

q588（p007 の部分の attempt）は B2 の所有の範囲だけを直し、compositor・Files の thumbnail と context・Image Viewer の指摘を「後の所有の attempt」に残した。
WS094 の完了（p007）にはこれらの解消が要る。2026-10-02 user: WS094 は現行設計のまま実装しきる。

## 範囲（q588-review.md の表の行）

- `display.c:243` の `zwl_desktop_is` を条件の中で呼ぶ、`menu.c:843` の `zwl_desktop_surface` を条件の中で呼ぶ（呼び出しと検査を分ける規則）。
- `desktop.c`: 797 行の 1 行に 3 つの節、`desktop_get`:539 の入れ子の呼び出し、`desktop_start` の spawn の結果の検査の順、`desktop_switched_off` の read の結果の検査の順、void の関数の出口、`desktop_word`:827 の return の注釈。
- `thumb.c`: 645 の複合の Boolean の return、685 の `thumb_gif_source` の型、722 の後片付けの後の結果の検査、759 の `thumb_adopt` の段落と明示の出口。
- `image.c`: 223 の `iv_image_orientation` の成功の return の段落、433 の `image_jpeg` の fclose の後の結果の検査。
- `ui-context.c:314` の `context_desktop` の最後の明示の成功の出口。
- compose.c の空行（358/373/403、既存の frame の log）は WS094 の範囲の外。WS099 p022 の全文規約へ回す（記録だけ）。
- 行番号は変わっているので中身で照らす。振る舞いは変えない（失敗の path の検査の順の変更は記録する）。

## 受け入れ

- 上の指摘が全て直るか、直さない理由（規則の解釈）が表にある。`style-check.py` の対象の file で新しい違反 0、既存の例外（`files/main.c` 2・`picture.c` の setjmp 1）だけ。
- build（`files`・`wayland`・`imageview`・`libkeiland.so`）の warning 0。
- host: `plan/ws094/tests/host-desktop.sh`・`host-thumb.sh`・`plan/tools/files/host-model.sh`・`plan/tools/imageview/run-host.sh` PASS。
- guest（QEMU の Venus）: `files-desktop-guest.sh install show input menu saved prune`、`criteria.sh … C9`（p076 の FAIL は BUG-125 の状態に従って区別）、boot test PASS。

## 検証の方法と範囲

コードの意味を変えない整形と検査の順の変更が中心。変えた領域の host と guest の手順を流す。QEMU の console・serial の log では判定しない。実機は p012。

## 未決の判断

なし。

## Event

2026-10-02 / ws094-beta1-plan-p014: fg019 の計画で新設。q588 の review の残りを所有つきの Phase に分けた。p007 の受け入れは不変。

## 結果（q622-i01、P2、2026-10-03 06:27〜）

worktree `/home/awe/zedBSD-worktrees/p2`（main `fdebeb7d3`）。Files の file は main の最新（P1 の q616 の後）の上で直した。意味を変えない規約の直しだけ。

| 指摘 | 直し |
| --- | --- |
| `display.c` `zwl_top_window` の `zwl_desktop_is` を条件の中で呼ぶ | 結果を `desktop_surface` に入れてから検査 |
| `menu.c` `context_create` の `zwl_desktop_surface` を条件の中で呼ぶ | 結果を `desktop` に入れてから検査（副作用の無い getter なので、短絡の順が変わっても結果は同じ） |
| `desktop.c` 797 行の 1 行に 3 つの節 | 節ごとに 1 行 |
| `desktop_get` の入れ子の呼び出し | `surface_id` に分けた |
| `desktop_start` の spawn の結果の検査の順 | spawn の直後に検査する。`starts++` は失敗と成功の両方の path に置き、数え方は同じ |
| `desktop_switched_off` の read の結果を close の後に検査 | read の直後に検査し、両方の path で close する（記録: close の位置だけが変わる） |
| void の関数の出口（`zwl_desktop_object_gone`・`tick`・`draw`・`unfocus`・`desktop_place`・`desktop_start`・`desktop_watch`・`desktop_new_token`） | 成功の注釈つきの明示の `return;` |
| `desktop_word` の return の注釈 | 結果を言う注釈にした |
| （同じ file）`desktop_switched_off(void)`・`desktop_new_token(void)` の定義 | 引数を別の行に |
| `thumb.c` `thumb_signed` の複合の Boolean の return | if で 0 と 1 を返す |
| `thumb_gif_source` の型の位置 | file の型の節（forward declaration の前）へ移し、役割の注釈を付けた |
| `thumb_gif` の後片付けの後の結果の検査 | 検査を直後にし、両方の path で `DGifCloseFile` |
| `thumb_adopt` の段落と明示の出口 | 段落の注釈と成功の `return;` |
| （同じ file、P1 の q616 の新しい違反）`fm_thumb` の結果の `if` の前の空行 | 空行と注釈（`style-check.py` の blank-after-brace） |
| `image.c` `iv_image_orientation` の成功の return の段落 | 段落を分け、成功の注釈 |
| `image_jpeg` の fclose の後の結果の検査 | 検査を直後にし、両方の path で fclose。拒否の理由の 3 分岐は `image_jpeg_refuse` に分けた（文言・errno は同じ） |
| `ui-context.c` `context_desktop` の最後の明示の成功の出口 | 成功の注釈つきの `return;` |
| compose.c の空行（358/373/403） | 範囲の外（WS099 p022 へ。記録だけ） |

### 確かめ

- `style-check.py`（6 file）: 違反 0。
- build: zedBSD の `wayland`・`files`・`imageview`・`libkeiland.so` は warning 0。Linux の keiland（`make -f userland/desktop/keiland-linux.mk all`）も exit 0・warning 0。
- host: `host-desktop.sh`・`host-thumb.sh`・`plan/tools/files/host-model.sh`・`plan/tools/imageview/run-host.sh` は全て PASS（[evidence](evidence/)）。
- guest（Venus、BIN=直した build）: `files-desktop-guest.sh install show input menu saved prune` は install・show・input・menu が通り、saved・prune が FAIL（[log](evidence/files-guest-sequence.log)）。`install saved prune` だけを流すと PASS（[log](evidence/files-guest-saved-prune.log)）。p014 の前の binary（`build/p2-p008-img`）で同じ順に流しても、同じ saved・prune が FAIL する（[log](evidence/files-guest-sequence-pre-p014.log)）。menu の step が notes.txt の名前を変え、項目を動かした後で、saved・prune が元の配置を期待するためと見ている。試験の順の問題で、今回の変更によるものではない。
- `criteria.sh … C9`（直した image）: 9/10。p052 だけが FAIL（[results](evidence/c9-results.txt)）。赤い窓が cascade の位置（472,282）に描かれ、緑の窓を覆っていた（[PNG](evidence/c9-p052-windows.png)）。client a（赤）の起動が遅れて、b（緑）より後に map された形。p052 だけを新しい guest で 3 回流し直すと 3/3 PASS（[results](evidence/p052-rerun-results.txt)）。p052 は p020 以降の 10 回でも PASS している。今回の変更（`zwl_top_window` の desktop の判定、context menu、desktop.c）は plain の look の窓の配置に関わらない。guest の stall（BUG-135 の系統）による間欠と見ている。p076 は PASS。
- boot test: PASS（[PNG](evidence/boot-login.png)）。

### 判定の提案

q588 の表の指摘は全て直した（compose.c の空行は範囲の外、記録だけ）。build・host・style は受け入れを満たす。guest の saved・prune の FAIL は、試験の step の順による既存の失敗（p014 の前の binary でも同じ）。C9 の p052 の 1 回は、流し直して 3/3 PASS の間欠。**cleared を提案する。**
