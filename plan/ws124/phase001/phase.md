<!-- awesome-plan project=zedbsd record=ws124-p001 -->

# ws124-p001: 取得・検証・license 監査と cross build の方針

Parent: [WS124](../ws.md)
Status: planned
Disposition: normal
Queue / attempts: none
Goal: Emacs の tarball を確定・検証・監査し、zedbsd 向けの configure を試して cross build の方式を決める。
Prerequisites: なし
Investigation bound: 3 時間。build の全体は p002・p003。

## 範囲

- 31.x の最新の安定版を確かめ（台帳の 31.1 か、新しい bugfix 版）、`build/distfiles` に取得し、GNU keyring で署名を検証し、size・SHA-256・根 directory を実測する。
- `plan/tools/packages/audit-licenses.sh` 相当の機械監査。GPL-3.0-or-later は package の境界の中（[設計方針 §2.1](../../master-design-policy.md)）。結果を `plan/ws124/provenance.md` に書く（形式は [WS032 provenance](../../ws032/provenance.md)）。
- 展開した tree で `configure.ac` の `opsys`（zedbsd → `unported`）、`config.sub`、`tputs` の検出、cross 時に既定値へ落ちる判定を洗い出し、必要な patch と autoconf cache の一覧を作る。base の curses（`tgetent`・`tputs`）で `--with-terminfo` が通るかを sysroot に対する configure で確かめる。
- build の分担を決めて `plan/ws124/design.md` に書く: host の Emacs（同じ版）が作るもの（`.elc`、DOC、charsets、leim、unidata）、target で作るもの（`temacs`、`emacs`）、dump の方式（`--with-dumping=none` と guest での pdump の比較）、lib-src の host 側の道具（`make-docfile`・`make-fingerprint`）。
- configure の option の初期案: `--without-x --without-ns --without-pgtk --without-gnutls --without-xml2 --without-libgmp --without-tree-sitter --without-native-compilation --without-modules --without-sound --without-dbus --without-gsettings --without-selinux --without-mailutils --with-terminfo`、zlib は任意。

## 受け入れ

- provenance（版・URL・size・SHA-256・署名の結果・根 directory）と監査の結果がある。
- 必要な patch（目的と理由つき）と cache の値（根拠つき）の一覧、host/target の分担と dump の方式が design.md に決まっている。p003 に人間の判断を持ち込まない（GUI の要否 D1 は端末版で進める）。

## 検証

- `userland/packages/tools/archive.sh verify` が実測値で ok。
- sysroot に対する configure の試行の log の要点（通った項目・止まった項目）を記録する。build はしない。

## 所有 path

`plan/ws124/`（phase001、provenance.md、design.md）、`build/distfiles` への取得、自分の worktree の `build/`。`userland/packages/editors/emacs/` の patch の試作は置いてよい（p002 で確定）。

## 依存・未決の判断

- 依存なし。D1（GUI）・D2（image の既定）は p004 までに要る。ws034-p010 との重複の整理は main。
