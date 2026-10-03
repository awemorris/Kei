# WS134 設計: システムモニター（System Monitor、Analytic Spatial UI）

ws134-p001（q649、P2、2026-10-03）。目標は [ws.md](ws.md)、コンセプトは [design/user-concept.md](design/user-concept.md)、参考の画像は
[design/reference-image.webp](design/reference-image.webp)。画面の配置の図は [design/layout-mock.png](design/layout-mock.png)
（`design/layout-mock.py` が描く。配置と色の token だけで、3D・光は無い。状態の chip と hostname の行は窓の中の上段の左に置く（§2.1）が、図には描いていない）。

前提の user の指示（2026-10-03）:

- 「イメージの通りじゃなくてよくて、要素を採用してほしいです。」→ 画像の再現ではなく要素とコンセプトを取り入れる。
- 「P2はとりあえずOSから取れない情報はスタブデータでそれっぽいアニメーションを表示しましょう。」→ OS から今取れない値は
  stub のデータで先に画面と動きを作る（§1.5）。
- 「P2はコードを書いてOKだと思います。衝突しないです。」→ この設計の review の後に実装へ進む（Phase の ID は Q1）。
- Q1（同日）: OS から情報を取る所は WS131 の分け方（compositor の中の `kl_backend_*` と libkeiland の公開 API）に合わせ、
  WS131 と重なる所を明記する（§1.4）。

---

## 0. 名前と置き場所

| 項目 | 決定 |
| --- | --- |
| program | `userland/desktop/monitor/`、package `monitor`、`/bin/monitor`（`ZEDBSD_USERLAND_PACKAGE` の program、desktop、amd64）。Linux・FreeBSD は `Makefile.linux`・`Makefile.freebsd` |
| 画面の名前 | 窓の題と App Home は「System Monitor」。app id `monitor`。OS の名前は画面に出さない（hostname は出す） |
| 記号の接頭辞 | `sm_`・`SM_`。log の接頭辞 `ZMON`（`ZMON READY`・`ZMON SAMPLE`・`ZMON FRAME`・`ZMON DONE`、試験が読む） |
| 起動 | `monitor [--source=sim\|system\|replay:FILE] [--period-ms=N] [--fps=N] [--size=WxH] [--timeout-s=N] [--token=T]`。既定は `--source=auto`（§1.5） |
| App Home | `apps.conf` に `System Monitor\|/bin/monitor\|monitor system activity cpu gpu memory network disk\|4f9d8f\|monitor`（Linux・FreeBSD の `apps.conf.in`、demo の `plan/ws035/demo/apps.conf` にも）。絵 `monitor` は compositor の `icons.c` に足す（無ければ頭文字の tile、後の任意） |
| link | desktop の app は `platform/amd64/vmunix.mk` に個別の動的 link の規則（Notes と同じ: libvulkan・libwayland-client・libkeiland・libkeiui・libtruetype・libc、`vmunix.mk:756` の filter-out に足す） |

---

## 1. 情報の出どころ

### 1.1 一覧（zedBSD は 2026-10-03 の main `f12306a6d` の読み、Linux・FreeBSD は一般の知識と Linux の host の確認）

記号: ○ 取れる、△ 一部・近い値、× 無い（追加の案は §1.3）。

| 情報 | zedBSD（今） | Linux | FreeBSD |
| --- | --- | --- | --- |
| CPU の数 | ○ sysctl `hw.ncpu`（`include/uapi/sysctl.h:40`、`src/kern/sysctl.c:185`） | `sysconf(_SC_NPROCESSORS_ONLN)`・`/proc/stat` の cpuN の行 | `hw.ncpu` |
| CPU 全体と core ごとの使用率 | × per-CPU の時間が無い（`sched_cpu`、`src/kern/sched.c:113-132`。tick の課金 `:885-897` は idle と process0 を課金しない） | `/proc/stat`（user・nice・system・idle・iowait・irq・softirq・steal の jiffies） | sysctl `kern.cp_times`（CPU ごとの user・nice・sys・intr・idle） |
| CPU の周波数 | × HAL の中だけ（TSC の較正 `src/hal/amd64/bsp-pcat/timecounter*.c`）、MSR の API が無い | `/sys/devices/system/cpu/cpuN/cpufreq/scaling_cur_freq` | `dev.cpu.N.freq` |
| load average | × | `/proc/loadavg` | `getloadavg()`・`vm.loadavg` |
| memory の total・free・使用 | ○ `/dev/system` の `KERN_SYSTEM_GET_VMSTAT`（`struct vm_statistics`、`include/uapi/system.h:44-55`、`system-device.c:439-510`。physical は byte）。`/dev/system` は 0666（`src/kern/devfs.c:404-410`） | `/proc/meminfo`（MemTotal・MemFree・MemAvailable・Cached・Buffers・SwapTotal・SwapFree） | `vm.stats.vm.v_page_count`・`v_free_count`・`v_inactive_count`・`v_wire_count`・`v_active_count`、`hw.pagesize` |
| page cache・buffer cache | ○ `vm_file`（数、page と推定）、sysctl `vfs.bufcache.current_bytes`・`vfs.cache_memory.stats` | Cached・Buffers | `vfs.bufspace`、inactive |
| Available | △ `vm_commit_available`（commit の会計、Linux の MemAvailable とは違う）。表示は free + 捨てられる cache（clean の file）で近似 | MemAvailable | free + inactive + laundry の近似 |
| swap の total・used | ○ `vm_statistics.swap_total`・`swap_free`（4096 byte の page の数、`system.h:24`） | SwapTotal・SwapFree | `vm.swap_info`（`kvm_getswapinfo` の sysctl 版） |
| network の interface ごとの RX/TX | ○ `SIOCGIFSTATS`（`struct if_data`、u64 の byte・packet、`include/uapi/netif.h:26-37`）。libkeiland の `keiland_network_get_links`（`received_bytes`・`sent_bytes`、`keiland.h:735-747`）も既にこれを出す | `/sys/class/net/IF/statistics/rx_bytes` 等、`getifaddrs` の `rtnl_link_stats` | `getifaddrs` の AF_LINK の `if_data`（`ifi_ibytes`・`ifi_obytes`） |
| link の速さ | × （`net_device` に速さが無い） | `/sys/class/net/IF/speed` | `if_data.ifi_baudrate` |
| disk ごとの読み書き（byte・回数） | × `struct disk`（`include/kern/disk.h:89`）に counter が無い。全体の `vfs.io.stats`（`IO_COMPLETE_READ`・`WRITE` の calls・bytes、`include/uapi/io-stats.h`、`src/kern/disk.c:1658-1666`）だけ | `/proc/diskstats`（読み書きの回数・sector・ms、io_ticks） | `kern.devstat.all`（`struct devstat`、libdevstat） |
| disk の latency・busy | × | `/proc/diskstats` の ms の列（Δms/Δops） | devstat の duration |
| disk の名前と種類 | △ `/dev` の `BLKGETINFO`（`include/uapi/block.h:29-42`、名前・大きさ・flags。種類（NVMe・USB・virtio）は無い） | `/sys/block/*`（`device/` の subsystem、`queue/rotational`） | devstat の `device_name`・`device_type` |
| GPU の名前・driver | ○ Vulkan の `vkGetPhysicalDeviceProperties`（compositor の device）。`/dev/gpuN` の `GPU_GET_INFO`（`include/uapi/gpu.h:46,148-155`）もあるが、compositor の中の backend からは Guardrail により使わない（§1.2） | DRM の `/sys/class/drm/cardN/device/{vendor,device}`、Vulkan | 同左、Vulkan |
| GPU の使用率・VRAM・温度・電力 | × i915 は engine の busy と RPS の周波数を中で数えている（`src/drivers/gpu/i915/gt-power.c:611,648,681`）が出さない。Venus は何も無い | amdgpu: `gpu_busy_percent`・`mem_info_vram_used/total`・hwmon の `temp1_input`・`power1_average`。i915: `gt_cur_freq_mhz`、busy は perf の PMU（権限が要る）。NVIDIA は NVML（対象外） | drm-kmod の sysctl は少ない（ほぼ ×） |
| 温度（CPU・thermal zone） | × 構造の interface が無い。`/dev/acpi`（0666、`src/drivers/acpi/acpi-dev.c`）に `\_TZ.X._TMP` を書いて読む text の評価だけ。MSR の熱の register は HAL の API が無い | `/sys/class/hwmon/*/temp*_input`、`/sys/class/thermal/thermal_zone*/temp` | `dev.cpu.N.temperature`（coretemp・amdtemp）、`hw.acpi.thermal.tzN.temperature` |
| 電池 | × （system bar の電池は描いた絵、`wayland/shell.c:3096-3122`。WS131 p005 の電源の state も今は全 OS で unsupported） | `/sys/class/power_supply/BAT*` | `hw.acpi.battery.life`・`state` |
| uptime・hostname | ○ `CLOCK_MONOTONIC`、`kern.hostname`（`gethostname`） | 同左 | 同左 |
| OS の版 | △ `uname` は libc の固定の文字（`userland/base/libc/posix.c:5295-5330`） | `uname` | `uname` |

QEMU の Venus の guest の特記: GPU は virtio-gpu（Venus、host の lavapipe）で、使用率・VRAM・温度・電力は guest から見えない（host の GPU の値で、
virtio に経路が無い）。thermal zone・電池は QEMU の q35 の既定の ACPI に無い。これらは QEMU では本物の値が出ない（§1.5 の stub か「—」）。

### 1.2 zedBSD の kernel の追加の案（HAL の API を変えない物）

| 記号 | 追加 | 形 | 大きさの見込み |
| --- | --- | --- | --- |
| K1（p005） | CPU ごとの時間 | sysctl `hw.cputimes`（CTL_HW の 2 段の新しい leaf。`kern_sysctl` の CTL_HW は 2 段だけを扱う、`src/kern/sysctl.c:185-231`）→ header `struct cpu_times_header { uint32_t version, struct_size, element_size, count, hz, reserved; }` と要素 `struct cpu_times_entry { uint64_t user, system, idle, other; }`（幅の固定、明示の padding、ILP32・LP64 の `_Static_assert`、`include/uapi/system.h:115-173` の作法）。tick の処理（`sched.c:885` の周り）で今の CPU の state を見て 1 を足す: idle の thread → idle、process0・kernel の thread → system、user の thread → user/system（今の `accounting_kernel_depth`）、`state != THREAD_RUNNING` の時 → other。**interrupt は取らない**（tick は HAL の timer の割り込みから文脈なしで呼ばれ、IRQ の入れ子を kernel が数えない（`src/hal/amd64/irq.c:539-561`）。取るには HAL の API が要る（review 4）。user の thread を割り込んだ IRQ と page fault は user に数わる、と文書にする）。書く側も読む側も `atomic_u64`（i386 の u64 は atomic でない）。offline の CPU は tick が無く 0 のまま。読み手は `ENOMEM` で大きさを取り直す | kernel 1 file + sysctl.c + uapi + sysctl の CLI の表示。1 Phase（4h、SMP の試験を含む） |
| K2（p006） | disk ごとの統計 | sysctl `hw.diskstats`（2 段の leaf）→ header（K1 と同じ形）と要素 `struct disk_stats_entry { char name[32]; uint32_t kind, flags; uint64_t id, generation, read_ops, write_ops, read_bytes, write_bytes, read_ns, write_ns, busy_ns; uint32_t inflight, reserved; }`（全 u64 の前で 8 の整列、明示の padding、`_Static_assert` で ILP32 と LP64 の大きさを一致）。**対象は物理の whole disk だけ**（`d_parent == NULL`、loop・ufs の内部の disk を除く。`disk_create` の 13 か所のうち partition・loop・ufs を外す、review 12）。`disk.c` の完了の経路（今 `vfs.io.stats` を数える `:1658-1666`）で disk ごとに足す。時刻は HAL の既存の `hal_rtc_read_counter`（`include/hal/hal.h:438-450`、API の変更なし）、使えない arch では tick。`read_ns`・`write_ns` は開始から完了までの和（Δns/Δops が平均の latency）、`busy_ns` は inflight が 0 でない時間。`kind` は登録する driver が渡す（NVMe・USB の mass storage・UAS・IDE・SD/MMC・その他。今の tree に virtio-blk と AHCI の driver は無い）。`id` は disk の作られた順の番号、`generation` は抜き差しで変わる | 1 Phase（4h、kind の登録は実際の driver の数だけ） |
| K3（p007） | GPU の telemetry | **sysctl** `hw.gputelemetry`（2 段の leaf、header と GPU ごとの要素 `{ char driver[16]; uint32_t valid, reserved; uint64_t time_ns, busy_ns, objects_bytes, objects_limit; uint32_t cur_mhz, req_mhz, min_mhz, max_mhz; }`）。**GPU の UAPI（`/dev/gpuN` の ioctl）にしない**: Guardrail「compositor は libvulkan だけを使い、GPU の UAPI を ioctl で直接呼ばない」（`plan/guardrail.md:23-33`）のため、backend（compositor の中）は sysctl を読む（review 3）。i915 は `gt-power.c` の busy の和（`:648-666`）に、読む時に実行中の分（`now - busy_since`）を足す（長い request の間 0 に見えないように）。engine ごとの値は今の計上に無いので出さない（全 engine の和だけ）。Venus・他の driver は要素を出さない（`valid` 0） | 1 Phase（4h） |
| K4（p009） | ACPI の温度・電池 | kernel の ACPI の driver が thermal zone の `_TMP` と電池の `_BST`・`_BIF/_BIX`・AC の `_PSR` を数秒ごとに評価して覚え、sysctl `hw.acpi.thermal`・`hw.acpi.battery` で出す（user が `/dev/acpi` の text の評価をしない）。**WS131 p005（電源）と重なる**: 電池は WS131 の power の state（`kl_backend_power_get_state`）の出どころにもなる | 1〜2 Phase（実機（5330）が要る。QEMU の q35 には zone も電池も無い） |
| K5（任意） | link の速さ | 新しい ioctl `SIOCGIFLINK` → `{ uint64_t speed_bps; uint32_t carrier, duplex; }`（`if_data` の layout は変えない） | 小。画面は速さが無くても自動の目盛りで描ける（§3.6）ので後回し |

HAL の API の変更が要る物（**この WS では行わない**、要るなら user の差分ごとの承認）: CPU の周波数（APERF/MPERF・`IA32_PERF_STATUS`）、
CPU package の温度（`IA32_PACKAGE_THERM_STATUS`）、電力（RAPL の MSR）、割り込みの時間（K1 の interrupt）。MSR を読む HAL の公開の API と、
割り込まれた文脈を kernel に渡す API が無いため。これらは stub のまま（§1.5）か、出さない。

GPU の名前は backend が sysctl ではなく compositor の Vulkan の `VkPhysicalDeviceProperties`（compositor は既に Vulkan の device を持つ）から取る
（`GPU_GET_INFO` の ioctl は Guardrail に反する、review 3）。

memory の `KERN_SYSTEM_GET_VMSTAT` は重い: `vm_reclaim_get_stats`（`src/kern/vm.c:5580-5634`）が reclaim の lock を持ったまま全 backing を走査する
（review 6）。backend は memory を 5 秒に 1 回だけ読み、p008(a) の前に所要時間を測る probe を入れる。重ければ O(1) の counter（`vm_resident`・
`vm_file`・`vm_clean` の加算を保つ）を kernel の追加に足す（K6、要れば）。Available の式は `free + vfs.cache_memory.stats の file data の捨てられる分`
（`include/uapi/cache-memory.h:31-46`）にする（`vm_clean` は anon と file の両方を数え、object store の page を含まないので使わない、review 21）。

### 1.3 Keiland の中の経路（WS131 の構成に合わせる）

docs/architecture/keiland.md と plan/ws131/design.md の構成: app は libkeiland の公開 API だけで system に触れ、OS の差は compositor の中の
`libkeiland-backend`（`kl_backend_*`）が吸収し、app には compositor の拡張（`kl_system_manager_v1` と子の object）で届く（WS131 §4）。
システムモニターの値もこの形にする:

```
 kernel（sysctl・/dev/system・SIOCGIFSTATS・/dev/gpuN）／ Linux（/proc・/sys）／ FreeBSD（sysctl・devstat）
        │ libkeiland-backend-<os>/monitor-<os>.c   kl_backend_monitor_sample()（新しい領域「monitor」）
        ▼
 compositor（wayland/monitor.c）: 購読している client がいる間だけ worker の thread で周期の sample
        │ kl_system_monitor_v1（kl_system_manager_v1 の get_monitor、manager の version を一つ上げる）
        ▼
 libkeiland（kl_system_monitor_*）: 累計の counter から率・% を作る、履歴は持たない
        ▼
 monitor（app）の data source の層（§1.5）: system・sim・replay
```

**backend の領域「monitor」**（WS131 の 7 領域に足す 8 つ目、OS ごとの file）:

```c
struct kl_backend_monitor_info {	/* 変わらない物。起動と hot plug の時 */
	unsigned cpu_count;
	char host[64];
	unsigned gpu_count, disk_count, link_count;
	struct { uint64_t id, generation; char name[48]; char driver[16]; unsigned kind; } gpu[KL_MONITOR_GPU_MAX];	/* 4 */
	struct { uint64_t id, generation; char name[32]; unsigned kind; uint64_t size_bytes; } disk[KL_MONITOR_DISK_MAX];	/* 8 */
	struct { uint64_t id, generation; char name[16]; } link[KL_MONITOR_LINK_MAX];	/* 16、loopback を除く */
};
struct kl_backend_monitor_sample {	/* 累計の counter と今の値。valid の bit が出せた field */
	uint64_t time_ns;			/* CLOCK_MONOTONIC */
	uint64_t valid;				/* KL_MONITOR_HAVE_CPU_TIMES・_MEMORY・_SWAP・_LINKS・_DISKS・_GPU_BUSY・_GPU_MEMORY・_GPU_FREQ・_TEMPERATURE・_POWER・_BATTERY … */
	struct { uint64_t user, system, idle, other; } cpu[KL_MONITOR_CPU_MAX];	/* 256、tick（cpu_hz で割る） */
	uint64_t cpu_hz;
	uint64_t memory_total, memory_free, memory_cache, memory_reclaimable, swap_total, swap_used;	/* byte */
	struct { uint64_t id, rx_bytes, tx_bytes; unsigned up; } link[KL_MONITOR_LINK_MAX];	/* id で info と対応 */
	struct { uint64_t id, read_ops, write_ops, read_bytes, write_bytes, read_ns, write_ns, busy_ns; } disk[KL_MONITOR_DISK_MAX];
	struct { uint64_t id, busy_ns, memory_used, memory_total; unsigned cur_mhz, max_mhz; int milli_celsius; unsigned milli_watts; } gpu[KL_MONITOR_GPU_MAX];
	int cpu_milli_celsius;
};
struct kl_backend_monitor *kl_backend_monitor_open(void);	/* 実在する network 領域（keiland-backend.h:200）と同じ形、review 8 */
int kl_backend_monitor_info(struct kl_backend_monitor *monitor, struct kl_backend_monitor_info *info);
int kl_backend_monitor_sample(struct kl_backend_monitor *monitor, struct kl_backend_monitor_sample *sample);
void kl_backend_monitor_close(struct kl_backend_monitor *monitor);
```

- 上限は network 領域と揃えて link 16（`KL_BACKEND_NETWORK_LINKS_MAX`、`keiland-backend.h:254`）、loopback は除く。disk 8、GPU 4、CPU 256。
- 機器は info の中で変わらない `id` と `generation` を持ち、sample の要素も `id` で対応させる（配列の位置で対応させない）。client は info が変わった
  機器の前の値を捨て、差が負（counter の巻き戻り・挿し直し）なら「reset」としてその間の率を出さない。kernel の元が u32 の field（`vm_reclaim_stats`、
  `include/kern/vm-reclaim.h:22-36`）は幅を header に書く（review 9）。
- **thread**: monitor の関数は `struct kl_backend` の状態に触れず、どの thread から呼んでもよい（header に明記）。compositor は **monitor 専用の
  thread** で sample し（WS131 の設定の store の worker と共用しない。disk の書きで sample の時刻が揺れ、VMSTAT の走査で flush が遅れるのを避ける）、
  結果を pipe で event loop に渡し、emit は event loop の thread だけで行う（review 7）。

**compositor の拡張**（WS131 §4.1 の共通の約束に従う）: `kl_system_manager_v1.get_monitor(new_id, period_ms)`（250〜10000、既定 1000）。
manager の version と opcode は Q1 が予約する（WS131 §4.2 は version 2 を WS113 の `get_displays` に充てる。monitor は 3 の案、review 10）。
`capabilities` の bit に monitor を足す。object `kl_system_monitor_v1`:
- event `device(kind, id, generation, name, size_hi, size_lo)`（機器ごとに 1 つ、作った時と抜き差しの時、`device_done(serial)` で区切る）・`removed(kind, id)`。
- sample の event `cpu(index, user_hi, user_lo, system_hi, system_lo, idle_hi, idle_lo, other_hi, other_lo)`・`memory(…)`・`link(id, …)`・`disk(id, …)`・
  `gpu(id, …)` → `sample_done(serial, time_hi, time_lo, valid_hi, valid_lo)`。Wayland の wire に 64 bit の整数が無いので u64 は hi・lo の 2 つの uint
  （review 10）。
- request `ack(serial)`・`set_period(ms)`・`destroy`。
- **流量の制御**（review 2）: compositor は client が前の sample の `ack` を返すまで次の sample を送らない（間に取った sample は捨てる。値は累計の
  counter なので率は次の sample で正しい）。sample は「全部か無しか」: emit の前に未送信の量（`zwl_client.output_bytes`、上限 `ZWL_OUTPUT_MAX` 1 MiB、
  `wire.c:115-121`）を見て、1 sample 分（最大 約 20 KB）が入らなければその sample を丸ごと飛ばす。app が止まっても compositor の送信の queue は
  1 sample より伸びず、他の event（frame callback・configure・titlebar）を押し出さない。
- 購読が 1 つ以上ある間だけ sample し、最小の周期は購読の中の最小、上限 4 Hz（250 ms）。memory は 5 秒ごと（review 6）。購読が無くなれば止める。
- 値は累計の counter（率は client が作る）。
- 認可は WS131 §4.1 の 5（compositor と同じ uid の client だけ）。process の一覧は出さない（範囲の外）。

**libkeiland の API**（WS131 §4.4 の `kl_system_*` に足す）:

```c
struct kl_system_monitor *kl_system_monitor_open(struct kl_system *system, unsigned period_ms);	/* 無ければ NULL・ENOTSUP */
int kl_system_monitor_take(struct kl_system_monitor *monitor, struct kl_monitor_frame *frame);	/* 新しい sample があれば 1 */
const struct kl_monitor_info *kl_system_monitor_info(const struct kl_system_monitor *monitor);
void kl_system_monitor_close(struct kl_system_monitor *monitor);
/* kl_monitor_frame: 前の sample との差から作った率（CPU %（全体・core ごと）、byte/s、ops/s、平均の latency（ms）、GPU %）と今の値、valid の bit */
```

### 1.4 WS131 と重なる所（明記）

| 重なり | WS131 の Phase | この WS の扱い |
| --- | --- | --- |
| backend の新しい領域（monitor） | p003（network は移動済み、`libkeiland-backend*/` は既に tree にある）〜p009 は 7 領域を 1 つずつ移す。monitor は WS131 に無い | WS131 の p009（GPU の buffer）までの統合の後に、この WS の p008（M3）が `libkeiland-backend-<os>/monitor-<os>.c` を足す。WS131 の命名（`kl_backend_*`）と checker（B1〜B3）に従う |
| app の移行と旧名の除去 | p016〜p020（app の移行）、p023（互換の名前の除去、B5） | **Q1 に依頼**: WS131 の app の移行の一覧と p023 の前提に monitor を足す（足さないと p023 の B5 で monitor の旧名が FAIL する、review 17） |
| path の予約 | WS131 の担当が `platform/amd64/vmunix.mk`・`wayland/apps.conf`・`wayland/icons.c` などを触る | monitor の p002 がこれらに行を足す時は Q1 が順を決める（小さな追加、衝突は merge で解く） |
| `kl_system_manager_v1` と `kl_system_*` | p010（拡張と client の API） | p008 は **WS131 p010 の統合の後**に始める。`get_monitor` で manager の version を上げる（WS113 の `get_displays` と同じ手順、WS131 §4.5）。WS131 の作業中の P1/P3 と file が重なる（`wayland/` の拡張の表・`libkeiland/system*.c`）ので、開始の前に Q1 が調整する |
| network の link の byte | p003・p010（`kl_system_network_v1` の `link`） | monitor は sample の時刻をそろえるため link の counter を自分で持つ。backend の中では network 領域の link の読みの関数を共有し、二重に実装しない |
| 電池・AC | p005（`kl_backend_power_get_state`、今は全 OS で unsupported） | monitor は電池を自分で読まない。表示するなら `kl_system_power_v1` の state を使う。電池の本物の値は K4（kernel）と WS131 p005 の後 |
| app の骨組み（`kl_app`・`kl_window_vulkan_surface`） | p015 | この app は p015 を待たない。今の libkeiui の窓（`KUI_PRESENT_NONE` と自前の Vulkan、Notes と同じ形）で作り、p016〜p020 の app の移行の一つとして後で `kl_app` に移す |
| 名前の変更（`kui_`→`kl_`、`keiland_`→`kl_`） | p013・p014 | 今の名前で書く。WS131 の改名の Phase が tree の全体を機械的に直す（rename-map の対象に入る） |

WS131 p010 の前の期間の app の値は、`--source=sim`（全部 stub）か、p008 の前の暫定の data source（§1.5 の `direct`）の、どちらにするかを
D1 で決める。**提案: direct は作らない。** WS131 の構成（app は OS に直接触れない）と矛盾し、後で消す code になる。p010 の前は sim で画面と動きを
完成させ、kernel の K1・K2 は先に入れて、`sysctl`（CLI）と host の試験で確かめておく。

### 1.5 data source の層と stub（2026-10-03 user「OSから取れない情報はスタブデータでそれっぽいアニメーションを」）

app の中の 1 つの層 `source.c`（`struct sm_source`）が、画面の側に同じ形の `struct sm_frame`（§1.3 の `kl_monitor_frame` と同じ field と
`valid`・`simulated` の bit）を周期で渡す。画面の code は値の出どころを知らない。

| source | 中身 | いつ |
| --- | --- | --- |
| `sim` | 全部の値を stub の model（§1.6）で作る | 既定（system が無い時）・試験・デモの image |
| `system` | `kl_system_monitor_*`（§1.3）。`valid` に無い field だけを sim の model が埋め、`simulated` の bit を立てる。**simulated の field は警告の段・Events・状態コアの全体の判定に使わない**（実機で偽の警告を出さない、review 5）。閾値を越えさせる注入（§1.6）は source が sim の時だけ | WS131 p010 と p008 の後の既定 |
| `replay:FILE` | 記録した frame の列（1 行 1 frame の text）を時刻どおりに流す。試験の決定性のため（同じ入力で同じ画面） | 試験 |

`--source=auto`: system が開ければ system、開けなければ sim。

**stub の印は画面に付けない**（2026-10-03 user「simという表記はつけなくていいです。私がどの項目がスタブか把握できていればいいです。」）。
代わりに下の表で stub の項目と、本物に替える時に要る OS・libkeiland の API を管理し、実装の各 Phase の報告に最新の表を添える。app の中では
`struct sm_frame` の `simulated` の bit と `ZMON SAMPLE … simulated=0x…` の log で区別できる（試験と調べのため）。

**stub の項目の一覧**（2026-10-03 の設計の時点。替える Phase は §5）:

| 画面の項目 | 今（zedBSD） | 本物にするのに要る OS の API | 要る libkeiland・backend の API | 替える Phase |
| --- | --- | --- | --- | --- |
| CPU 全体・core ごとの %（サマリー・タイル面・コアの上層） | stub | K1 `hw.cputimes`（新）。Linux `/proc/stat`、FreeBSD `kern.cp_times` は今ある | `kl_backend_monitor_sample` の `cpu[]`、`kl_system_monitor_v1` の `cpu` event、`kl_system_monitor_take` | p005 + p008 |
| CPU の周波数（CPU の plate の補助の数字） | stub | HAL の MSR の API（D3、user の承認）。Linux `scaling_cur_freq`、FreeBSD `dev.cpu.N.freq` | sample に `cpu_mhz` を足す | HAL の承認の後 |
| memory の Used・Cache・Available・Swap（サマリー・Memory の層・コアの内部の密度） | stub（WS131 p010 の前） | 今ある: `/dev/system` の `KERN_SYSTEM_GET_VMSTAT`、`vfs.bufcache`。Linux `/proc/meminfo`、FreeBSD `vm.stats.vm.*` | sample の `memory_*`・`swap_*`、`kl_system_monitor_v1` の `memory` event | p008 |
| network の RX/TX（サマリー・流れ・コアの周りの線） | stub（WS131 p010 の前） | 今ある: `SIOCGIFSTATS`。Linux `/sys/class/net`、FreeBSD `getifaddrs` | sample の `link[]`（backend の network 領域の link の読みを共有） | p008 |
| network の link の速さ（目盛りの上限の線） | 出さない（自動の目盛り） | K5 `SIOCGIFLINK`（新、任意）。Linux `/sys/class/net/IF/speed`、FreeBSD `ifi_baudrate` | info に `speed_bps` | 任意 |
| disk の読み・書き（byte/s・ops/s）（サマリー・流れ・コアの下のリング） | stub | 全体の和は今ある `vfs.io.stats`（`IO_COMPLETE_READ`・`WRITE`）、disk ごとは K2 `hw.diskstats`（新）。Linux `/proc/diskstats`、FreeBSD devstat | sample の `disk[]`、info の `disk[]` | p006 + p008 |
| disk の latency（Latency の plate・lane の詰まり） | stub | K2 の `read_ns`・`write_ns`（新） | 同上（率の計算は libkeiland） | p006 + p008 |
| GPU の使用率・周波数・memory（GPU の plate・カード・コアの側面） | stub（QEMU の Venus は p008 の後も stub） | K3 `hw.gputelemetry`（新、i915 だけ）。Linux amdgpu・i915 の sysfs、FreeBSD はほぼ無い | sample の `gpu[]`、info の `gpu[]` | p007 + p008 |
| GPU・CPU の温度（GPU カードの熱の gradient・meter） | stub | HAL の MSR の API（D3）か K4 の ACPI の thermal zone。Linux hwmon、FreeBSD coretemp・`hw.acpi.thermal` | sample の `milli_celsius` | p009（K4）か HAL の承認の後 |
| GPU の電力（meter） | stub | HAL の RAPL の MSR（D3）。Linux hwmon `power1_average`（amdgpu） | sample の `milli_watts` | HAL の承認の後 |
| 電池 | 出さない | K4（ACPI の `_BST` 等） | WS131 p005 の `kl_backend_power_get_state`・`kl_system_power_v1` | WS131 p005 + p009（K4） |
| hostname・CPU の数・uptime（title bar） | 本物（今ある `gethostname`・`hw.ncpu`・`CLOCK_MONOTONIC`。app が読むのは libc の標準の関数だけ） | — | p008 の後は info から | — |

### 1.6 stub の model（もっともらしく動く値）

- 各値は `基準 + 遅い揺らぎ（周期 20〜90 秒の正弦の和）+ 速い雑音（1/f に近い、低域の通した乱数）+ ときどきの山（平均 40 秒に 1 回、
  立ち上がり 1〜2 秒・減衰 5〜15 秒）`。値は物理の範囲に clamp する。乱数は `--seed` で固定（試験の決定性）。
- 相関: CPU の山の時は GPU・memory も少し上がる（係数 0.3）。network の山と disk の書きは独立。core ごとは全体の値に core ごとの偏り
  （ある core が熱い、を数十秒ごとに移す）。
- memory: Used は 25〜45% をゆっくり、Cache は Used と逆に少し、Swap はたまに 0 から少し。GPU の温度は使用率の 1 次遅れ（時定数 20 秒）で
  38〜72 ℃、電力は使用率に比例。disk の latency は通常 0.2〜1.5 ms、書きの山で 3〜8 ms（「詰まる」の表現を見せる）。
- 警告の段（§3.7）を見せるため、平均 3 分に 1 回、どれかの値を注意の閾値の上に 20〜40 秒置く（`--sim-calm` で無し）。**source が sim の時だけ**。
- stub は値を補うためのもので、無い機器を作らない（GPU が 1 つの機械に 2 枚目のカードを出さない、disk・link の数は info のまま。sim の source だけは
  `--sim-gpus=N` などで機器の数を決める、review 25）。

---

## 2. 画面の構成

### 2.1 採用する要素と採用しない要素

| 参考の画像・コンセプトの要素 | 扱い |
| --- | --- |
| 上段のサマリーのプレート（CPU・GPU・RAM・VRAM・Network・Disk、値と sparkline） | 採用。VRAM は GPU の plate に入れ 5 枚にする（GPU が無い・stub の機械で枠が空かないように） |
| 中央の状態コア（多層の半透明の立方体とリング、NORMAL の語） | 採用。製品の顔。§3.2 |
| CPU core のタイル面（立体のレリーフ、tap した core の吹き出し） | 採用。§3.3 |
| GPU のモジュールカード（環の使用率、VRAM・温度の bar） | 採用。GPU の数だけ（最大 2 枚を並べ、それ以上は scroll）。§3.4 |
| Network・Disk の波の graph | 「流れ」として採用（帯と粒子）。latency の小さな plate も採用。§3.5・3.6 |
| メモリの層構造のブロック（コンセプト） | 採用（画像には無い）。下段の中央 |
| Recent Events の帯 | 採用。下端の細い帯。出来事は data から作る（§3.8） |
| 上の bar の「System Cockpit」・機械の名前・状態・日時・検索・設定 | 窓の title bar（Keiland の CONTROLS）に: 名前と、時間軸の切替（`KEILAND_CONTROL_GENERIC` を 4 つ、選ばれた物を `set_control_state(checked)`。titlebar の API に segmented・色つきの chip の role は無い、`keiland.h:303-324`、review 14）。**状態の chip と hostname は窓の中**（上段の左の小さな行）に描く。日時は system bar にあるので出さない。検索・設定は置かない |
| 外枠の太いベゼル・強い青の発光・HUD 風の飾り | 採用しない（コンセプトの「光る・スキャンライン・HUD を避ける」） |

### 2.2 層（奥行き）

1 つの空間に 4 つの層の板が浮く。窓（Wayland の surface）は不透明で、app が自分の背景まで描く（compositor の glass は使わない、§4.4）。

| 層 | 中身 | 奥行き（z、world の単位 = px） | 見え方 |
| --- | --- | --- | --- |
| L0 背景の情報層 | 薄い格子・遠い履歴の帯（1 時間の CPU を淡く） | −120 | 最も暗い、視差が大きい |
| L1 詳細 | 中段の 3 枚の周り・下段（Network・Memory・Disk）・Events | −40 | 面は `surface.1` |
| L2 サマリー | 上段の 5 枚、状態コアの plate | 0 | 面は `surface.2`、縁が一番はっきり |
| L3 手前 | 展開した card・警告で前に出た plate | +30〜+60 | 一時的 |

影ではなく層の違い（面の明度・縁の太さ・視差・わずかな縮尺）で区切る。

### 2.3 配置（横長、基準 1280x800 の論理 px。[layout-mock.png](design/layout-mock.png) は 1440x900 で同じ比）

```
 ┌ title bar（zdesktop が描く CONTROLS）: System Monitor · host · 4 CPUs · up 2:14 │ ●Normal │ 1m 5m 15m 1h ┐
 ┌ CPU ─────┐┌ GPU ─────┐┌ Memory ──┐┌ Network ─┐┌ Disk ────┐   L2 サマリー（高さ 116）
 │ 37%  ~~~ ││ 21%  ~~~ ││ 2.1/8GiB ││ 4.2 Mb/s ││ 12 MB/s  │
 └──────────┘└──────────┘└──────────┘└──────────┘└──────────┘
 ┌ CPU cores ────────┐┌ System state ───────────┐┌ Graphics ─────────┐   L1 / L2（中段、高さ 360）
 │ 立体のタイル面     ││   多層のコアとリング     ││ GPU のモジュール   │
 │ Overall/High/Low   ││   NORMAL · 一行の要約   ││ カード × 1〜2      │
 └────────────────────┘└─────────────────────────┘└────────────────────┘
 ┌ Network ─────────────┐┌ Memory ────┐┌ Disk ─────────────┬ Latency ┐   L1（下段、高さ 210）
 │ RX/TX の帯と粒子      ││ 層のブロック ││ Read/Write の 2 本  │ 0.8 ms  │
 └───────────────────────┘└────────────┘└────────────────────┴─────────┘
 ┌ Events: 14:32 … 14:35 … ────────────────────────────────────────────┐   L1（高さ 40）
```

- 余白: 外 24、plate の間 16。角丸は 14（plate）・8（中の部品）・6（tile）。罫線は 1 px（論理）。
- 窓の既定の大きさ 1200x760、最小 900x600。1920x1280（デモの LCD）では同じ配置を 1.5 倍の文字の尺度で。
- 縦長（タブレットを縦に、幅 < 高さ）: 上段 5 枚を 3+2 の 2 行、中段は状態コアを上に大きく、その下に CPU cores と Graphics を並べ、下段を縦に積み、
  全体を縦に scroll。
- 狭い（幅 < 1100）: 上段の sparkline を消し、中段の左右を 1 列に。

### 2.4 色と質感の token（コンセプトの「青みのダークグレー・チャコール〜スレート・シアン／アイスブルー・アンバー・抑えた赤橙」）

| token | 値 | 使う所 |
| --- | --- | --- |
| `bg.deep`・`bg.mid` | `#10151E`・`#181F2B` | 背景の縦の gradient |
| `surface.0`・`.1`・`.2` | `#1B232F`・`#1F2836`・`#26303F` | 層ごとの面（手前ほど明るい） |
| `edge.0`・`.1`・`.2` | `#303D4E`・`#405268`・`#5C7692` | 縁（手前ほど明るく、はっきり） |
| `text`・`text.dim`・`text.faint` | `#D6E0EC`・`#8A9AB0`・`#5E6D82` | 文字（少し青灰） |
| `accent.cyan`・`accent.ice`・`accent.mint` | `#5CC4E6`・`#AAD6F0`・`#6ED6B0` | 通常の値（CPU・memory はシアン、GPU はアイス、network はミント） |
| `warn.amber` | `#E6A84C` | 注意、disk の書き |
| `crit.coral` | `#D8664E` | 危険（抑えた赤橙） |
| 乳白の面 | 面の色に 3% の白の低周波の noise と、内側 1 px の明るい縁（上辺 +8% の明度） | 「薄い乳白セラミック＋半透明樹脂」。本物の blur はしない（§4.4） |

文字: 値 30/24 px の数字は **JetBrains Mono**（等幅で slide の時に幅が揺れない。libtruetype に GSUB が無く Inter の `tnum` は効かない。tree には
Regular だけなので bold は合成、review 20）、題 16 px（Inter）、補助 13 px。最小 12 px（タブレットで小さすぎる文字を避ける）。

---

## 3. 3D と動き

### 3.1 共通の動き（「反応」ではなく「呼吸」）

| 動き | 量 | 速さ |
| --- | --- | --- |
| plate の浮遊（視差） | 層ごとに 1〜4 px（L0 4、L1 2、L2 1、L3 0）。pointer・指の位置で camera を ±1.5° 傾け、触れていない時は周期 12 秒の微弱な揺れ（0.3°） | 慣性: 臨界減衰の spring（固有振動 2 Hz） |
| 数値の更新 | 桁ごとに slide（増えた桁は上へ、減った桁は下へ、4 px）、前の値は 25% の不透明度で 150 ms 残る | 180 ms、ease-out |
| graph | sample の間を Catmull-Rom で補間し、時間軸は連続に流す（1 秒ごとに跳ばない） | 毎 frame |
| 状態の変化だけ強く | 閾値の段の変化（§3.7）、card の展開 | 300〜450 ms |
| 背景の粒子・光 | L0 に淡い点（最大 40、不透明度 ≤ 6%）がゆっくり漂う | 10〜30 px/s |

避ける物: 常時の回転、scanline、六角形・HUD、点滅。動きの量は `--calm`（OS の「動きを減らす」の設定があればそれ）で視差と粒子を止める。

### 3.2 状態コア（中央）

- 形: 3 つの入れ子の角丸の立方体（外 1.0、中 0.68、内 0.38 の大きさ、角丸 8%）を、少し上から見る（camera の仰角 22°、方位 −30°、
  正射影に近い望遠、FOV 18°）。下に 2 本の楕円のリング（台座）と、周りを巡る細い線（最大 6 本）。
- 素材: 外と中は半透明の樹脂（不透明度 0.18〜0.35、fresnel で縁を明るく）、内は内側から淡く光る（emission 0.4〜1.0）。
- 状態との対応（値は 0〜1 に正規化して spring で追う）:

| 状態 | 表現 |
| --- | --- |
| CPU が高い | 外殻の上面と上半分の明度 +0〜40%（縦の gradient の位置が下がる） |
| GPU が高い | 側面の層（中殻）が厚く（大きさ 0.68→0.74）、周期 3 秒で ±1.5% 脈動 |
| memory の圧迫 | 内殻の中の点（instanced の小さな立方体、0〜64 個）が密になる。Available が 10% を切ると点の色がアンバーへ |
| network が多い | 周りの細い線の上を光の点が流れる（速さ ∝ log(byte/s)、本数 ∝ link の数） |
| disk I/O が多い | 下のリングが回る（0〜0.15 回転/秒、∝ log(ops/s)）。書きが多いと 2 本目が逆向き |
| 全体の状態 | Normal: 内の光はシアン。Elevated: 外殻の縁がアイスに強く。Warning: 内の光がアンバー寄り（混合 40%）、コアが L3 へ 10 px 前に。Critical: 抑えた赤橙、脈動が 1.5 秒周期に |

- 意味のない回転はしない。操作（drag）の時だけ方位を ±25° 回せ、離すと spring で戻る。
- 下の文字: 状態の語（NORMAL・ELEVATED・WARNING・CRITICAL）と一行の要約（「CPU 82% for 2 min」など、最も悪い値の説明）。

### 3.3 CPU core のタイル面

- core の数だけの浅い角丸の箱（tile）を、√n に近い格子に並べ、状態コアと同じ camera で斜めに見る（面の傾き 35°）。
- 高さ: 使用率 0→1 で 0→18 px（論理）にせり上がる（spring、200 ms）。色は明度の差（`surface.1` → `accent.cyan` の 70%）。85% を越えた tile
  だけ上の縁が薄く光る（emission 0.3）。
- tap・click した tile は L3 へ浮き、吹き出し（Core 7 · 78% · user/system の内訳 · 1 分の sparkline）を出す。もう一度で戻る。
- 下の行: Overall・Highest（core 番号）・Lowest。core が 32 を越える機械は格子を細かくし、tile の間を 2 px に。

### 3.4 GPU のモジュールカード

- GPU ごとに 1 枚の薄い plate（角丸 10）。左に使用率の環（`kui` の ring と同じ見た目、環の充填 = Util）、右に 3 本の細い meter（Memory・
  Temperature・Power）と周波数の数字。
- Util が上がると card の中の「活性の領域」（環の裏の淡い面）が静かに広がる（半径 ∝ Util、spring 600 ms）。温度は card の地に下から上への
  薄い熱の gradient（38 ℃ で無し、80 ℃ でアンバー 12%）。
- tap で展開（§3.9）: 使用率・周波数・memory の時系列、engine ごとの busy（K3 の後）。

### 3.5 Network の流れ

- 半透明の帯（tube を真横から見た形）が 2 本: RX（シアン、左→右）・TX（ミント、右→左）。帯の太さ = その時の量（対数の目盛り）、帯の中を流れる
  光のパルスの速さ = 量。spike（直前 10 秒の平均の 3 倍）の時だけパルスの密度が上がる。
- 帯の上に時系列の線（時間軸の範囲、§3.10）を薄く重ねる。目盛りは自動（直前の範囲の最大の 1.25 倍を 1・2・5 の段に丸める）。
  link の速さ（K5）があれば上限の線を引く。
- interface が複数なら、上の行に interface の chip（en0・wlan0）、選ぶと切り替え（既定は全部の和）。

### 3.6 Disk の流れ

- 読み（シアン）と書き（アンバー寄りのアイス）を別の lane に。各 lane は薄いパルスが左から右へ流れ、量で密度と明るさ。
- latency が上がると（平均 2 ms 超）lane の右端にパルスが溜まって詰まる（間隔が縮み、少し滞る）。異常（10 ms 超が 5 秒）の時だけ lane の縁が
  アンバー。
- 右に Latency の小さな plate（平均の ms、小さな棒の histogram、Good・Slow・Stalled の語）。disk が複数なら chip で選ぶ（既定は和、latency は最大）。

### 3.7 段階的な警告（コンセプトの 4 段）

| 段 | 条件（既定、全て 10 秒の持続） | 表現 |
| --- | --- | --- |
| 0 通常 | — | — |
| 1 注意の気配 | CPU 全体 > 80%、memory Available < 15%、swap が増え続ける、disk latency > 5 ms、GPU > 90%、温度 > 80 ℃ | 対象の plate の縁に薄いアンバーの輪郭（不透明度 40%） |
| 2 注意 | 1 の条件が 60 秒 | 対象の plate が L3 へ少し前（+20）、関連する graph に発生時刻の縦の guide 線、Events に 1 行、状態コアが Warning |
| 3 危険 | CPU > 95% 120 秒、Available < 5%、swap が 90% 超、latency > 50 ms | 輪郭と状態コアが `crit.coral`、title bar の chip が Critical |

戻る時は 1 段ずつ（各 5 秒の hysteresis）。閾値は `sm_rules` の 1 つの表（後で設定にできる形）。

### 3.8 出来事（Events）

data から作る: 段の変化（上り・下り）、link の up/down、disk・GPU の出入り（info の変化）。stub の field の変化からは作らない（D2、review 5・19）。最大 200 件を覚え、帯には最新の 3 件、
tap で L3 の一覧（時刻・語・値）に展開。

### 3.9 card の展開（tap）

1. tap した plate が L3 へせり出す（z +60、大きさ 1.04、360 ms、spring）。周りは L1 の側へ 2% 後退し、明度 −15%。
2. plate が詳細の大きさ（窓の 70%×60%）へ変形し、中の部品が分かれて並び直る（各部品が自分の位置へ 40 ms ずつずれて動く）。
3. 下から詳細の chart（時間軸の範囲の線 graph、複数の系列）がせり上がる（240 ms）。
4. 外を tap・Esc で逆の順に戻る。長押しで「固定」（ピンの印、外の tap で閉じない。もう一度の長押しで外す）。

### 3.10 操作（タブレットと pointer）

| 操作 | タブレット | pointer・key |
| --- | --- | --- |
| plate・tile・card を浮かせる／展開 | tap | click |
| 詳細の固定 | 長押し（500 ms、libkeiland の gesture の LONG_PRESS） | 右 click の menu の「Pin」か P |
| 時間軸（1 分・5 分・15 分・1 時間） | 水平の swipe（`DRAG_END` の速さで分類、imageview の `touch_swipe` と同じ考え）。graph の上で | wheel（Shift+wheel）、← → |
| 俯瞰（全体）と詳細（選んだ 1 枚）の切替 | 2 本指の pinch（`keiland_gesture_pinch`、縮めると俯瞰、広げると詳細）。2 本指の tap は libkeiland の gesture に無い（2 本目で `MULTI_PRESSED` になり TAP を出さない、`libkeiland/gesture.c:216-222`・`:327-352`）ので、app の側で「2 本の指が 250 ms 以内に下りて、動き 8 px 未満で 300 ms 以内に離れた」を検出する（libkeiland の公開 API は変えない） | Tab で次の plate、Enter で詳細、Esc で俯瞰 |
| コアを回して見る | 状態コアの上の drag | drag |
| 時間軸の切替（title bar） | title bar の segmented の control | 同じ |

- swipe は「1 本の指だけで始まり、1 本の指だけで終わった drag」で、graph の plate の上で始まった物に限る。pinch の間の重心の drag の `DRAG_END` は
  swipe にしない。状態コアの上の drag は回して見る操作で、swipe にしない（review 13）。

履歴: 1 秒の sample を 1 時間（3600）、10 秒の平均を 24 時間（8640）持つ（ring buffer、app を閉じると消える）。持つ field は graph に描く物だけ:
CPU 全体と core ごとの %（u8）、memory の 4 つ（u16 の ‰）、link・disk ごとの byte/s 2 つ（f32）、disk の latency（f32）、GPU ごとの %・memory・温度（u8・u16）。
16 core・2 link・2 disk・1 GPU で 1 sample 約 60 byte、合わせて約 0.8 MiB（review 24）。

---

## 4. 描画の方式と frame の予算

### 4.1 窓と描画

- libkeiui の窓（`kui_window_open`、`KUI_PRESENT_NONE`）で Wayland・xdg-shell・titlebar（WS070 の CONTROLS）・入力（touch を含む）を受け、
  描画は app の Vulkan（`VK_KHR_wayland_surface`・FIFO の swapchain）。Notes（`userland/desktop/notes/window.c:79-81`・`render.c`）と同じ形。
  WS131 p015 の `kl_window_vulkan_surface` が来たらそれに移す（§1.4）。
- 文字: 変わらない label（題・単位・補助の語）は libkeiui の CPU の canvas（`kui_canvas_*`・text）で plate ごとの texture に一度だけ描く（大きさが
  変わる時に描き直す）。**数字は glyph の atlas**（JetBrains Mono の 0-9・`.`・`%`・`/`・空白など、大きさごとに一度 upload）を instanced の quad で描き、
  桁の slide と前の値の残りは shader（各桁の quad に位置と不透明度）で行う。値の更新で texture を upload しない（review 15）。
- 2D の部品（環・meter）と graph の線・帯・sparkline・粒子・3D は GPU（shader）。sparkline も GPU の pass 4 にそろえる。
- 文字の texture を描き直す時は、frame の fence を待ってから書く（Notes と同じく 1 frame を待ってから次、`notes/render.c` の冒頭の方針）。
- shader は GLSL を `shaders/regenerate.py`（Notes と同じ、host の glslang で SPIR-V を生成し `shaders.h` に埋める）。

### 4.2 pass の構成（1 frame）

| 順 | pass | 内容 | 費用の見込み |
| --- | --- | --- | --- |
| 1 | 背景 | 全面の quad: gradient・格子・L0 の淡い履歴の帯・粒子（instanced の点） | 小 |
| 2 | plate | 全 plate を 1 回の instanced の draw（角丸の SDF の quad、縁・内側の明るい縁・乳白の noise・層ごとの tint）。z は層の値で、視差は vertex shader | 小 |
| 3 | 3D | タイル面（core の数の instanced の箱、不透明、depth を書く）を先に。状態コアは半透明の殻を cull で 2 回: 外の背面 → 中の背面 → 内（不透明に近い発光、depth を書く）→ 中の前面 → 外の前面。半透明の draw は depth を読むが書かない。台座のリングと周りの線は殻と交わるので、交わる所の順の誤りは許す（review 22） | 中（頂点は合わせて 5,000 未満） |
| 4 | graph・流れ | sample の履歴を storage buffer に置き、vertex shader で線・帯を作る（CPU で頂点を作らない）。パルスは instanced の quad | 小〜中 |
| 5 | 文字の texture | plate ごとの texture を premultiplied alpha で重ねる | 小 |
| 6 | 展開と警告 | L3 の plate（展開の card・詳細 chart） | 展開の間だけ |

MSAA は使わない（Notes と同じく sample 1）。縁は SDF の shader で anti-alias、3D の箱は角丸の mesh と fresnel で縁の段差を目立たせない。

### 4.3 frame の予算と速さの段

| 状態 | frame の率 | 理由 |
| --- | --- | --- |
| 操作中・展開・警告の遷移 | 60 fps（FIFO） | 手に付いてくる |
| 呼吸（触れていない、見えている） | 既定 30 fps、`--fps` で変える。QEMU の Venus（lavapipe）では 20 fps を推奨 | 常時のわずかな動きと連続の graph。compositor の負荷を抑える |
| 静止（`--calm`、または 30 秒操作が無く値の変化が小さい） | sample ごと（1 fps）＋値の変化の 180 ms の遷移だけ | 電池・発熱 |
| 隠れている（最小化・他の desktop） | 0（描かない、sample の受け取りだけ続ける） | 下の「frame の許可」 |

**frame の許可（review 1）**: compositor は描いた窓の frame callback だけを完了させ（`compose.c:2050-2061`）、最小化・他の desktop の窓は描かない
（`shell.c:2140-2163`）。libvulkan の FIFO の present は前の frame callback を最大 10 秒待ち、時間切れで `VK_ERROR_SURFACE_LOST_KHR` にして surface は
戻らない（`wsi-wayland.c:24`・`:768-773`）。xdg の `suspended` の状態も compositor は送らない。よって app は:
1. present の前に自分で `wl_surface_frame` を要求し（WSI の分とは別の callback）、**前の自分の callback が届くまで次の frame を描かない**（callback を
   描画の許可にする）。届かない間は描かず、event loop は sample と入力だけを扱う（`kui_window_dispatch` の timeout は sample の周期）。
2. 許可が来ない時間が 1 秒を越えたら「隠れている」とし、log `ZMON VISIBLE 0`。届いたら `ZMON VISIBLE 1` で描き直す。
3. それでも `VK_ERROR_SURFACE_LOST_KHR`・`VK_ERROR_OUT_OF_DATE_KHR` が出たら、VkSurface と swapchain を作り直す（p002 の受け入れに入れる）。

予算（1 frame の GPU の時間）: 5330（i915、1920x1280）で 4 ms 以下、QEMU の Venus（1280x800、lavapipe）で 25 ms 以下。CPU（app）は
frame あたり 2 ms 以下（文字の canvas は値が変わる時だけ）。`ZMON FRAME fps= gpu_ms= cpu_ms=` を 5 秒ごとに log に出し、試験が読む。

### 4.4 compositor の glass を使わない理由

compositor の `keiland_glass` は窓の中の矩形を desktop の上のすりガラスにする（Files・Settings の pane）。この app は奥行き・視差・展開で
plate が動き、z の違う plate が重なるので、2D の矩形の列の glass では表せない。また glass の blur は compositor の毎 frame の費用になる。
よって窓は不透明にし、乳白の質感は app の shader で作る（本物の背景の blur はしない。背景が app 自身の静かな gradient なので、blur と区別が
付かない）。窓の外側（title bar）は Keiland の標準のまま。

---

## 5. 実装の Phase の分け方と受け入れ

ID は Q1 が割り当て済み（2026-10-03）。QEMU の試験は T1/T2 に依頼。実機（5330）の試験は Q1 が user と時間を決める。2026-10-03 user「システムモニターは
私に確認しなくていいので、どんどん実装して動かしてください。」で、p002 から順に実装する。

| Phase | 内容 | 依存 | 受け入れ（QEMU の Venus。log は guest の file を `files-guest.sh run` で読む、console・serial の log は使わない） | 実機 | 見積もり |
| --- | --- | --- | --- | --- | --- |
| p002（M1 骨組み） | package・zedBSD の build（Linux・FreeBSD の Makefile も）・libkeiui の窓（`KUI_PRESENT_NONE`）と自前の Vulkan（FIFO、自前の frame callback の許可、SURFACE_LOST の回復）・title bar（題・時間軸の 4 つの control）・data source の層と **sim**・**replay**・履歴の ring・背景と plate の pass（SDF の角丸、層の tint、視差なし）・label の texture・数字の glyph atlas・sparkline（GPU）・`ZMON` の log・試験の mode・App Home の行 | — | build warning 0（zedBSD。Linux の gcc・clang、FreeBSD は Makefile の同期と host の compile）。host 試験（sim の決定性、replay の読み、率の計算と reset、履歴の ring、警告の段の表）。guest: `ZMON READY`、replay の入力の値が `ZMON TEXT plate=… value=…`（描いた文字）と一致、`--clock=fixed --calm` の PNG が golden と許容の差の内（1280x800、1920x1280）、最小化して戻すと `ZMON VISIBLE 0`→`1` で PNG が更新、`ZMON FRAME` の CPU の時間（fence の待ちを含む）が予算内、`ZMON MEM`（heap と Vulkan の確保の量）が 5 分で増えない | — | 2 日 |
| p003（M2 3D と動き） | 状態コア・CPU のタイル面・GPU のカード・Network と Disk の流れ・Memory の層・視差・数字の slide・段階的な警告・Events | p002 | replay（Normal・Warning・Critical・多 core・2 GPU・disk 無し）ごとの固定の時計の PNG を golden と比べ、`ZMON LEVEL` の遷移の順が表どおり、simulated の field が段・Events を動かさない（host 試験）、呼吸 20 fps と操作 60 fps の `ZMON FRAME` | 5330 で 30/60 fps（目視は user） | 2〜3 日 |
| p004（M3a 操作） | tap の展開・長押しの固定・swipe（1 本指、graph の上）・pinch の俯瞰・app で検出する 2 本指の tap・key と pointer・`--calm` | p003 | QMP の touch・pointer・key で `ZMON CARD expand`・`pin`・`RANGE`・`VIEW` と PNG。pinch の後に RANGE が変わらない | 5330 の touch | 1 日 |
| p005（K1） | kernel: `hw.cputimes`、sysctl の CLI の表示、`top` の CPU の行 | — | kernel build warning 0、guest で `sysctl hw.cputimes` の 2 回の差: `yes > /dev/null` の CPU の user が増え、全 CPU の和 ≈ 経過 × CPU の数（±10%、QEMU の tick の揺れ）、SMP の stress、boot-test | — | 0.5 日 |
| p006（K2） | kernel: `hw.diskstats`（物理の whole disk、kind・id・generation） | — | guest で `dd` の読み書きの前後で該当の disk の bytes・ops が増え、他は増えない、latency の和が正、USB の disk の抜き差しで generation が変わる（QMP の device_del・add）、boot-test | NVMe（5330） | 0.5〜1 日 |
| p007（K3） | kernel: `hw.gputelemetry`（i915 の busy の和と実行中の分・周波数・objects） | — | build、Venus では要素 0、boot-test | 5330 で負荷で busy・周波数が動く | 0.5 日 |
| p008（M3 本物の値） | **4 つに分けて進める（Q1 に追加の ID を依頼）**: (a) backend の monitor 領域（zedBSD: p005・p006・p007・vmstat（5 秒ごと、所要時間の probe）・SIOCGIFSTATS・Vulkan の device 名）、(b) Linux・FreeBSD の backend、(c) compositor の `kl_system_monitor_v1`（専用の thread、ack と sample の丸ごとの間引き）と libkeiland の `kl_system_monitor_*`、(d) app の system の source | WS131 p010 までの統合（Q1 が P1 の進みを見て開始を決める）、p005・p006 | 3 OS の build と WS131 の checker（B1〜B3）、host 試験（率・counter の巻き戻り・機器の抜き差しの id・u64 の hi/lo・client が 60 秒読まない時に compositor の送信の queue が 1 sample を越えない）、guest で `--source=system` の memory が `top` の表示と ±1%、負荷の CPU の % が上がる、USB の disk の抜き差しで Disk の plate が追従 | 5330 | 3〜4 日 |
| p009（K4） | kernel: ACPI の thermal・電池（WS131 p005 と調整） | 実機 | — | 5330 で温度・電池 | 1〜2 日 |
| p010（M4） | 全文規約・回帰・デモの通し | p002〜p008 | `style-check.py`、全部の試験、boot-test | 5330 の通し | 1 日 |

順: p002 → p003 → p004（UI は sim で完成）。kernel の p005・p006・p007 は並行でよい。p008 は WS131 次第。

---

## 6. 判断（user・Q1）

| # | 問い | 提案・決定 |
| --- | --- | --- |
| D1 | WS131 p010 の前に app が OS の値を直接読む暫定の source（direct）を作るか | 作らない（§1.4）。sim で UI を完成させ、p008 で本物にする |
| D2 | stub の印 | **決定（2026-10-03 user）: 付けない**。stub の項目は §1.5 の表で管理し、各 Phase の報告に添える |
| D3 | HAL の MSR の API（CPU の周波数・温度・RAPL の電力）と割り込みの時間 | この WS では求めない。必要なら別に差分を出して user の承認 |
| D4 | 名前（System Monitor、`/bin/monitor`） | 案のまま（user「確認しなくていい」） |
| D5 | 呼吸の既定の frame の率 | 30 fps（QEMU では 20） |
| D6 | process の一覧（top の代わり） | 範囲の外（参考の画像とコンセプトに無い） |
| D7 | GPU の telemetry の出し方 | sysctl（Guardrail「compositor は GPU の UAPI を ioctl で呼ばない」に合わせる、review 3） |
| Q1 への依頼 | manager の version・opcode の予約（monitor は 3 の案）、WS131 の app の移行の一覧と p023 の前提に monitor を足す、p008 の分割の ID、`apps.conf` 等の path の順 | §1.3・§1.4 |

## 7. 試験の道具

- `plan/ws134/tests/monitor-guest.sh`（files-guest と同じ Venus の guest）、`monitor-p00N.sh`（replay で起動し、PNG と `ZMON` の log で判定）。
  `ZMON` の log は app の stdout を guest の file（`/tmp/monitor.log`）に書き、`files-guest.sh run` で読む（QEMU の console・serial の log は使わない）。
- **試験の mode**（review 16）: `--clock=fixed:T`（時計を止め、呼吸・視差・粒子・graph の流れを T の時点で止める）、`--calm`、`--seed=N`。この mode の
  PNG は毎回同じになるので、golden の PNG（`plan/ws134/tests/golden/`）と画素の差の割合（既定 0.5% 以下）で比べる。
- 描いた文字の確かめ: app は plate ごとに描いた文字を `ZMON TEXT plate=cpu value="37%"` で出す（数字の atlas の quad を組む時の文字列）。入力（replay）
  の値と照合する。
- memory: `ZMON MEM heap=… vulkan=…`（app が数える malloc の和と `vkAllocateMemory` の和）を 30 秒ごと。kernel に RSS が無い（`process_info` は
  `virtual_bytes` だけ）ので、app の自己申告で確かめる。
- frame の時間: `ZMON FRAME fps= cpu_ms= wait_ms=`（GPU の timestamp の query は i915 の対応が不確かなので、fence の待ちの時間で代える）。
- replay の入力は `plan/ws134/tests/replay/*.txt`（Normal・Warning・Critical・多 core・2 GPU・disk 無し・機器の抜き差し）。
- host 試験: `plan/ws134/tests/host/`（sim の model、率の計算と reset、履歴、警告の段の遷移の表、simulated の field を段から除く、2 本指の tap と
  swipe の分類）。

## 8. design-reviewer の指摘と反映（2026-10-03）

| # | 指摘 | 反映 |
| --- | --- | --- |
| 1 高 | 隠れた窓で FIFO の present が 10 秒止まり surface lost | §4.3 の「frame の許可」（自前の frame callback、SURFACE_LOST の回復、p002 の受け入れ） |
| 2 高 | compositor の送信の上限（1 MiB）で sample が欠け他の event も落ちる | §1.3 の流量の制御（ack、sample の丸ごとの間引き）、p008 の host 試験 |
| 3 高 | GPU の ioctl は Guardrail に反する | K3 を sysctl に、GPU の名前は Vulkan から（§1.2、D7） |
| 4 高 | K1 の interrupt は HAL の API なしに取れない | interrupt を外し other に、文書化（§1.2、D3） |
| 5 高 | stub の値が実機で警告・Events を出す | simulated の field を段・Events・コアから除く、注入は sim だけ（§1.5・1.6・3.8） |
| 6 | VMSTAT が重い | memory は 5 秒ごと、所要時間の probe、要れば K6（§1.2） |
| 7 | sample の thread | monitor 専用の thread、emit は event loop だけ（§1.3） |
| 8 | backend の API の形 | `struct kl_backend_monitor *kl_backend_monitor_open(void)`、link 16（§1.3） |
| 9 | 抜き差し・巻き戻り・index | id と generation、負の差は reset、u32 の幅（§1.3） |
| 10 | 64 bit・版 | hi・lo、機器ごとの `device` event、version は Q1 が予約（§1.3） |
| 11 | UAPI の layout・sysctl の段 | 2 段の leaf、header に version・struct_size・element_size・count、`_Static_assert`、ENOMEM で取り直し（§1.2） |
| 12 | K2 の範囲 | 物理の whole disk、実際の driver、`hal_rtc_read_counter`（§1.2） |
| 13 | gesture | 2 本指の tap は app で検出、swipe は 1 本指だけ、コアの drag は除く（§3.10） |
| 14 | titlebar の API | GENERIC の control 4 つと checked、状態の chip は窓の中（§2.1） |
| 15 | 文字の texture と slide | 数字は glyph の atlas と shader、label だけ texture（§4.1） |
| 16 | 試験が誤りを見つけられない | 固定の時計の mode と golden、`ZMON TEXT`・`MEM`・`FRAME`、`top` と比べる（§5・§7） |
| 17 | Phase と依存 | p008 を 4 つに、WS131 の一覧への追加と path の順を Q1 に依頼、見積もり（§1.4・§5・§6） |
| 18 | K1・K3 の細部 | THREAD_RUNNING でない時は other、atomic_u64、実行中の busy を足す、engine ごとは出さない（§1.2） |
| 19 | sim の切り替えの Event | 除いた（§3.8） |
| 20 | 文字の種類 | 数字は JetBrains Mono、bold は合成（§2.4） |
| 21 | Available の式 | free + cache_memory の file data の捨てられる分（§1.2） |
| 22 | 半透明の順 | 外の背面 → 中の背面 → 内 → 中の前面 → 外の前面、depth を書かない（§4.2） |
| 23 | apps.conf の色・Linux/FreeBSD の一覧 | 色は `4f9d8f`（青緑、他と重ならない）、Linux・FreeBSD の `apps.conf.in` と demo の一覧にも足す（§0） |
| 24 | 履歴の memory | 持つ field を決めた（§3.10） |
| 25 | GPU の 2 枚目 | stub は機器を作らない（§1.6） |
