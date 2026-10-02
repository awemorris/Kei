<!-- awesome-plan project=zedbsd record=ws129-p002 -->
# ws129-p002: image の license の一覧

Status: planned
Disposition: normal
Parent: [WS129](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none（未承認）
目安: 3〜4h

## 範囲

1. release の image（当面は `config/ci/config-amd64.mk` とデモの config の和）に入る全ての component を列挙する: 自作の source（Zlib 等、SPDX の行から）、`userland/packages/` の外部 package（openssh・openssl・curl・zlib・expat・libcxx・clang・ca-certificates・font など）、
   firmware（i915・AX211・RTL8822B）、取り込んだ表（RTL8822B の `.inc`、browser の Unicode/WHATWG）、LLVM の runtime。
2. 各 component の license の本文の場所（image の中の path と source の中の path）を確かめ、足りないものを洗い出す。
3. 一覧を生成する script（`tools/release/license-inventory.py` など、置き場所は p001 と合わせる）: rootfs の staging と package の metadata から一覧（component・版・license・本文の path）を作り、image の `/usr/share/licenses/` に一覧と本文が揃うかを検査する。
4. `plan/tools/packages/audit-licenses.sh` を走らせ、結果を記録する。GPL 系が image に入っていないことを確かめる（範囲外の例外は guardrail の記録だけ）。

## 受け入れ

一覧（`plan/ws129/licenses.md` と生成物）、script の host の試験、audit の結果、足りない本文の一覧（直すのが他の WS・package なら main に依頼）。image に本文を入れる変更は package の Makefile ごとになるので、その差分は main と調整する。

## 所有 path

`plan/ws129/`、新しい script（`tools/release/` の下）。

## 依存

なし。release の config が p004 で決まったら p006 の前に再生成する。

## 未決の判断

なし。
