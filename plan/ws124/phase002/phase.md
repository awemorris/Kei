<!-- awesome-plan project=zedbsd record=ws124-p002 -->

# ws124-p002: host 用 Emacs と package の骨組み

Parent: [WS124](../ws.md)
Status: planned
Disposition: normal
Queue / attempts: none
Goal: lisp の byte-compile と DOC の生成に使う host 用の Emacs（同じ版）を作り、target の package の Makefile の取得・展開・patch を整える。
Prerequisites: p001 cleared（版・hash・patch の一覧・分担）
Investigation bound: 3 時間。

## 範囲

- `userland/packages/editors/emacs/Makefile`: `ZEDBSD_EXT_emacs_*`（版・URL・size・SHA-256・ROOT・patch）と `ZEDBSD_EXTERNAL_SOURCE`、`patches/`（config.sub の zedbsd、`opsys` の zedbsd）。
- host 用 Emacs: 同じ distfile を別の作業 directory（`build/packages/emacs/host`）に展開し、host の cc で `--without-x --without-native-compilation` など最小の構成で build する。install は作業 directory の中だけ（host の `/usr` に入れない）。target の build は `EMACS=` でこれを使う。
- host の道具の依存（make 変数・stamp）。toolchain（`toolchain/`、共有の `build/llvm` 等）は変えない。

## 受け入れ

- `make emacs-source`（相当）で検証・展開・patch が通る。host の Emacs が `--batch` で動き、`emacs-version` が target と同じ。
- 通常の build が暗黙に network I/O をしない（取得は `-download` の時だけ）。

## 検証

- host の Emacs で `emacs --batch -f batch-byte-compile` を小さな `.el` に当てる。
- build（warning の確認）。boot test は不要（image に入れない）。

## 所有 path

`userland/packages/editors/emacs/`、`plan/ws124/`、自分の worktree の `build/`。

## 依存・未決の判断

p001。未決の判断なし。
