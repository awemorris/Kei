<!-- awesome-plan project=zedbsd record=ws014-p011 -->
# ws014-p011: Model viewer の 8 個目の vkAllocateMemory -4 — memory の量と上限の測定（BUG-144、BUG-120 との関係）

Status: in-progress（測定の準備と読みの見積もり済み・T1 の測定待ち。修正は user の助言の後）
Disposition: normal
Parent: [WS014](../ws.md)
Bug: [BUG-144](../../bugs/BUG-144.md)（関係: [BUG-120](../../bugs/BUG-120.md)）
Queue: q643（P1 generation11、2026-10-03。user「Model viewerが作成する頂点リストやテクスチャなど、もしかして非常に大きいんじゃないでしょうか。…CPUとGPUのメモリ使用量がわかったら教えてください。アドバイスできるかも。たぶんBUG-120と同じバグなんじゃないかなあ？それも確認してください。」）

## 読みの見積もり（model の file と mview の renderer.c から計算、1 個あたり）

| 物 | 大きさ | 置き場所 |
| --- | --- | --- |
| texture 13 枚（RGBA8、1024² ×4・512² ×2・512×1024 ×2・1024×512 ×3・1024×256 ×2） | 基本 30 MiB、mipmap 込み約 40 MiB | device-local（host-visible を避ける） |
| 頂点 25,861 × 32 B | 0.8 MiB | device-local |
| index 37,000 三角形 × 12 B | 0.43 MiB | device-local |
| staging buffer（一番大きい texture 1 枚分、blit の mipmap なら 4 MiB、CPU の mipmap なら 5.3 MiB） | 4〜5.3 MiB | **host-visible、map したまま一生持つ** |
| scene の uniform（--shading=pixel の時） | 数百 B | host-visible |
| depth 960×640×4 | 2.3 MiB | device-local |
| swapchain の image（WSI、960×640×4 × 枚数） | 約 2.3 MiB × 枚数 | 共有（zdesktop が import） |
| CPU: model の読み込み（model.txt 2.6 MB の解析、texture の pixel 30 MiB を memory に読む） | RSS は 30〜40 MiB 以上の見込み | process の heap |

## 上限の見立て

- **Venus（QEMU、BUG-144）**: CPU から map できる allocation（`GPU_BLOB_MAPPABLE`）だけが host-visible の aperture（`hostmem=256M`、
  `venus_aperture_reserve` の first-fit、compaction なし）を使う。足りないと ENOSPC → `VK_ERROR_OUT_OF_DEVICE_MEMORY`（-4）。device-local の texture
  （約 40 MiB）は host 側（lavapipe）に置かれ aperture を使わない。mview 1 個の aperture は staging の 4〜5 MiB と transport の reply・stream 程度で、
  mview だけで 256 MiB を使い切るとは見積もれない。7 つの app（各 app の CPU 描画の frame の upload 用の host-visible、zdesktop の glass・壁紙など）の
  合計で尽きた見込みだが、**数字が要る**。
- **i915（実機、BUG-120）**: GPU の object の枠 `I915_GT_MAX_OBJECTS` 128（`src/drivers/gpu/i915/memory.c`）は i915 の driver の表で、Venus は使わない。
  よって BUG-144 と BUG-120 は**別の仕組み**だが、どちらも「固定の大きさの GPU の資源の枠を多くの app が分け合って尽きる」同じ形。mview 1 個は i915 で
  texture 14・頂点・index・staging・uniform・depth・swapchain で約 20 の object を使う見込みなので、実機では mview を 5〜6 個開くと先に BUG-120 の枠に当たる。

## 測定の準備（この Phase の commit）

- `src/drivers/gpu/venus/venus.c`: 診断の log（恒久）。context を開いた時に `venus: context=N opened pid=P`、aperture に空きが無い時に
  `venus: aperture full context=N request=B aperture=A used=U blobs=K largest_hole=H` と context ごとの `venus: aperture context=N bytes=B blobs=K`。
  kernel の build は warning 0、`amd64 vmunix check: PASS`。
- `plan/ws014/tests/aperture-bug144.sh`: App Home の全 app を開いた後の venus の log と全 process の RSS、続けて mview だけを 1 個ずつ失敗まで
  開いて各 RSS と失敗の時の aperture の内訳。T1 に依頼（結果待ち）。

## 次

数字（aperture を誰がどれだけ持つか、mview の RSS、何個目で失敗するか、穴の断片化）を Q1 経由で user に報告し、助言を待ってから直す。直し方の候補:
mview の staging を load の後に解放する（4〜5 MiB の aperture を返す）、texture の upload を小さな staging の分割で行う、aperture の割り当てを
大きさ順・断片化の少ない方式に、hostmem を大きくする（QEMU の起動の option、試験の環境）、i915 の object の枠を動的に（BUG-120）。
