<!-- awesome-plan project=zedbsd record=ws125-p001 -->

# ws125-p001: 取得・検証・監査と cross build

Parent: [WS125](../ws.md)
Status: planned
Disposition: normal
Queue / attempts: none
Goal: vim の tarball を確定・検証・監査し、zedBSD amd64 向けに cross build して binary と runtime を stage する。
Prerequisites: なし
Investigation bound: 3 時間。

## 範囲

- 9.2 系の最新の tag を `git ls-remote` で確かめ、GitHub の tag archive を取得し、`git get-tar-commit-id` が tag の commit と一致することを確かめ、size・SHA-256・ROOT を実測する。`plan/ws125/provenance.md` に記録。license の機械監査（`plan/tools/packages/audit-licenses.sh` 相当）。
- `userland/packages/editors/vim/Makefile`（`ZEDBSD_EXT_vim_*`、`ZEDBSD_EXTERNAL_SOURCE`）、`patches/`（config.sub ほか必要なもの）。
- configure: `--host` に cross の triple、`--with-tlib=curses --disable-gui --without-x --enable-multibyte --with-features=huge --disable-netbeans --disable-nls`（nls は libintl の実体を確かめて決める）、`--prefix=/usr`、cache の `vim_cv_*`（根拠を results に書く）。
- `make` と `make DESTDIR=… install` で stage。`check-dynamic-elf.py` で ELF を確かめる。

## 受け入れ

- 検証済みの source、stage の `/usr/bin/vim` と `/usr/share/vim/vim92/`、ELF の検査 PASS、provenance と監査の結果。

## 検証

build の warning の確認、ELF の検査。guest で試しに起動してよいが、受け入れは p003。

## 所有 path

`userland/packages/editors/vim/`、`plan/ws125/`、自分の worktree の `build/`。

## 依存・未決の判断

依存なし。D1・D2 は p003 で要る。
