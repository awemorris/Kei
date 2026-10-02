# WS112: 公式入力・native環境・後続command

Status: q585-i01の有限調査。Observation: 2026-10-02 UTC。image本体取得、guest起動、rootfs展開、buildは未実施。
入力の全URL/hashは[調査表](phase001/survey.md)、encoder/成果物は[package契約](package-contract.md)。

## inputとtrustの確認

checksum metadataの値と、未取得image本体のdigestは別々の検証である。
新Queueでbounded downloadを行い、version URL/filename/size/hashを照合してから専用overlay/rootfsを作る。HTTP失敗やhash差異時にlatestへ自動fallbackしない。

| OS | 固定input・署名の観測 | 後続の検証 |
| --- | --- | --- |
| Debian13 | 既存20260914-2601 generic qcow2とSHA512SUMSが一致。`SHA512SUMS.sign`はHTTP404 | 既存SHA512 pin/TLS公式URLを維持し、実imageのSHA512を照合。存在しない署名を検証済みとしない |
| Ubuntu26.04 | 既存release-20260927。SHA256SUMS.gpgを公式documentationのcloud-image fingerprintで暗号検証、exit0 | isolated keyringで同じfingerprint/SUMS署名を検証してからimage SHA256照合 |
| RPi OS13 Lite arm64 | 2026-09-15 image.xz、圧縮SHA256とImagerの展開SHA256/sizeを確認。image.sigは取得可能 | 圧縮digest→公式download keyによるimage署名→展開raw image digest/size。署名がraw/compressedのどちらを対象とするかは検証commandで確認し、曖昧なまま成功としない |
| Fedora44 | Generic44-1.7。signed CHECKSUMをFedora44公式keyで暗号検証、exit0 | 同じfingerprint/署名とGeneric image SHA256/size。UEFI-UKIは別inputなので拒否 |
| Arch | versioned `images/v20261001.604814/`はHTTP200、latestと同じfilename/hash | 版付きSHA256/SHA256.sig、arch-boxes CI key、image SHA256/size。repoは2026/10/01 snapshotを固定しnative pacman署名を保持 |

署名の小metadata検証はisolated `GNUPGHOME`をignored tempに置き、host user keyringを変更しなかった。
`gpg --batch --status-fd 1 --verify SIGNATURE SUMS`（Fedoraはclearsigned CHECKSUM）のexit0と`VALIDSIG`を照合した。
Web of Trustは設定していないため`TRUST_UNDEFINED`があっても、公式文書で公表されたfingerprintとの一致を別に確認した。image本体は検証していない。

| Trust source（2026-10-02閲覧） | Fingerprint / 実確認 |
| --- | --- |
| [Ubuntu公式verify guide](https://ubuntu.com/docs/public-images/public-images-how-to/verify-image-checksum/) | `D2EB44626FDDC30B513D5BB71A5D6C4C7DB87C81`、SUMS signatureのVALIDSIG一致、signature date2026-09-29 |
| [Fedora security](https://fedoraproject.org/security/)・[公式fedora.gpg](https://fedoraproject.org/fedora.gpg) | `36F612DCF27F7D1A48A835E4DBFCF71C6D9F90A6`、Fedora44 CHECKSUMのVALIDSIG一致、signature date2026-04-24 |
| [Arch公式arch-boxes README](https://raw.githubusercontent.com/archlinux/arch-boxes/master/README.md) | primary `1B9A16984A4E8CB448712D2AE0B78BF4326C6F8F`、signing subkey `656E4C5AC1CC3B86E539D97E343635A6859A9174`、SHA256.sigのVALIDSIG一致、signature date2026-10-01 |
| [Raspberry Pi公式engineerのkey更新案内](https://forums.raspberrypi.com/viewtopic.php?t=394045) | `F4AADD86C4687D69AE04543E796C114AD12B2292`、image.sigのpacket issuerと一致。web searchでstaff案内を確認、page openは403。image署名の暗号検証は未実施 |
| [Debian公式cloud index](https://cloudfront.debian.net/cdimage/cloud/) | cloud imageの現行署名は提供されず、公式HTTPS/TLS等を案内。既存WS108の固定URL/hash契約を保持、独自署名基盤を追加しない |

keyserverの応答だけをtrust sourceにしない。RPM/pacman/aptのrepository signatureはimage署名と別にnative guestで検証・記録する。
公開URLの将来保持を保証する資料は得られていない。固定inputが消えた場合は失敗を記録し、版/pinの明示改訂をp001等へ戻す。

## x86_64のnative QEMU build

Debian/Ubuntuは実WS108のQ35、8GiB/4CPU、20GiB専用overlay、cloud-init CIDATA、loopback SSH/QMPを維持。
Fedora Generic cloudとArch cloudimgもcloud-initによるbuild user/鍵とnative repo setupを候補とし、実guest readinessは後続Phaseで確認する。
Arch basic imageのdefault passwordへ依存しない。[arch-boxes公式README](https://github.com/archlinux/arch-boxes)でcloudimgのcloud-init搭載を確認した。
base imageはread-only、guest専用overlay/lock/temporarySSH key、hostfwdは127.0.0.1だけ、TCGをCIの既定候補とする。
既存のsocket/keyboard/serial/console log制約を保ち、新OSへboot例外があるとは解釈しない（D1）。

後続のguest command（今回未実行）:

```sh
cat /etc/os-release
uname -m
cc -dumpmachine
cc --version
cc -print-sysroot
cc -print-file-name=libc.so
pkg-config --modversion libdrm vulkan
readelf --version
python3 --version
```

`ID/VERSION_ID`、machine/compiler target、system libc/headersのpackage所有者をnative DBから確認する。
`-print-sysroot`が空ならnative既定rootであり、それだけで失敗としない。headers/libraryの実pathとownerも記録してhost toolchain混入を拒否する。
Debian系は`dpkg --print-architecture`と`dpkg-query -W`、Fedoraは`rpm --eval '%{_arch}'`と`rpm -qa`、Archは`pacman -Q`をbuildinfoへ保存。
native導入commandはDebian系`apt-get update`/`apt-get install`、Fedora`dnf install`、Arch`pacman -Syu`/`pacman -S`。
package名は[形式契約](package-contract.md)の候補を同OSのrepoで確認し、signature checksを無効化しない。host側installは行わない。
Debian/Fedoraのupdates repoは動くため、image pinだけではtoolchain版を固定しない。installed版とrepo由来を記録して再実行差異を明示する。
Archは[2026/10/01公式repo](https://archive.archlinux.org/repos/2026/10/01/core/os/x86_64/core.db)のHEAD200/size129871を確認。core/extra両repoのsnapshot整合とpackage署名は後続native DBで確認する。

共通sourceを専用guest directoryに転送し、native `make -j4 -f userland/desktop/keiland-linux.mk all`→`install`→`install-session`を順に実行する。
専用DESTDIRからproductionだけを選び、test/demo削除とapps.conf調整後、ELF/依存/manifest監査を行う。build guestが起動した事実をpackage runtime合格としない。
package queryと展開はtarget native toolまたは形式を理解する読取toolで行う。native ELFをhost `ldd`で実行しない。

## RPi OS実rootfsの成立性（D2未決）

公式[image .info](https://downloads.raspberrypi.com/raspios_lite_arm64/images/raspios_lite_arm64-2026-09-15/2026-09-15-raspios-trixie-arm64-lite.info)はRPi reference2026-09-15、
pi-gen commit `2c235fa703cacb65e0fe0b2ab2fcb23d44dfd268`、stage2（Lite）を記録。bounded GETで確認したpackage inventory:

| Package | 公式.infoに記載された版 / CPU |
| --- | --- |
| gcc / gcc-14 | `4:14.2.0-1` / `14.2.0-19`、arm64 |
| binutils / make / python3 | `2.44-3` / `4.4.1-2` / `3.13.5-1`、arm64 |
| dpkg / dpkg-dev | `1.22.22` arm64 / `1.22.22` all |
| linux-image-rpi-v8 | `1:6.18.50-1+rpt1`、arm64 |
| raspberrypi-archive-keyring / raspberrypi-sys-mods / raspi-config | `2025.1+rpt1` all / `1:20260914` arm64 / `20260730` all |
| cloud-init / rpi-cloud-init-mods | `25.2-1~bpo13+1+rpt20` / `1:20260119`、all |

これらはimageの公開inventoryであり、今回rootfsやcompilerを実行した証拠ではない。
OS-releaseはDebian由来のIDの場合がある。IDを`raspios`等と決め打ちせず、固定raw image hash、RPi reference/`/etc/rpi-issue`等の実marker、RPi packages/repoを組み合わせて確認する。
rootfs抽出はraw partition tableとfilesystemを専用directoryへ扱い、host block device/既存mountを変更しない方式を選定する。

| 方式候補 | 成立性の一次資料 / 未解決点 |
| --- | --- |
| RPi board image + QEMU raspi3b/raspi4b | [QEMU raspi](https://www.qemu.org/docs/master/system/arm/raspi.html)は対応machineを列挙するが、raspi4はGENET/PCIe未実装。現行hostQEMU10.0.11とdocs11.1.50の差、kernel/DTB/SD容量/SSH transportを要確認。board imageだけでboot/ネットワーク成立としない |
| QEMU virt + 補助generic arm64 kernel + 実RPi rootfs/chroot | [QEMU virt](https://www.qemu.org/docs/master/system/arm/virt.html)はgeneric virt機器に適合するkernelを要する。RPi公式[bcm2711 defconfig](https://raw.githubusercontent.com/raspberrypi/linux/rpi-6.18.y/arch/arm64/configs/bcm2711_defconfig)の閲覧ではVIRTIO/ARCH_VEXPRESS記述を確認できない。実installed configは未取得。補助kernelの別OS由来/pin/署名と起動記録が必要で未選定 |
| 実RPi rootfs + QEMU user-mode arm64 + 隔離chroot | [QEMU user](https://www.qemu.org/docs/master/user/main.html)はtarget syscallをhost kernelへ変換、arm64 gcc自体はRPi rootfs内のnative実行を候補にできる。target kernelのbootをしない点、host kernel共有、rootfs/namespace/host bindの境界が設計選択として未承認 |

公式[pi-gen README](https://github.com/RPi-Distro/pi-gen)はarm64 branch、stage2 Lite、QEMUを用いた異CPU生成の説明があるが、Keilandの隔離build成立や上記方式の受け入れを保証しない。
[binfmt_misc](https://www.kernel.org/doc/html/latest/admin-guide/binfmt-misc.html)のF flagはchroot内interpreter問題への手段。host global登録/特権/namespace設計を今回実行も既定化もしない。
いずれも全compileは実RPi rootfsのarm64 compiler/system headers/libraryから行い、host cross gccや別Debian sysrootをRPi native buildとして使わない。
user-modeの場合は`uname -m`だけではkernel/CPU証拠が足りないため、compiler target、gcc ELFのAArch64、出力全ELFのMachine、native dpkg arm64とRPi provenanceを確認する。

## 判断と再開

D1: 新3OS build guestの許可されたboot判定/transportをmainが既存承認と照合する。共有Guardrail改訂が必要ならmainが判断元を保存する。
D2: RPiのsystem guest/補助kernel/rootfs user-mode方式は受け入れ・host境界に影響するため、Agent A2が選択済みとはしない。上表のfactsを判断資料とする。
p001はこれらの未解決人間判断を残したままclearしない。後続Queueは実input取得/起動で成立性を実証し、失敗・不足・経過時間を有限に記録する。
