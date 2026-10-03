<!-- awesome-plan project=zedbsd record=ws134-p002 -->
# ws134-p002: システムモニターの骨組み（M1）

Status: in-progress（P2、2026-10-03。実装済み・T1 の試験待ち）
Parent: [WS134](../ws.md)
設計: [design.md](../design.md) §0・§1.5・§1.6・§2・§4・§5

## 範囲

design.md §5 の p002: package・3 OS の build・libkeiui の窓（`KUI_PRESENT_NONE`）と自前の Vulkan（FIFO、自前の frame callback の許可、
SURFACE_LOST の回復）・title bar（時間軸の 4 つの control）・data source の層と sim・replay・履歴・plate と文字（glyph の atlas）・graph・`ZMON` の log・
試験の mode（`--clock=fixed`）。3D・動き・操作は p003・p004。

## 実装（`userland/desktop/monitor/`）

| file | 中身 |
| --- | --- |
| `monitor.h` | data の型（`sm_info`・`sm_frame`、valid と simulated の bit）、source・history・rules・format の API。Wayland・Vulkan に依らない（host 試験が build する） |
| `source.c` | sim（seed の決まる model: 波・低域の雑音・spike・閾値の上の excursion（sim の時だけ）・hot core・GPU の温度の 1 次遅れ）と replay（text の `info`・`frame` の行、表で読む） |
| `history.c` | 系列ごとの 1 時間の ring |
| `rules.c` | 4 段の状態（注意 10 秒で Elevated、60 秒で Warning、危険の閾値で Critical、5 秒ごとに 1 段ずつ下がる）。**simulated の field は使わない** |
| `format.c` | %・size（二進）・rate（十進、bit/byte）・used/total・uptime |
| `atlas.c` | 6 つの文字の style の ASCII の glyph を libkeiui の text で一度描く atlas（数字は JetBrains Mono、label は Inter） |
| `scene.c` | layout（論理 1280x800 を伸ばす、文字は小さい方の尺度）と frame の shape・glyph の列: 背景・in-window の header（host・CPU 数・uptime・状態の chip）・上段 5 枚・CPU の tile・状態（平面の入れ子の正方形、3D は p003）・GPU の card と meter・Network・Memory の層・Disk の 2 lane と Latency・Events。値の文字を `ZMON TEXT plate=… value=…` で log |
| `render.c` | Vulkan（Notes と同じ形）: 2 つの pipeline（shape・glyph）、atlas の linear の image、`OUT_OF_DATE` の作り直し、`SURFACE_LOST` の surface と swapchain の作り直し（`sm_renderer_recover`） |
| `main.c` | option、窓（libkeiui）、titlebar（GENERIC の control 4 つ、checked）、自前の `wl_surface_frame` を描画の許可にする loop（1 秒届かなければ `ZMON VISIBLE 0`）、`ZMON` の READY・SAMPLE・LEVEL・FRAME（5 秒）・MEM（30 秒）・DONE |
| `shaders/` | `draw.vert`・`shape.frag`（solid・plate の SDF・line・gradient・area・disc）・`glyph.frag`、`regenerate.py` → `shaders.h` |

build: `Makefile`（zedBSD）・`Makefile.linux`・`Makefile.freebsd`、`keiland-linux.mk`・`keiland-freebsd.mk` の一覧、`platform/amd64/vmunix.mk`
（filter-out と動的 link の規則）。試験: `plan/ws134/tests/`（`config-amd64-monitor.mk`・`build-monitor-image.sh`・`replay/normal.txt`・
`host/`・`monitor-p002.sh`）。`plan/tools/files/build-files-image.sh` に他の試験の file を足す `FILES_EXTRA` を足した。

## 確かめ

- build: `make ZEDBSD_CONFIG=plan/ws134/tests/config-amd64-monitor.mk … bin/monitor`（zedBSD の clang -Werror）warning 0。image の build（`build/p2-p002-img`）exit 0。
- Linux: 8 file を gcc の `-std=gnu17 -Wall -Wextra -Werror -fsyntax-only`（Linux の build の flag）で通した。`make keiland-linux` の link と FreeBSD は未実施。
- host 試験（`plan/ws134/tests/host/run.sh`、ASan/UBSan）: PASS（sim の決定性と範囲・replay の値と時刻・履歴の ring・rules の時間と simulated の除外と 1 段ずつの下降・format）。
- `style-check.py`: `userland/desktop/monitor/*.c` の違反 0（試験の host-test.c に段落の comment の指摘が残る）。
- QEMU（T1 に依頼）: `monitor-p002.sh`（replay の固定の時計で `ZMON TEXT` の値が入力と一致、sim で 20 秒の sample と frame）と PNG。結果は未着。

## 未実施・残り

- 最小化して戻す試験（`ZMON VISIBLE 0`→`1`）・golden の PNG の比較・1920x1280・5 分の `ZMON MEM` は、最初の PNG を見てから p003 の試験と一緒に。
- App Home の行（`apps.conf`）は p003 で（絵 `monitor` の有無を含めて）。

## stub の項目（design.md §1.5、今は全部の値が stub）

hostname・CPU の数・uptime は本物（libc の標準の関数）。CPU・memory・swap・network・disk・latency・GPU の全ての値は sim の model（`--source=sim`）か記録（replay）。本物にする API は design.md §1.5 の表（p005〜p009、p011〜p013）。
