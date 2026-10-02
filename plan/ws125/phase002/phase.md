<!-- awesome-plan project=zedbsd record=ws125-p002 -->

# ws125-p002: package の staged tree を image に入れる共通の仕組み

Parent: [WS125](../ws.md)
Status: planned
Disposition: normal
Queue / attempts: none
Goal: package が数千の file（vim の runtime、Emacs の lisp、Python の標準 library）を、file ごとの `--file` を並べずに image へ入れられるようにする。
Prerequisites: なし
Investigation bound: 3 時間。

## 背景

今の `ZEDBSD_PACKAGE_FILES` は `--file DEST=SRC` を file ごとに並べ、root の `Makefile` の rootfs の規則と config の stamp（`files='$(strip $(ZEDBSD_PACKAGE_FILES))'`）で 1 つの shell の引数に展開される。Linux の 1 引数の上限（MAX_ARG_STRLEN = 128 KiB）により、数千の file は入らない。`make-arch-overlay-image.py` にも同じ並びが渡る。

## 範囲

- 新しい指定（案: `--tree DEST=STAGEDIR`、または stage が書く manifest の file を `--manifest` で渡す）を、root の `Makefile` の rootfs の組み立て、config の stamp（中身ではなく manifest の hash で変化を検出）、`tools/build/make-arch-overlay-image.py` と検査の側に足す。mode（実行可能な file）と symlink を保つ。
- 既存の `--file`・`--mode` の振る舞いは変えない。
- 小さな試験の package（または vim の stage）で、tree の中身が image に入り、file 数・mode・link が一致することを確かめる。

## 受け入れ

- 3000 file 以上の tree を入れても build が通り、image の中身が stage と一致する（file の一覧・size・mode を照合）。既存の image の中身が変わらない（変更前後の一覧の差が 0）。

## 検証

image の中身の照合の script（`plan/ws125/tests/`）、build、`plan/tools/boot-test.sh`。

## 所有 path

root の `Makefile` の rootfs・stamp の部分、`tools/build/make-arch-overlay-image.py`・`check-arch-overlay-image.py`、`userland/base/package.mk`（要れば変数の定義）、`plan/ws125/`。これらは全 package が共有するので、Queue にするときに main がこの所有を確認する。

## 依存・未決の判断

依存なし。WS124 p004・WS126 p005 の前提。
