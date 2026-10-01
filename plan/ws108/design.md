# WS108 確定設計 / q545

Authority: 2026-10-02ユーザーの指定make/2OS QEMU build/install/runtime/既存CI release指示。[WSの判断](ws.md#2026-10-02--ws108-user-targets)。旧container案・release除外を置換。amd64、/opt/keiland、WS105対応済production境界を引継ぐ。

## Package manifest / P1

runtime binary package `keiland`。production: wayland compositor、terminal/files/settings/notes/textedit/imageview/pdfviewer、libexec/keiland-ime、10 private dynamic libraries、fonts/notices/IME dictionaries/gradient wallpaper/apps config/session desktop。
既存Linux buildにあるtest-only wlshm/wltest/vkdemo/mview/kuidemoとshare/mviewをruntime stagingから除外し、Apps HomeのModel viewer/Widget Demoもpackage用configから除く。元のnative Linux install/WS105 demoを変えない。browser/xserver/EGL/GLES/development headers/test packageは今回の配布対象に加えない。tests/mview sourceのcompositor display helper利用はruntime依存APIとして保持、source配置とpackage用途は別。
package version `0~git<UTC commit date>.<12sha>-1+debian13` / `...+ubuntu2604`、Architecture amd64。ABI後段system Vulkan/libdrm/glibcからnative guestでdpkg-shlibdepsと全ELF NEEDEDを照合。system Vulkanはdlopenなのでlibvulkan1を明示、Mesaはdefault software rendering smokeに用いる。libpam-systemd/logind/session、kbd等の意味も記録。session fileは/usr/share/wayland-sessions、user config Appsをconffile登録。package maintainer scriptsで自動起動、host操作、user data削除をしない。
project Zlibとfont OFL/Apache notices、既存dictionaryのWS095 relicensing/verified pinを収録。external runtime systemlibsはpackage managerの依存として別配布、vendoring無し。

## QEMU native procedure

`make keiland-linux-debian` / `make keiland-linux-ubuntu2604`は独立release driverを呼ぶ。host requires Python3/curl/ssh/tar/QEMU/xorriso。KVM availableなら利用、CI KVMなしはTCG、amd64 targetを偽装しない。amd64 guest memoryはMasterの2026-09-24決定に合わせ8GiB。
公式cloud images固定URL+checksum。Debian13 generic amd64 20260914-2601 SHA512 a733e7d4...、Ubuntu26.04 resolute release-20260927 SHA256 88006518...。完整hashはinputs.jsonへ。read-only cacheと各run専用qcow2 overlay、cloud-init seedと試験専用key、SSHは127.0.0.1ランダムforward port、QMP PNG。serial/console logs判定禁止。OS ID/version/archとSSHを必ず検証。
build guestとinstall test guestは同じpinned baseから別overlay。build guest apt署名検証を保ちnative gcc/dpkg-dev/libvulkan-dev/libdrm-dev等をinstallし、同一source snapshotから既存keiland-linux.mkをbuild/stage、runtime-filter/deb control・manifest・licenses・dependencies、dpkg-deb --root-owner-group。
source archiveはtracked Linux必要roots＋release tools（未commit自分の実装を含む）のallowlist、.internal/toolchain/buildを含めない。external dictionary/emojiは既存pin/hash検証で取得。source archive hashとcommit/dirty/tool/compiler/installedpackages/distro/guest hashを`.buildinfo.json`へ記録（Debian standard .buildinfoと混同しない）。
install guestはbuild tools無しfresh overlayにruntime依存とdebだけ導入。全NEEDED/lddとsession/conffileを確認、productionのdirect KMS compositor/app/Terminal入力、SSH/QMP screenshot、public client（build guestでcompile）を検証。upgrade/reinstallとremove/purgeでuser data/編集済configの保持を確認。両OSで同じ手順。time bounds: boot240s、apt1200s、build1200s、runtime各60s、全driver40min。失敗を記録し残りscopeを独立実行、endless retryしない。

## CI / release

既存 `.github/workflows/ci.yml`に2OSのmatrix job追加、ubuntu runner上でQEMU guestを実際に起動して同じmake targetを呼ぶ。toolchain/image/nightly zip jobは維持。各OS固有artifact name、deb/buildinfo.json/checksum/manifest/smoke PNGをupload、job/missing outputを失敗にする。release jobはbuildと2OS packaging完了をneeds、download artifactをmerged directoryへ、既存nightly release filesにdebとbuildinfo/checksumを追加。
push自動実行無し。localで同じdriver/guest手順を検証しremote Actions run未実施を明記。ユーザーの実行指示はCI定義/成果物作成の承認、remote push/publishを今回行う指示ではない。

## Phases / bounded verification

p001 q545最大60分: inputs/pin/manifest/依存/license/guest利用可能性/設計を固定、QEMU SSH/OS/PNG準備。p002最大120分: build/deb/2OS fresh install/runtime/upgrade/remove driverと指定make、実2OS成果物。p003最大60分: 既存CI matrix/releaseの組込み、同一driver evidence/YAML/build output契約。p004最大60分: 全変更のfull rules/packaging/security/data ownership/failure cleanup/diff/両OS final checks。C生成は既存public testを再利用し新規C不要。共通production C変更が必要なら限定しfinalaffected regressions/bootを追加。
P1 manifest、P2各guest native build/ELF/deb、P3各freshguest導入/GUI/session/public client/upgrade/remove、P4CI failing gates/distinct artifacts/release files、P5full conformance/既存CI維持をWS自身のacceptanceで照合。WS106未確定ime-probeはpackage対象外でこのscoped prerequisiteを阻害しない。MG007全体の完了とはしない。

## Primary references / availability checked 2026-10-02

[Ubuntu26.04](https://releases.ubuntu.com/26.04/)、[official Ubuntu image](https://cloud-images.ubuntu.com/releases/resolute/release-20260927/)、[Debian image](https://cloud.debian.org/images/cloud/trixie/20260914-2601/)、[dpkg-shlibdeps](https://manpages.debian.org/trixie/dpkg-dev/dpkg-shlibdeps.1.en.html)。full hashesを公式checksumから確認、host QEMU10.0.11/xorriso1.5.6/KVM。image hash mismatchは実行前拒否。

Event ws108-q545-design: ユーザーの具体化を全4Phase/WS/Master/Guardrailに反映、GitHub delivery pending。旧release/container案はこの設計で置換。
