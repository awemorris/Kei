<!-- awesome-plan project=zedbsd record=ws046-p016 -->

# ws046-p016: BUG-093 Noct の source の stamp を patch の中身で判定する

Parent: [WS046](../ws.md)
Status: in-progress
Disposition: normal
Queue: Q1 の割り当て（2026-10-03、P1。toolchain の変更は Q1 が main として許可、`userland/base/noct/Makefile` だけ）
Bug: [BUG-093](../../bugs/BUG-093.md)

## 目的と受け入れ条件

新しい worktree では checkout で Noct の patch の mtime が新しくなる。そのため `make toolchain` が共有の `build/NoctLang` を取り出し直そうとし、
「refusing to replace existing source tree」で止まる。

受け入れ条件:
- 中身が同じ patch なら、mtime に関係なく作り直さない。worktree で `make -n toolchain` に Noct の取り出し・cmake が無いこと。
- 中身が変われば、今まで通り検出すること。
- 共有の tree は消さない。

## 変更

`userland/base/noct/Makefile` だけを変える。差分は [BUG-093-noct-identity.diff](../../bugs/BUG-093-noct-identity.diff) と同じ。

- identity に `patches-sha256` の行を足す。host の tree は 0001〜0003、target の tree は 0001〜0004 を連結した SHA-256。
- make が読む時に、stamp と identity を中身で受け入れる（`NOCT_HOST_SOURCE_ACCEPTED`・`NOCT_SOURCE_ACCEPTED`）。受け入れた stamp の rule は前提を持たない。
- 受け入れない時は今の動きを保つ。host の tree は拒み、理由の行を出す。target の tree は置き換える。
- 移行（main）: 共有の `build/NoctLang/.zedbsd-source-identity` の末尾に `patches-sha256=6f317e482eb695d5dce44d1ef2a029248d64415283ba5b5a1505fc2bba6031b6` を足す。

## 検証

scratch（repo の複写に差分を当てた物）での結果は BUG-093 の ticket にある:
- 行の無い identity は拒む。
- 行のある identity と新しい mtime では何もしない。
- 中身を変えると取り出す。
- host と target の verify は rc 0。

移行の後に、worktree で `make -n toolchain` と `make toolchain` を流す（結果は下に追記する）。
