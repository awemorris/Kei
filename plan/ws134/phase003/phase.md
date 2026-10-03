<!-- awesome-plan project=zedbsd record=ws134-p003 -->
# ws134-p003: システムモニターの 3D と動き（M2）

Status: uncleared（q645、P2、2026-10-03。T1-043 で sim の fps 4.7（条件 15 以上）。P2 はユーザーの指示でラップアップ、直しは未着手）
Parent: [WS134](../ws.md)
設計: [design.md](../design.md) §2.2・§3・§4.2

## 実装

- `space.c`（新）: 状態コア（3 つの入れ子の箱、CPU で上面が明るく、GPU で中殻が厚く脈動、memory で殻の間の点が密に（Available 10% 未満でアンバー）、
  network で 3 本の軌道の点が速く、disk の仕事で下のリングの明るい弧が回る、内の光は 4 秒の呼吸、level の色）、CPU のレリーフ（core ごとの箱が
  load でせり上がる、85% 超は縁が光る、奥から描く）、motion（pointer に 2 Hz の spring で追う camera の傾き、値への 1 次遅れ、phase）。
  CPU で投影して面ごとに陰影を付けた 2D の三角形にし、shape の shader で描く（design.md §4.2 の追記: i915 の compiler の制約、depth・instancing 無し）。
- `draw.c`・`draw.h`（新）: 描画の基本（色の token、quad・rect・gradient・plate・disc・text・segment・triangle）を scene.c から分けた。
- `scene.c`: 層ごとの視差（L0 4・L1 2・L2 1 論理 px、pointer の傾き＋12 秒の揺れ）、警告の段の plate（Elevated で縁が淡いアンバー、Warning で
  アンバーと前へ、Critical で coral）、値の slide（変わった桁が 180 ms で上（増）・下（減）から入り、前の値が淡く残る）、Network の 2 本の流れ
  （RX は左から・TX は右から、量で数と明るさ）、Disk の lane の流れ（latency で奥に詰まる）、GPU が 1 つの時の使用率の履歴、GPU の短い名前。
- `rules.c`: rule ごとの level（plate の警告に使う）、`count_simulated`（sim の source の時だけ sim の値で段を付ける。system の時は除く）。
- `main.c`: titlebar の時間の control を group 無しの text の pill に（group の face は icon で「...」と出ていた、T1-041 の PNG）、sim の GPU の名前を
  描画の device の名前に（「GPU 0 GPU 0」の重複）、sim は起動の前に 5 分の履歴を埋める（graph が右端だけだった）、Memory の上段は使用量と「of 8 GiB」。
- 試験: replay `calm8`・`warning`・`critical`（8 CPU、130 秒）、`monitor-p003.sh`（3 つの replay の固定の時計の PNG と level の順、sim 16 CPU・2 GPU を
  pointer を動かして 20 秒、fps 15 以上）。host の preview（`tests/host/preview.sh`・`preview.c`・`preview.py`）。

## 確かめ

- build: zedBSD の monitor（-Werror）warning 0、image（`build/p2-p002-img`、replay を含む）exit 0（image は scene.c の comment と空行だけの直しの前の tree）。
  Linux の flag の gcc `-fsyntax-only -Werror` 全 file。style-check 違反 0。host 試験 PASS（rules の count_simulated を足した）。
- host の preview（guest の絵の参考）: [critical-host.png](preview/critical-host.png)（Critical: CPU と Disk の plate が coral、コアの内が赤橙、Events 3 件）、
  [sim-1920-host.png](preview/sim-1920-host.png)（1920x1240、16 CPU・2 GPU、pointer で傾けた）。
- replay の level の時刻（host で計算）: warning は Elevated 70 s・Warning 120 s、critical は Elevated 15 s・Warning 65 s・Critical 110 s（latency 60 ms の 10 秒）。
- QEMU（T1 に依頼）: `monitor-p003.sh`。結果は未着。

## stub の項目

p002 と同じ（全ての値が sim か replay、本物は hostname・CPU の数・uptime）。

## 結果（T1-043、a02f7eecd、QEMU、2026-10-03）

uncleared。`monitor-p003.sh` を 2 回（66 s・65 s、変更なし）流して同じ:

- FAIL: `sim: 4 fps (want 15 or more) MISSING`（sim.log `ZMON FRAME fps=4.7 wait_ms=5.21`・`4.7 / 6.92`・`4.6 / 5.96`）。guest は llvmpipe で描いている
  （`ZMON READY … device="llvmpipe (LLVM 19.1.7, 256 bits)"`）。流し直しの時の host: CPU の圧力 0、IO の avg10=6.87、T2 の QEMU が 1 つ。
- ok: 3 つの replay の level の順（Elevated → Warning → Critical）、state の文字、sim の cpus=16 gpus=2、failure 無し、zdesktop の ERROR 無し。
- PNG（T1 の目視）: critical.png に 8 個の CPU の箱・Critical の赤い枠・Events 3 行、sim.png に 16 個の箱と GPU 2 台。
  `/home/awe/zedBSD-worktrees/t1/build/t1-monitor/t1-043/`・`t1-043-retry/`。
- 実機・Venus の image では未測定。

### fps の調べ（途中）

- host で scene を作る CPU の時間を測った（`preview.c` に `PREVIEW_BENCH=N` を足した、commit 1227ff647）:
  `PREVIEW_BENCH=200 plan/ws134/tests/host/preview.sh OUT 1200 690 120000 sim:3:16:2` → **0.284 ms/frame**（20796 vertices、62 draws）。
  guest の CPU が host の 10 倍遅くても 3 ms で、scene の build は原因ではない。
- guest の `wait_ms`（present の後の fence の待ち）は 5〜7 ms で、1 frame の 213 ms の大半ではない。残りは未測定:
  `vkAcquireNextImageKHR`（FIFO で compositor が buffer を返すまで）、`vkQueueSubmit`・`vkQueuePresentKHR`（llvmpipe が描画や wl_shm への複写を
  ここで同期に行う可能性）、compositor の frame callback の間隔（zdesktop の合成も llvmpipe）。

### 再開の条件（次の担当へ。Q1 の 2026-10-03 の指摘 3 点）

1. fps: `render.c` の各段（acquire・submit・present・fence）と frame callback の間隔を `ZMON FRAME` に足して、QEMU の llvmpipe の image と
   **Venus の image の両方**で測り、phase.md に両方書く。llvmpipe の fill が重いなら描画を軽くする（全面の背景・plate の重なり・殻の半透明の
   overdraw を減らす、変わらない層を毎 frame 描き直さない等）。条件を llvmpipe に合わせて下げるだけで済ませない。比較に同じ image の他の
   Vulkan app（Notes 等）の fps も見る。
2. sim の Graphics の 2 台目が「GPU 1 Simulated GPU」と出る → ユーザーの「simという表記はつけなくていいです」に反する。`source.c` の sim の
   GPU 名を、それらしい名前に（例: 実在の型番を騙らない一般的な名前）。`plan/ws134/tests/host/host-test.c` の期待も合わせる。
3. GPU 0 の名前「llvmpipe (LLVM 19.1.7, 2」が途中で切れる → 括弧の前で切るか省略記号（`scene.c` の `short_name` の周り）。
4. 直したら build（warning 0）・host 試験（`tests/host/run.sh`）・preview を流し、T1 に `monitor-p003.sh` を再依頼。

