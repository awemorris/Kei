<!-- awesome-plan project=zedbsd record=ws034-p058 -->
# ws034-p058: 移植で見つかった libc の不足をまとめて取り込む（第 1 弾）

Status: planned
Disposition: normal
Parent: [WS034](../ws.md)

## 方針（2026-10-02 ユーザー）

「このように、GNUソフトウェアの移植で判明した、POSIX標準でないが重要なAPIは、積極的に取り込みます。」（[Guardrail](../../guardrail.md) の「移植で見つかった API の取り込み」）

## 範囲（ws115 の port-contract §11 などで見つかったもの）

1. `PRId64`・`PRIu64` などの `<inttypes.h>` の書式が `int64_t`（amd64 で `long`）と合っていない（`"lld"`→`"ld"`）。header の不整合の修正（libtiff・libxkbcommon・glib・expat の `-Wformat` の原因）。
2. C11 の `static_assert` を `<assert.h>` に。
3. `RTLD_NOLOAD` を `<dlfcn.h>` と ld.so に（既に読み込まれていれば handle を返し、無ければ読み込まずに NULL）。
4. `CLOCK_PROCESS_CPUTIME_ID`（と要るなら `CLOCK_THREAD_CPUTIME_ID`）を `<time.h>` と kernel の時計に（kernel の対応の要否を調べ、要るなら kernel の変更も範囲。HAL の API の変更は事前承認）。
5. `<arpa/nameser.h>`（resolv.h にある定義を標準の場所に）。
6. `<string.h>` から BSD・glibc と同じく `strcasecmp` などの `<strings.h>` の宣言が見えるようにするか（POSIX の名前空間を WS001 の観点で確かめて決める）。
7. C99 の `<fenv.h>` の丸めの mode（`FE_UPWARD`・`FE_DOWNWARD`・`FE_TOWARDZERO` と `fesetround`）。今は最近接への丸めだけ（GTK 4.18.6 が使う。ws115-p002）。libm（WS076）の正しさと合わせて確かめる。
8. `uint`・`ushort`・`ulong` などの BSD・glibc の `<sys/types.h>` の型（GTK が使う。名前空間は WS001 の観点で、feature macro の下に置くかを決める）。
9. `<malloc.h>`（非標準だが広く使われる。GTK の roaring が include）と、`sigjmp_buf` と `jmp_buf` を同じ型にするか（glibc と同じ。libpng の longjmp と sigsetjmp の組で GTK が困った）。
10. 既に追加済み（Q1、2026-10-02）: `<alloca.h>`、`getc_unlocked` ほか stdio の `*_unlocked`。

各項目は ABI を壊さないことを確かめ、libc と base の program の build（warning 0）、host の試験、boot-test。追加した API の一覧を Guardrail の索引に足す。package 側の回避の patch（glib・cairo・pango・libxkbcommon・libepoxy）は、libc に入った後に外せるかを WS115 で確かめる。
