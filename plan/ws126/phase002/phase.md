<!-- awesome-plan project=zedbsd record=ws126-p002 -->

# ws126-p002: build 用 Python と target の interpreter（T1）

Parent: [WS126](../ws.md)
Status: planning
Disposition: normal
Queue / attempts: none
Goal: 同じ版の build 用 Python を host に作り、target の `python3` と T1 の module を cross build して stage する。
Prerequisites: p001 cleared（方式の確定、planning の理由）。
Investigation bound: 4 時間。

## 範囲

- `userland/packages/lang/python3/Makefile`（`ZEDBSD_EXT_python3_*`、`ZEDBSD_EXTERNAL_SOURCE`）、`patches/`。
- build 用の Python: 同じ distfile を `build/packages/python3/host` に展開して host の cc で作る（host の `/usr` に入れない）。
- target: `ZEDBSD_EXTERNAL_CROSS_ENV` と cache で configure（`--host`、`--build`、`--with-build-python`、`--prefix=/usr`、`--without-ensurepip`（D2 は p005）、zlib は `libs/zlib` の stage）。`make` と `DESTDIR` の install で stage。`check-dynamic-elf.py` で `python3`・`libpython`・拡張 module の ELF を確かめる。
- 依存する package の stage は OpenSSH と同じく path で名指す（変数の読み込み順に依らない）。
- libc の不足は回避せず記録し、main 経由で WS034 か Bug へ。

## 受け入れ

- stage に `/usr/bin/python3`、`/usr/lib/python3.<x>/`（T1 の module を含む）。ELF の検査 PASS。guest で `python3 -c 'print(1)'` を試し、結果を記録（受け入れの本体は p005）。

## 検証

build の warning の確認、ELF の検査、試しの guest の実行。

## 所有 path

`userland/packages/lang/python3/`、`plan/ws126/`、自分の worktree の `build/`。

## 依存・未決の判断

p001。
