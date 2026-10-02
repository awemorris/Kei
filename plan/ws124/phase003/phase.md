<!-- awesome-plan project=zedbsd record=ws124-p003 -->

# ws124-p003: target の cross build

Parent: [WS124](../ws.md)
Status: planning
Disposition: normal
Queue / attempts: none
Goal: zedBSD amd64 向けに lib-src と `emacs` の binary を cross build し、ELF の契約を確かめる。
Prerequisites: p002 cleared。p001 で dump の方式が決まっていること（planning の理由）。
Investigation bound: 4 時間。

## 範囲

- `ZEDBSD_EXTERNAL_CROSS_ENV` の wrapper と autoconf cache（p001 の一覧）で configure。lib-src の host 側の道具は p002 の host build のものを使う（Makefile の変数か patch）。
- `src/emacs`（または `temacs`）を作り、`tools/build/check-dynamic-elf.py` で application の ELF・`DT_NEEDED`（libc.so ほか）・symbol versioning の無いことを確かめる。
- 見つかった libc・kernel の不足は回避せず記録し、WS034（libc の受け皿）か Bug へ main 経由で送る。

## 受け入れ

- target の binary と lib-src の道具が staged directory にでき、ELF の検査が通る。patch は目的と理由を results に書く。

## 検証

build の warning の確認、ELF の検査。guest での実行は p004（試しに起動するのはよい）。

## 所有 path

`userland/packages/editors/emacs/`、`plan/ws124/`、自分の worktree の `build/`。

## 依存・未決の判断

p002。libc の不足が出たらその修正は別 WS の Phase（依存として uncleared にする）。
