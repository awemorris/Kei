<!-- awesome-plan project=zedbsd record=ws074-p177 -->
# ws074-p177: BUG-133 — `wb_units_reserve` の幾何の増長後の byte 数の overflow

Q1 が割り当て（2026-10-03、P2）。ws074 は別の session（browser3）も使う。この Phase は P2 の q645 の続きで、変更は `base/buffer.c` の 1 か所だけ。

Status: in-progress（実装と host 試験が済み、cleared の判定は Q1）
Disposition: normal
Parent: [WS074](../ws.md)
Bug: [BUG-133](../../bugs/BUG-133.md)

## 範囲（Q1、2026-10-03「wb_units_reserve の算術を直す小さな修正なら行う。変更は最小に」）

`userland/desktop/libbrowser/base/buffer.c` の `wb_units_reserve` だけ。他の browser の code は変えない。QEMU の試験は要らない（Q1）。

## 原因

`wb_units_reserve` は要求を `SIZE_MAX/2 - length` で抑えるが、`buffer_grown` は need が収まるまで倍にし、`SIZE_MAX/2` を越える直前の値の倍
（64 bit で 2^63）まで行く。`capacity * sizeof(uint16_t)` が 2^64 になって 0 に wrap し、`realloc(data, 0)` が成功して、確保していない容量を
`units->capacity` に書いていた（後の copy が heap を越えて書く）。byte の buffer（`wb_buffer_reserve`）は要素が 1 byte なので wrap しない。

## 直し

`buffer_grown` の後、capacity が `SIZE_MAX / sizeof(uint16_t)` を越えたら need そのものに落とす（need ≤ SIZE_MAX/2 なので byte 数は収まる）。
倍の増長はそれ以外で変わらない。差分は buffer.c の +7/−1 行（comment を含む）。

## 確かめ（host）

- `plan/ws074/tests/bug133-units.c`（`cc -I userland/desktop/libbrowser plan/ws074/tests/bug133-units.c userland/desktop/libbrowser/base/buffer.c
  userland/desktop/libbrowser/base/utf.c`）: 空の buffer に 2^62+1 units の reserve が、**直す前は error 0・capacity 2^63（FAIL、ticket の静的な計算の
  再現）**、直した後は ENOMEM（PASS）。SIZE_MAX/2 を越える要求の ENOMEM と通常の append も ok。`-fsanitize=address,undefined` でも PASS。
- zedBSD の clang（-Werror）で buffer.c の compile は warning 0。`style-check.py` は buffer.c の変えた行・試験に違反 0。
- QEMU・browser の回帰は Q1 の指示で行わない（ふつうの大きさの要求では倍の増長が変わらないため）。
