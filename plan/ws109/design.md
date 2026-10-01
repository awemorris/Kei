# WS109 FreeBSD15 移植の先行設計

## 引き継ぐ決定

ユーザー「Linux版KeilandをFreeBSD15に移植」「グラフィックはLinuxのコードをそのまま使える」「audio/network/WiFi は FreeBSD backend」。
共通 renderer/compositor/標準 Vulkan を共有する方針。F-065 の独自 Wayland/libkeiland、独自 WSI→system Vulkan、/opt/keiland を保つ。
FreeBSD 分を新 WS に promote し、WS105 completed の acceptance に追加しない。

## 実装の証拠と要確認

| 部分 | 現 Linux source の事実 | FreeBSD で確認する境界 |
| --- | --- | --- |
| composition/paint/標準 Vulkan | platform と分離した source を持つ | 共通 source を再利用、同じ public contract と render/sync の証拠 |
| libvulkan-compat/wsi-swapchain.c・wayland/linux/gpu-linux.c | linux/dma-buf.h、IMPORT/EXPORT_SYNC_FILE ioctl | FreeBSD15 の実 DRM/sync fd ABI と backend extension の能力。Linux header の存在を仮定しない |
| backend.c | RTLD_DEEPBIND は #ifdef。glibc の binding を Linux で検証済み | FreeBSD rtld で同名 Vulkan/compat symbols の循環/誤解決が無い実証 |
| compositor os/seat/input | Linux KD/VT、logind/DBus、sys/sysmacros、evdev の扱い | native VT/device permissions と非 root seat、evdev header/ABI・fd受渡し |
| libkeiland audio | ALSA の Linux module | FreeBSD OSS/mixer 等の native API、列挙/音量/mute/通知。PCM の要否は設計時に確定 |
| network/link/WiFi | Linux 状態の取得、共通 WPA wire | FreeBSD interface/link/net80211 と wpa_supplicant の利用可能な範囲。IP/DHCP は service と UI の所有を区別 |
| build/install/runtime | GNU/Linux compiler flags、multiarch、/opt paths、shared protocol | native libc/toolchain、GNU make 利用可否、pkg dependencies・runtime dir・配置 |

Linux の描画本体が共有できることと、OS の fd/ioctl/seat まで変更無しで使えることは別々に確認する。
CPU wait/fallback は Linux にあるが FreeBSD の buffer import/export が存在する証拠にはしない。
必要な ABI が無い場合は、適用可能な同期・buffer方式と public contract の影響を示して選択を解決する。未検証の dma-buf/sync_file を FreeBSD 対応済みと記録しない。

## p001 の調査・判断

初期案: native FreeBSD15.x amd64、固定 release と headers、system Vulkan/driver、disposable guest で build/サービス、実表示を確認できる device/環境。
非 root device 所有には handbook が説明する seatd も候補。ただし我々の compositor の backend として採用する決定は未了。
device/GPU/仮想GPUが未確定なので VM の画面成功を約束しない。必要な環境と方法を具体化し、従来の boot 規則に必要な scoped exception を記録する。
FreeBSD rc/service/permission の手順、licenseとGPL条件、audio/network/WiFi の実検証条件を定義してから後続 Queue を選ぶ。
system driver の GPL/LinuxKPI と Keiland 自身の source の license を区別。GPL-free 要件に矛盾する stack を黙って採用しない。

## 参照（2026-10-01 確認）

- [FreeBSD15.0 公式情報](https://www.freebsd.org/releases/15.0R/)と[release notes](https://www.freebsd.org/releases/15.0R/relnotes/): fixed target の入口。
- [FreeBSD handbook Wayland](https://docs.freebsd.org/en/books/handbook/wayland/): drm-kmod、非 root の seatd と runtime の配置。Keiland への互換性そのものは証明しない。

source/header/ABI の固定版への突合せと device 検証は p001 未実行。ports/upstream source の license と interface を参照できても、外部 implementation を我々の source へ取り込まない。
