<!-- awesome-plan project=zedbsd record=ws129-p003 -->
# ws129-p003: 版の一つの源

Status: planning（p001 とユーザーの版の名前の判断を待つ）
Disposition: normal
Parent: [WS129](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 2h

## 範囲

Makefile の変数（例 `ZEDBSD_RELEASE`・`ZEDBSD_RELEASE_NAME`、nightly の既定は git の短い hash つき）から header と `/etc/os-release` を生成し、libc の `uname`（`userland/base/libc/posix.c:5324-5325`）、
起動の表示（`src/hal/i386/cmain.c` の版の文字、amd64 の起動の表示があればそれ）が使う。Settings の About（WS089 の `userland/desktop/settings/about.c`）の差分は main に依頼する。

## 受け入れ

QEMU の SSH で `uname -a` と `/etc/os-release`、`plan/tools/boot-test.sh` の PNG。build warning 0、全文規約。版の変数を変えると make が作り直す（依存の確認）。

## 所有 path

`Makefile` の版の部分、生成の規則、`userland/base/libc/posix.c` の uname の部分、`src/hal/i386/cmain.c` の版の文字、`plan/ws129/`。

## 依存

p001、ユーザーの版の名前。

## 未決の判断

版の名前。
