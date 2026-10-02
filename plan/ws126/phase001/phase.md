<!-- awesome-plan project=zedbsd record=ws126-p001 -->

# ws126-p001: 取得・検証・監査と cross build の方針

Parent: [WS126](../ws.md)
Status: planned
Disposition: normal
Queue / attempts: none
Goal: CPython の tarball を確定・検証・監査し、zedbsd 向けの configure を試して、module ごとの依存と cross build の方式を決める。
Prerequisites: なし
Investigation bound: 3 時間。build の全体は p002。

## 範囲

- 3.15.0 の公開を確かめ（無ければ 3.14 系の最新）、`Python-<版>.tar.xz` を取得し、python.org の Sigstore の bundle か PGP の署名で検証し、size・SHA-256・ROOT を実測する。`plan/ws126/provenance.md`。license の機械監査（同梱の expat・libmpdec・HACL*・mimalloc 等の license を含む）。
- 展開した tree で `config.sub`、`configure` の `ac_sys_system`／`MACHDEP`、共有 library（`LDSHARED`・`CCSHARED`・`SHLIB_SUFFIX`・`LDLIBRARY`）、cross の cache（`ac_cv_file__dev_ptmx=yes`、`ac_cv_file__dev_ptc=no`、`ac_cv_buggy_getaddrinfo=no` ほか）の要る値を洗い出し、patch と cache の一覧を作る。
- sysroot に対して configure を試し、`Modules/Setup.stdlib` と configure の結果から module ごとの「作れる・依存が無い・libc の不足」の表を作る（WS の表を実測で更新）。
- 方式を `plan/ws126/design.md` に決める: build 用の Python（同じ版を host で作る）、拡張 module を共有 object にするか静的にするか、`--enable-shared` の有無、install の除外（`test/`・`idlelib`・`tkinter`・`turtledemo`）、`.pyc` を build で作るか（`compileall` を build 用の Python で）、P5 の `python3 -m test` の集合。

## 受け入れ

- provenance・監査、patch と cache の一覧（根拠つき）、module の表、design.md の方式。p002・p003 に人間の判断を持ち込まない（D1 は p004 だけに効く）。

## 検証

`archive.sh verify` が実測値で ok。configure の試行の log の要点を記録。build はしない。

## 所有 path

`plan/ws126/`、`build/distfiles` への取得、自分の worktree の `build/`。`userland/packages/lang/python3/` の patch の試作は置いてよい。

## 依存・未決の判断

依存なし。D1（T3 の範囲）は p004、D2（pip）・D3（image の既定）は p005 までに要る。
