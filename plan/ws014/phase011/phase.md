<!-- awesome-plan project=zedbsd record=ws014-p011 -->
# ws014-p011: Model viewer の 8 個目の vkAllocateMemory -4 — memory の量と上限の測定（BUG-144、BUG-120 との関係）

Status: cleared（Q1 判定 2026-10-03、2026-10-03 user「(A) 1 GiB に広げたことで十分として、BUG-144・124 を閉じる。」。T2-001: boot-test PASS、hostmem 1G で mview 14 個起動、256M は 4 個目で失敗の対照）。元の記載: in-progress（測定の準備と読みの見積もり済み・T1 の測定待ち。ENOSPC の意味の修正を実装。容量の修正は user の助言の後）
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

## BUG-124 との関係と -4 の読み直し（2026-10-03、P1）

- **-4 は `VK_ERROR_DEVICE_LOST`**（OUT_OF_DEVICE_MEMORY は -2）。ticket の「-4（OUT_OF_DEVICE_MEMORY）」は読み違いだった。
- BUG-124 の `vkCreateSwapchainKHR result=-4 errno=8`: zedBSD の errno 8 は **ENOSPC**。Venus の driver で ENOSPC を返すのは `venus_aperture_reserve`
  （host-visible の aperture の満杯）だけ（`gpu.c` の資源数の上限は Venus では UINT32_MAX）。よって **BUG-124 も BUG-144 と同じ aperture の満杯**で、
  窓を大きくした時の swapchain の作り直し（新しい image の blob を古い物が残る間に取る）で起きる。
- libvulkan の `vulkan_kernel_error`（`userland/desktop/libvulkan/context.c`）は ENOMEM だけを OUT_OF_DEVICE_MEMORY にし、**ENOSPC を DEVICE_LOST に変えて
  context 全体を失ったことにしていた**。そのため、窓や memory が入らなかっただけの app が終わっていた。
- 修正（容量の修正とは別の、意味の修正）: ENOSPC も OUT_OF_DEVICE_MEMORY にして context を失わせない。libvulkan の build は warning 0。
  容量そのもの（aperture の使い方・hostmem・staging）は測定と user の助言の後。

## q647: hostmem を大きくできるか（2026-10-03、P1、読み）

user「Venusの窓は私には判断できないです。もっと大きくしていいならしてください。」→ 確かめた結果、**今の kernel では 256 MiB より大きくできない**。

- `src/drivers/gpu/venus/transport.c` の `venus_map_aperture` は host-visible の BAR を**丸ごと** kernel に map し（blob の slice が BAR の
  移動に巻き込まれないように）、`bar.size > VENUS_MAX_APERTURE_BYTES`（256 MiB、`internal.h`）なら EOPNOTSUPP で aperture を断る。BUG-124 の
  「hostmem=1G で session が起動しなかった」はこれ。
- その上限は、amd64 の HAL の kernel の device の窓（`src/hal/amd64/space.c`、`AMD64_DEVICE_PD_COUNT` 256 × 2 MiB = **512 MiB**、全 device の map の共有）
  に収めるため。1 GiB の BAR を丸ごと map する余地は無い。kernel は blob の中身を自分でも読み書きする（`venus.c` 884・941 行の `resource->mapping`）
  ので、map 無しにはできない。
- OVMF・q35: QEMU の virtio-gpu の hostmem は 64 bit の prefetchable の BAR で、OVMF は 4 GiB より上に置ける（今の 256M も同じ BAR）。guest の
  RAM 8 GiB・host の memory は 1〜4 GiB の hostmem の妨げにはならない見込み（hostmem は host 側で必要な分だけ使う）。妨げは上の kernel の 2 つ。

大きくする道（どれも判断が要る）:
1. **HAL の device の窓を広げる**（例 256 → 1024 PD = 2 GiB）。`src/hal/` の変更で、HAL の責務（kernel の仮想 address の配置）に当たる → user の事前承認が要る。
   その上で `VENUS_MAX_APERTURE_BYTES` を 1 GiB に。
2. **Venus の driver を、BAR を丸ごとでなく blob ごとに map する作りに変える**（`drv_pci_device_map_bar_region` はある）。driver だけの変更だが、
   丸ごと map の理由（BAR の移動・slice の寿命）を設計し直す規模。生きている blob の合計は窓（512 MiB の残り）に縛られる。
3. 256 MiB のまま、使い方を減らす（mview の staging の解放、他の app の upload の host-visible の削減）。BUG-144 の測定の数字で効き目を見る。

この Queue で入れたこと: hostmem の値を 1 か所（`plan/tools/guest/venus-hostmem.sh`、`VENUS_HOSTMEM`、既定 256M、環境で上書き）に集め、9 か所の
試験（zdesktop-guest・volume-guest・files-guest-p1・rtl-guest・venus-session-check・gles/venus・noct/g3-venus・venus-qemu.py・aperture-bug144 の注釈）を
それに揃えた。値は 256M のまま（上の理由）。Windows の配布物 `tools/release/kei-nightly/README.txt` も `hostmem=256M`（報告だけ、変えていない）。

## 案 3 の実装 1（2026-10-03、P1、Q1 の指示）

- `userland/tests/mview/renderer.c`: staging buffer（host-visible、4〜5.3 MiB、一生 map）を model の upload の後に解放する（各 upload は submit を待つので
  GPU の使用は終わっている）。mview 1 個あたり aperture を 4〜5 MiB 返す。build（config/ci、`bin/mview`）warning 0。
- 他の app（libkeiui の present）: 窓の大きさの host-visible の canvas（毎 frame の CPU の描画の upload に要る）と 6 頂点の buffer（page 1 枚）だけで、
  減らせる明らかな物は無い。aperture の割り当て（first-fit、compaction 無し）を best-fit にするかは、T1-027 の数字（断片化の大きさ、largest_hole）を見て決める。

## 案 1 の実装（2026-10-03、P1、user「HALの変更と言っても、インタフェースでなく実装ですよね。それなら修正してOKです。」、Q1 経由）

`include/hal/hal.h` は変えていない（`hal_space_map_device` の意味・引数は同じ、仮想 address の配置だけ）。

- `src/hal/amd64/space.c`: kernel の device の窓を、共有の MMIO の directory（`system_mmio_pd` の 256〜511、0xffffffffe0000000、512 MiB）から、
  専用の 2 枚の PD（`system_device_pd[2]`）を持つ kernel の PDPT の 507・508 番（0xfffffffec0000000〜0xffffffff40000000、**2 GiB**、ACPI の窓 509 番の直前）
  へ移した。`AMD64_DEVICE_PD_COUNT` は 1024、`device_leaf_tables` も 1024 個（leaf table は今まで通り使う所だけ確保）。窓の位置と ACPI の窓との境は
  `_Static_assert` で固定。PML4 511 の PDPT は全 process で共有なので、新しい PDPT の entry は fork 済みの space にも見える（`build_ram_map` の
  256〜510 の空 PDPT と同じ理由）。`system_mmio_pd` の 256〜511 は空く。
- 他の platform: device の窓の定数は amd64 の `space.c` だけ（arm64 の `space.c` に同じ物は無い）、Venus の driver は amd64 だけ（`platform/amd64/vmunix.mk`）。
  影響は無い。
- `src/drivers/gpu/venus/internal.h`: `VENUS_MAX_APERTURE_BYTES` を 1 GiB に（aperture の allocator は全て uint64）。
- `plan/tools/guest/venus-hostmem.sh`: 既定を 1G に（注釈も直した）。9 か所の試験はこの値を使う。

確認: kernel の build（`ZEDBSD_CONFIG=plan/ws089/tests/config-amd64-settings.mk`、vmunix）は warning 0。HAL の space の host 試験は無い。
QEMU の boot-test（HAL の変更なので必須）と aperture-bug144.sh（hostmem 1G と 256M で mview を何個開けるか）は試験の担当に予約した（結果待ち）。実機は未実施。

## aperture-bug144.sh の直し（2026-10-03、P1、T1-027 で数字が取れなかったため）

T1-027（CI の image）で数字が取れなかった理由は 2 つ:
- CI の image には 10-02 から mview が入っていない。
- zedBSD の ps は rss を持たない（`-o pid,rss,vsz,args` は usage で断られる）。

直したこと:
- image は mview のある `plan/ws035/tests/build-zdesktop-image.sh`（config-amd64-zdesktop.mk）を使う。guest に `/bin/mview` が無ければ `MISSING` と出して止まる。
- CPU の memory は、各段の process の VSZ（`ps -o pid,vsz,args`、KiB）と、system 全体の確保済みの物理 memory（`/sbin/sysctl hw.memory.stats` の `allocated=`）で記録する。段の差がその段の重さになる。zedBSD には process ごとの RSS が無い。
- App Home の全 app の段（1）は `APPHOME=1` の時だけ流す（T1-027 で CI の image で済んでいる）。

T2 の 1 回目（b4b4f5d66）で分かった 2 点を直した（2026-10-03）:
- `files-guest.sh start` の直後に流すと、SSH の準備の前に `MISSING /bin/mview` と誤って止まった。今は先に `guest.py wait --timeout 600` で SSH を待つ。
- `vsz_kib` が全行 `gone` だった。zedBSD の ps は起動の引数（`--token=mN`）を出さないので、grep で見つからなかった。今は mview を起動した shell の `$!` で pid を取り、`ps -A -o pid,vsz` からその pid の行を読む。
T2 の数字（allocated、QEMU）は BUG-144 に Q1 が記録した。
