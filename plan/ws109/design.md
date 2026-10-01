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

## 2026-10-02 q550の調査結果 / design候補の更新

[対応表・取得の証拠・D1〜D3](../history/ws109/q550/survey.md)。初期15.xは15.1-RELEASE amd64 UFSに具体化（image hash verified）、native loaderにもRTLD_DEEPBINDがある。drm-kmod fixedBSDUAPIにはsync-file import/exportがあるがGPLv2を含み、systemstackの採用はユーザー判断待ち。QEMU virtioのconsole framebufferは実DRM/Vulkan表示の証明にならない。OSS mixerのみ（既存F4範囲、PCM追加なし）、AF_LINK/net80211/WPApath/Unixaddress、PTYlibutil、Filesextattrをnative境界で扱う候補。F1 environment/license未確定のため設計の確定とはしない。後続Phase scope/criteria変更は回答/実環境の証拠に基づき保存してからコメントする。

## 2026-10-02 直接のユーザー判断 / 再設計

ユーザー「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」（2026-10-02 JST、このchat）。D1:既存FreeBSD drm-kmod利用可、GPL-free systemstack条件をこの範囲で置換。Keiland sourceの寛容license/外部実装を取り込まない境界は維持。D2:WS109専用FreeBSD QEMU guestのloopback SSH/QMP PNG検証を承認。D3:実機はユーザーが準備、入手前にnative build/backend実装を進める。実GPU/WiFi結果は将来の実機関門に残し、mock/QEMUbuildで代替しない。

q550のorigin p001の未決D1/D2は解消、D3は実機準備まで実装を進める承認。p001はfixed software/ABI/driver contractとQEMU実environmentを確定する。F1 physical deviceの実model、F3表示/fence/seat、F4realWiFiは実機到着後の最終関門を保つ。p002のnative独立build/library chainはverified p001 software outputがprerequisite。p003/p004はbackend実装＋guestで可能な実API確認を先行し、hardware acceptanceが必要な全Phaseの結果を分けて残す。p005は全source規約＋3OS regression、WS final acceptanceには未実施実機項目を持ち越す。

## q551 native design output

[Actual environment/ABI and execution procedure](../history/ws109/q551/environment.md). p002 L1→p003/p004 backend outputs→p002 L2 integration→p005 conformance/regression/physical gates. No wholePhase dependency cycle; actual scoped output is mandatory before selection. libseat/basu linkage is not selected for production; native seatd-only permissive contract is verified by p003.
