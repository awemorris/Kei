# q585-i01: Linux package契約調査

Status: in-progress / 契約調査の部分証拠。実装・OS image取得・guest起動・package生成は未実施。
Observation: 2026-10-02 UTC、Agent A2、base `0e68854ace6a84b06eb23268ae75c4cf7b79b6da`。調査上限は07:12〜08:12 UTC。
Approval / scope: [A2 Queue](../../agents/A2/queue.md)、[p001](phase.md)。

## 実sourceと既存成果の照合

| Source / completed WS | 観測結果 | 後続変更の契約 |
| --- | --- | --- |
| [root Makefile](../../../Makefile) | Debian/Ubuntuの2入口だけが`run.py`を呼ぶ | 既存2入口を維持、RPi/Fedora/Archの3入口を追加 |
| [run.py](../../../tools/release/keiland-linux-deb/run.py) | source allowlist、pinned input hash、専用overlay/lock、loopback SSH、native build後に第2fresh guestのsmokeを必須実行 | p002でbuildとruntimeを分離。必須経路はnative build→stage/形式監査→成果物。任意runtimeと過去証拠を残す |
| [build.py](../../../tools/release/keiland-linux-deb/build.py) | amd64/2OS限定、native all/install/install-session、5test app除外、dpkg-shlibdeps、conffiles、manifest/buildinfo/checksum | 形式共通のproduction stageと形式別encoder/依存計算を分離。RPiはarm64を実確認 |
| [verifier](../../../tools/release/keiland-linux-deb/verify-artifacts.py) | 2 distro/amd64限定、smoke JSONと3 PNGが必須、manifestはsession/重複/test appを検査 | runtime入力を解除し5OS/CPU/形式/source/checksum/独立payload比較を必須にする |
| [CI](../../../.github/workflows/ci.yml) | `keiland-deb`の2OS matrix、TCG、45min、image/zipとdebのrelease | 5OS全件をneeds/download/最終verifierへ渡し、既存image/zip保持。rpm/Arch拡張子もfilesへ |
| [native mk](../../../userland/desktop/keiland-linux.mk) / [WS111](../../ws111/ws.md) | `bin/keiland-desktop`を0755でinstall、GDM entryはwayland直接Exec | 共通launcher収録を検査、GDM直接Execを保持 |
| [WS105](../../ws105/ws.md) / [WS108](../../ws108/ws.md) | system libc/native toolchain/private ELF境界と実QEMU/native package結果はcompleted | 新OS合格へ転用しない。過去のGUI/導入結果を保存 |

`run.py`のsource snapshotはdesktop/base compatibility/PDF/明示libc compatibility header・digest source/font licenseだけのallowlist。
target libc/toolchain/共有build/.internalを含めず、source archiveのuid/gid=0、mtime=commit epoch、source commit/hash/dirtyを記録する。
旧driverはruntime client `vk-chain-test`もbuild/candidateへ入れるが、package/release payloadからは除外する。
WS112では必須package工程からclient compileも除き、任意runtime側だけに残す。

## 5OSの入力候補と観測

下表のhashは公式metadataとの一致確認であり、image本体の取得/hash照合を実施したという意味ではない。checksum署名の暗号検証は後掲の通り一部実施した。
mutable `latest`をproduction pinにしない。image pinsとguest内repositoryから導入するbuild packagesの版/由来は別々に記録する。

| Target / OS / CPU | 版とinput URL | Hash / 確認結果 |
| --- | --- | --- |
| `keiland-linux-debian` / Debian13 / amd64・x86_64 | [20260914-2601 generic qcow2](https://cloud.debian.org/images/cloud/trixie/20260914-2601/debian-13-generic-amd64-20260914-2601.qcow2) | SHA512 `a733e7d49442a03e70d03e4eb5aaf3967f3efc69ef70952f9bb10fc1ee2c4876eb95956b5ad2d31350e5fada768feb651352535fb8cd1233f61998a5a7d2e93c`、既存inputs.jsonと公式SHA512SUMSが一致 |
| `keiland-linux-ubuntu2604` / Ubuntu26.04 / amd64・x86_64 | [release-20260927 cloud img](https://cloud-images.ubuntu.com/releases/resolute/release-20260927/ubuntu-26.04-server-cloudimg-amd64.img) | SHA256 `8800651811af9a85465ad1d552add729947bb16488dddb4a9b5305a3d97332b2`、既存inputs.jsonと公式SHA256SUMSが一致 |
| `keiland-linux-rpi` / Raspberry Pi OS Lite Trixie / arm64・aarch64 | [2026-09-15 official image](https://downloads.raspberrypi.com/raspios_lite_arm64/images/raspios_lite_arm64-2026-09-15/2026-09-15-raspios-trixie-arm64-lite.img.xz) | compressed SHA256 `cdf4f3bfac35ae947b46e4e767f935453810549779ac3290e05a6754aee627e5`、公式download pageと同名.sha256が一致。official Imager catalogのexpanded image SHA256 `49fafba626ec00e0f9800349b9edae6caf8cfc223f673b875b72ac2797ed9576`、サイズ3061841920 bytes |
| `keiland-linux-fedora44` / Fedora44 / x86_64 | [Cloud Base Generic44-1.7 qcow2](https://download.fedoraproject.org/pub/fedora/linux/releases/44/Cloud/x86_64/images/Fedora-Cloud-Base-Generic-44-1.7.x86_64.qcow2) | SHA256 `28680fe5b371a5a82ebf43a31926e086a168e59949d03969c5093e7071f90b7f`、公式download pageとofficial redirectで取得したsigned CHECKSUMの値が一致。image583729152 bytes |
| `keiland-linux-arch` / Arch snapshot / x86_64 | [Cloudimg20261001.604814 qcow2](https://geo.mirror.pkgbuild.com/images/v20261001.604814/Arch-Linux-x86_64-cloudimg-20261001.604814.qcow2) | SHA256 `360f0fa49db6813bdc8e35bed230a2dc2ae3567b7b5ab74719c0a706e4e34e87`、版付きdirectoryの同名.SHA256とlatest値が一致。image578080256 bytes。`v`なしdirectoryは404、将来保持は保証しない |

Debian/Ubuntuは既存pinを保持する。RPiは公式Lite arm64の実rootfsを使い、Debian13 arm64だけをRPiと称して出荷しない。
FedoraのGeneric imageを選ぶ。UEFI-UKI imageは別hash/firmware経路なので取り違えを拒否する。
CPUはDebian系metadataではamd64/arm64、ELF/`uname -m`ではx86_64/aarch64、RPM/Archではx86_64を明示対応させる。

## 共通production payload

sourceのnative install membershipから生成し、stageと独立package展開を全件比較する。同じsource revisionを用い、OS/CPUごとのELF hashが同一であるとは要求しない。

| Path / 内容 | Mode / 契約 |
| --- | --- |
| `/opt/keiland/bin/{wayland,terminal,files,settings,notes,textedit,imageview,pdfviewer}` | native ELF、0755。`keiland-desktop`は共通shell script0755 |
| `/opt/keiland/libexec/keiland-ime` | native ELF0755 |
| `/opt/keiland/lib/{libwayland-client.so,libkeiland.so,libkeiui.so,libtruetype.so,libpdf.so,libz-compat.so,libpng-compat.so,libjpeg-compat.so,libgif-compat.so,libvulkan.so.1}` | private ELF0755、SONAMEとRUNPATH `/opt/keiland/lib`を監査。systemlibの代替としてglobal exportしない |
| `/opt/keiland/share/fonts`・`share/licenses` | native mkのfont/notice membership、0644。Inter/JetBrainsMono/DroidSansFallback/NotoColorEmojiを保持 |
| `/opt/keiland/share/kei/ime/ja/{SKK-JISYO.X,SKK-JISYO.kei}` | 0644、既存宣言のarchive/dictionary hashとWS095 license保持 |
| `/opt/keiland/share/keiland/wallpaper.ppm`・`wallpapers/*.ppm` | 0644、専用build由来。host/userの画像をsource snapshotへ混ぜない |
| `/opt/keiland/etc/keiland/apps.conf` | 0644、各package形式のconfig保持metadataを設定。mview/kuidemo entryを削除 |
| `/opt/keiland/share/doc/keiland/{copyright,README}` | 0644、license・導入説明・runtime未検証の範囲を記録 |
| `/usr/share/wayland-sessions/keiland.desktop` | 0644、[現行entry](../../../userland/desktop/wayland/linux/keiland.desktop)のwayland直接Exec、launcherへ差替えない |

全archive entryのuid/gid=0。directories0755、filesは上表、symlink追加時はtarget/typeをmanifestへ明示する。
`wlshm/wltest/vkdemo/mview/kuidemo`、share/mview、headers、static archives、test client、browser/xserver/EGL/GLESはruntime payloadに含めない。
install scriptsでsession自動起動、host/user data削除を行わない。
実sourceのprivate Vulkan proxyはsystem loaderをabsolute dlopenするため、DT_NEEDED計算だけでruntime依存が完結しない。
`libvulkan-compat/Makefile.linux`はmultiarch・x86_64-linux-gnu・aarch64-linux-gnu・lib64・libを探索候補としている。
loader候補が新OSのfilesystemで実在することは後続のnative package auditで確認する。

## 小さいmetadata取得の証拠

web toolで一次download/仕様を閲覧後、Python3 urllibでchecksum/index/catalogだけをGET（request timeout20s、body上限200000または500000 bytes、並列最大6）。OS/image bodyはGETしていない。

| URL / 出典 | 2026-10-02確認結果 |
| --- | --- |
| [Debian SHA512SUMS](https://cloud.debian.org/images/cloud/trixie/20260914-2601/SHA512SUMS) | webは取得不可、bounded HTTP200で該当qcow2のhashを照合 |
| [Ubuntu SHA256SUMS](https://cloud-images.ubuntu.com/releases/resolute/release-20260927/SHA256SUMS) | webは取得不可、bounded HTTP200で該当amd64.imgのhashを照合。SUMS.gpgを公式cloud image keyで暗号検証、exit0/VALIDSIG一致 |
| [RPi downloads](https://www.raspberrypi.com/software/operating-systems/) | webでLite64-bit/2026-09-15/Trixie/6.18/hashを確認。direct HTMLは403 |
| [Imager catalog](https://downloads.raspberrypi.com/os_list_imagingutility_v4.json) | bounded HTTP200、Lite64-bitのURL/expanded hash/size、`init_format=cloudinit-rpi`を確認 |
| [RPi image checksum](https://downloads.raspberrypi.com/raspios_lite_arm64/images/raspios_lite_arm64-2026-09-15/2026-09-15-raspios-trixie-arm64-lite.img.xz.sha256) | bounded HTTP200、download pageと一致。latest/latest-sha256 shortcutは404なので使用しない |
| [RPi repo InRelease](https://archive.raspberrypi.com/debian/dists/trixie/InRelease) | bounded HTTP200、Origin/Label Raspberry Pi Foundation、Codename trixie、arm64収録。signature未検証 |
| [Fedora downloads](https://fedoraproject.org/cloud/download/) | web/HTMLで44-1.7 Generic x86_64のURL/hashとsigned CHECKSUM参照を確認 |
| [Fedora CHECKSUM](https://download.fedoraproject.org/pub/fedora/linux/releases/44/Cloud/x86_64/images/Fedora-Cloud-44-1.7-x86_64-CHECKSUM) | official redirect→`ftp.yz.yamagata-u.ac.jp`でHTTP200、Generic hash一致。dl.fedoraproject.org直URLとarchives直URLは404。公式Fedora44 keyでCHECKSUM署名を暗号検証、exit0/VALIDSIG一致 |
| [Arch index](https://geo.mirror.pkgbuild.com/images/v20261001.604814/)・[SHA256](https://geo.mirror.pkgbuild.com/images/v20261001.604814/Arch-Linux-x86_64-cloudimg-20261001.604814.qcow2.SHA256) | bounded HTTP200でfilename/hashと版付きURLを確認。latestと版付きSHA256.sigの双方をarch-boxes公式README keyで暗号検証、exit0/VALIDSIG一致。版付きSUMS値は同一 |

署名fingerprint/日付/未確認範囲とnative環境commandは[環境証拠](../native-environments.md)、encoder/依存/config/version/manifest/CI契約は[形式契約](../package-contract.md)へ記録。
host Python3.13.5/git2.47.3/curl8.14.1/GnuPG2.4.7/QEMU10.0.11を読取確認。ignored tempのisolated keyringのみ使用し、host install/buildは行わなかった。

## 未決 / 依存を解除しない条件

| ID | 不足 / 判断 | 再開条件 |
| --- | --- | --- |
| D1 | Fedora/ArchへのSSH/QMP boot例外適用をmainがuserへ確認、返答待ち。RPiは採用方式の外側Debian既存例外を使用 | Fedora/Archの判断元と共有Guardrailをmainが保存後に解消。serial/console log判定を許可したと解釈しない |
| D2 | 解決: mainがdelegated technical判断として外側既存Debian13 pinned QEMU VM＋内側公式RPi OS13 Lite arm64 rootfs/QEMU-user native compilerを採用 | [環境契約](../native-environments.md)の隔離/syscall/OS/compiler/ABI証拠commandをp003の新Queueで実証。実環境成立や実行許可と混同しない |
| D3 | Ubuntu/Fedora/Archのchecksum署名trust rootは実検証済み。RPi image署名はfull image未取得、Debian現行SUMS署名は提供されない。versioned Arch URL/repo snapshotは確認済み | 既存Debian pin契約を保持。後続Queueで実imageのhash/利用可能な署名を確認、公式input消失なら明示pin改訂へ戻す。将来保持を保証しない |
| D4 | native encoder/依存計算/query/extraction commandを定義。公式package index名/版とRPi.info inventoryを確認、実guest/実ELF未確認 | 後続Queueでnative DBと実ELF依存を照合し、候補名や公開inventoryを実native build合格としない |

p001は全基準を実証するまでin-progress。未知を後続Phaseへ押し付けてclearしない。
