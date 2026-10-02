<!-- awesome-plan project=zedbsd record=ws127-p004 -->

# ws127-p004: thumbnail の拡張（F-035）

Status: planning（2026-10-02 ベータ1の計画。要る判断・成果は「未決の判断」と「依存」）
Disposition: normal
Parent: [WS127](../ws.md)
Queue: なし
依存: p001 でユーザーが採用。WS079 の libpdf（読むだけ）
目安: 3h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `files/thumb.c`・`peek.c`・Makefile の link、`picture/` を変えるなら WS128 と直列、`plan/ws127/tests/`

## 範囲

(1) PDF の 1 頁目の thumbnail（libpdf の `pdf_page_render` を使う）。(2) disk の thumbnail の cache（freedesktop.org の `~/.cache/thumbnails/normal`、mtime と URI の MD5）。(3) 動画は WS122（動画プレーヤ）の decoder の後で、今回は外。

## 受け入れ

100 項目（PDF 10 を含む）の folder で PDF に thumbnail が出る（画面）、2 回目の表示で cache から読む（log）、壊れた PDF で落ちない（host 試験）。
変えた source の style-check の違反 0、build の warning 0、`files-regress.sh` の 14 本と boot test PASS。

## 未決の判断

採否（p001）。cache の上限と消し方

## 検証の方法と範囲

QEMU の Venus（`plan/tools/files/files-guest.sh`、`files-regress.sh`）と host の試験。console・serial の log では判定しない。各 Phase の最後に `files-regress.sh` の 14 本と boot test。
Settings と共有の `canvas.c`・`text.c`・`icons.c` を変えたら `sh plan/ws089/tests/host-build.sh` と Settings の host 試験も流す。

## Event

2026-10-02 / ws127-beta1-plan: fg019 の計画で新設（planning）。p001 の結果とユーザーの選択で範囲を確定し planned にする。
