<!-- awesome-plan project=zedbsd record=ws094-p014 -->

# ws094-p014: q588 の review の所有外の規約の指摘を直す

Status: planned（2026-10-02 ベータ1の計画。Queue なし）
Disposition: normal
Parent: [WS094](../ws.md)
Queue: なし
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
