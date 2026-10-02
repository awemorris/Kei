# WS112: 形式・依存・CI成果物契約

Status: q585-i01の設計調査。下記commandは後続Queueの実装/確認手順であり、今回build/package/CI実行はしていない。
Observation: 2026-10-02。正本: [設計](design.md)、[入力/実source証拠](phase001/survey.md)。

## native build・encoder

同じsource snapshotから各OSのnative compiler/headers/system libcでcompileする。
共通stageを先に生成し、encoderがpayloadを再compile・strip・再配置しない。形式の付属metadataはstage外とする。
shared library依存はnative ELFのNEEDED/symbol versionと各OSのpackage DBを突合する。hostの`ldd`でforeign ELFを実行しない。

| OS | native build packages / tool | encoderと形式 | 後続の独立確認 |
| --- | --- | --- | --- |
| Debian13 / Ubuntu26.04 | 現行`build-essential pkg-config python3 binutils dpkg-dev libvulkan-dev libdrm-dev curl ca-certificates xz-utils` | `dpkg-deb --build --root-owner-group STAGE PACKAGE`、deb2.0、control/conffiles/md5sumsとdata archive | `dpkg-deb --info/--field/--contents/--raw-extract`、`ar t`、controlと展開payloadをmanifest比較 |
| RPi OS Lite Trixie arm64 | 同じDebian系packagesを**公式RPi rootfs内**のnative aptで確認。既存packagesも版を記録 | 同じdpkg encoder、`Architecture: arm64`、RPi image/repo/marker由来をbuildinfoへ | `dpkg --print-architecture=arm64`相当の照合、全ELF machine AArch64、RPi identity証拠、deb形式/展開比較 |
| Fedora44 x86_64 | `gcc make pkgconf-pkg-config python3 binutils rpm-build libdrm-devel vulkan-headers vulkan-loader-devel curl ca-certificates xz cpio`候補。native repoで存在/versionを確認 | native`rpmbuild -bb --nocheck --nodebuginfo`、ad hoc specの`%install`は既存stageをコピー、binary rpmだけ。私有topdir/buildroot、root所有metadata | `rpm -qp --qf`、`rpm -qp --requires/--provides/--dump/--scripts`、`rpm2cpio`→専用rootへ`cpio`、native header/payloadと全manifest比較 |
| Arch x86_64 | `base-devel python vulkan-headers libdrm vulkan-icd-loader`候補、pacman/makepkg/libarchive(zstd/bsdtar)/fakeroot。native repoの同一日付を用いる | 最小PKGBUILDの`package()`でstage収録、native`makepkg --noconfirm --nocheck`、rootではなく専用非特権user。`.pkg.tar.zst`、source packageを生成しない | `pacman -Qip/-Qlp PACKAGE`、`bsdtar -tf/-tvf/-xOf`と専用展開、`.PKGINFO/.BUILDINFO/.MTREE`と全manifest比較 |

Encoderは公式package形式を満たす小さな既存toolとして選択。Debian packaging skeleton/Fedora dist-git/mock/Arch devtools/source packageの全面導入は必要としない。
RPM specはempty`%check`/`--nocheck`、Archはcheck関数なし/`--nocheck`とし、CI runtimeを間接復活させない。
RPM buildroot postprocessorとArch makepkgのstrip/debug機能が既存stageを変えたら、共通manifestと不一致で失敗させる。必要な抑止設定はp004/p005でnative版に対して確認する。
RPMはprivate file由来のProvidesを抑止し、Requiresの除外はstageで充足するprivate SONAMEだけに限定する。file全体のRequires除外でlibc等のsystem依存を消さない。
Archは専用makepkg設定のPKGEXTを`.pkg.tar.zst`に明示し、`!strip !debug`と必要directory保持を確認。圧縮toolとquery/extraction tool版も記録する。

公式一次仕様（2026-10-02閲覧）:
[dpkg-deb1.22.22](https://manpages.debian.org/trixie/dpkg/dpkg-deb.1.en.html)はroot-owner-groupと形式/query/extraction、
[rpmbuild6.0.2](https://rpm.org/docs/6.0.x/man/rpmbuild.1)はbinary-only/チェック省略、
[rpm6.0.2](https://rpm.org/docs/6.0.x/man/rpm.8)はuninstalled package/header/file metadata query、
[PKGBUILD](https://man.archlinux.org/man/PKGBUILD.5.en)・[makepkg](https://man.archlinux.org/man/makepkg.8.en)はversion/arch/非root encoder、
[pacman](https://man.archlinux.org/man/pacman.8.en)はpackage queryを確認した。これはguestのtool版を観測した結果ではない。
Fedora official package indexの[rpm-build44](https://packages.fedoraproject.org/pkgs/rpm/rpm-build/)は6.0.2-1.fc44、
Archの[pacman](https://archlinux.org/packages/core/x86_64/pacman/)は7.1.0.r9.g54d9411-2、[base-devel](https://archlinux.org/packages/core/any/base-devel/)は1-2を観測。

## 依存とconfig・license

| OS | 必須runtime依存の計算 / 明示分 | 任意サービス / config |
| --- | --- | --- |
| Debian13/Ubuntu26.04 | 既存`dpkg-shlibdeps -O -xkeiland -lSTAGE/opt/keiland/lib`、private Vulkanはshlibs.localとself-dependency除外。生成されたlibc6等に`libvulkan1, mesa-vulkan-drivers, libpam-systemd, kbd`を追加 | `Suggests: gdm3, wpasupplicant`を継承。conffiles `/opt/keiland/etc/keiland/apps.conf` |
| RPi OS13 arm64 | native dpkg-shlibdeps、RPi rootfs/repoのarm64 dependency名/版を確認。loader/ICD/PAM/kbdは同じ名前を候補とし実apt DBで検証 | GDM/WPAは任意。conffilesは同じpath。GUI/ICD実動作は試験しない |
| Fedora44 | native RPM ELF dependency generatorでlibc/libm等のcapability+versionを計算、private SONAMEだけをself-containedとして除外。absolute dlopen用`vulkan-loader`とICD用`mesa-vulkan-drivers`、seat/console用`systemd-pam,kbd`は明示候補 | `Suggests: gdm, wpa_supplicant`候補。apps.confに`%config(noreplace)`、noticeに`%license`。private Vulkanをsystem`libvulkan.so.1()(64bit)`のProvidesにしない |
| Arch | native ELF→package所有者照合、private SONAMEはstage内で充足、system ABI最小版を記録。`glibc, vulkan-icd-loader, vulkan-driver, systemd, kbd`候補 | `optdepends`にgdm/wpa_supplicant、`.PKGINFO`の`backup = opt/keiland/etc/keiland/apps.conf`（leading slashなし） |

RPMの[dependency仕様](https://rpm.org/docs/6.0.x/manual/dependencies.html)はRequires/Providesと弱い依存を区別する。
system loaderは[Fedora vulkan-loader](https://packages.fedoraproject.org/pkgs/vulkan-loader/vulkan-loader/)と[mesa-vulkan-drivers](https://packages.fedoraproject.org/pkgs/mesa/mesa-vulkan-drivers/)を確認。
[systemd-pam](https://packages.fedoraproject.org/pkgs/systemd/systemd-pam/)259.8-1.fc44、[kbd](https://packages.fedoraproject.org/pkgs/kbd/kbd/)2.9.0-4.fc44、
[gdm](https://packages.fedoraproject.org/pkgs/gdm/gdm/)50.3-1.fc44、[wpa_supplicant](https://packages.fedoraproject.org/pkgs/wpa_supplicant/wpa_supplicant/)2.11-9.fc44を公式indexで確認。これはbuild guest内の導入版ではない。
[Arch loader](https://archlinux.org/packages/extra/x86_64/vulkan-icd-loader/)には複数vendorのvulkan-driver候補が存在する。
全system依存名/最小版の最終確定は各native package DB＋実ELFから行い、上表の候補だけで形式/依存合格とはしない。
[dpkg-shlibdeps](https://manpages.debian.org/trixie/dpkg-dev/dpkg-shlibdeps.1.en.html)のprivate library検索/self除外を既存sourceへ照合した。

Arch[PKGINFOv2](https://man.archlinux.org/man/PKGINFO.5.en)は`xdata = pkgtype=pkg`、pkgname/pkgbase/pkgver/arch/builddate/size/packager、depend/optdepend/backup/licenseを収録する。
[BUILDINFOv2](https://man.archlinux.org/man/BUILDINFO.5.en)はPKGBUILD hash、native build環境/installed package版を収録し、Keilandの外部JSON buildinfoと区別する。
RPM/Arch license summaryは既存payloadの実license inventoryから集約し、fontやBSD由来の実装を一律Zlibと表記しない。
既存font notices/プロジェクトcopyrightとWS095 IME relicensingを保持。payload外のbuild toolsのlicenseをKeiland source licenseの変更と解釈しない。
package scriptsによる起動やuser data削除は禁止、install/remove runtime試験はCI受け入れ条件にしない。

## version・成果物の集合

`DATE=source commitのUTC YYYYMMDD`、`SHA12=source commit先頭12桁`、native package nameは`keiland`。
Debian/Ubuntuは既存version`0~gitDATE.SHA12-1+debian13/ubuntu2604`を保持。RPiは`0~gitDATE.SHA12-1+rpios13`。
RPMは`Version: 0.0.gitDATE.SHA12`と`Release: 1.fc44`、Archは`pkgver=0.0.gitDATE.SHA12`と`pkgrel=1`。
deb version文字列を他形式へそのまま代入しない。native parserでこのversion/archの受理と、同日の2commitおよび日付変更時の比較をp002〜p005で記録する。
SHAの辞書順はcommit時系列を保証しないため、上記version案だけで厳密なupgrade順を保証したとしない。現行deb契約は維持し、新formatの正式versionはnative比較結果も見て確定する。

| target_id | package architecture / ELF | package filename |
| --- | --- | --- |
| debian13 | amd64 / x86_64 | `keiland_0~gitDATE.SHA12-1+debian13_amd64.deb` |
| ubuntu2604 | amd64 / x86_64 | `keiland_0~gitDATE.SHA12-1+ubuntu2604_amd64.deb` |
| rpios13 | arm64 / AArch64 | `keiland_0~gitDATE.SHA12-1+rpios13_arm64.deb` |
| fedora44 | x86_64 / x86_64 | `keiland-0.0.gitDATE.SHA12-1.fc44.x86_64.rpm` |
| arch | x86_64 / x86_64 | `keiland-0.0.gitDATE.SHA12-1-x86_64.pkg.tar.zst`（Arch形式＋target_idでOS識別） |

各OSはpackage1件と同stemの`.manifest.json/.buildinfo.json/.sha256`を出力する。stemは**既知の完全suffix**（`.deb/.rpm/.pkg.tar.zst`）を除いて定義する。
checksumにはpackage/manifest/buildinfoの3件を漏れ/重複なく含める。全5OSで計20files、build bootのPNG等はrelease成功条件に含めない。
artifact namespaceは`keiland-linux-TARGET_ID`、専用directoryは`build/keiland-packages/artifacts/TARGET_ID`案。
旧`KEILAND_DEB_BUILD/KEILAND_DEB_ACCEL`の既存指定をp002で保持またはaliasし、互換を黙って切らない。

manifestはstageと独立展開のpath/type/filehash/mode/uid/gid/linktargetを比較、許可prefix/session以外・absolute symlink escape・path traversal・test/development payloadを拒否する。
directoriesの整合も含め、package形式自身のmetadata（DEBIAN/.PKGINFO/.BUILDINFO/.MTREE等）はpayload manifestから区別する。
buildinfoはtarget_id/OS-release/OS証拠/package architecture/ELF machine/source commit/source archive SHA256/dirty/epoch/image pin/compiler/tool版/native installed packagesとrepository由来/package hash/format/dependenciesを含める。
新contractのversion fieldを明示し、旧WS108 metadataを新schemaへ暗黙変換しない。

source archiveは全5jobで同じallowlistとcanonical member metadata、同じsource commit、deterministic outer gzipを使う。
現行`tarfile.open(..., 'w:gz')`はtar member mtimeを固定してもGzipFileへmtimeを指定しないため、外側gzip headerは時刻依存である（host Python3.13.5 `TarFile.gzopen`の呼出しを読取確認）。
p002でouter gzip mtime/filenameも固定し、source archive hash一致を検証してからp006の全OS hash一致gateへ渡す。

## CI/release verifierと失敗確認

matrixは上表の5target_idと指定make target/CPU/formatを明示し、sourceはcheckoutの同一`github.sha`。
Debian/Ubuntu/Fedora/ArchのQEMU x86_64ではTCGを基本に、RPi方式は[環境調査](native-environments.md)のD2解決後に設定する。
host dependencies、artifact path、期限は各jobの方式から定める。現在45minの全2OS jobを新RPiへ無根拠に流用せず、p003の実経過時間を根拠にする。
build guest起動確認はcompiler実行の前提、package導入/GUI runtimeの関門とは別。boot方法D1未決時は新guestを開始しない。

releaseは既存image/zip buildと**5package matrix全件**をneeds。download後、package内のnative metadataとsidecarを読み、5OS/CPU/source/形式が一意に揃うことを再確認する。
未知file/重複basenameがmerge-multipleで上書きされる前に、per-target directoryまたはartifact indexで検査する。
`require-clean`、checkout revision、deterministic source hash、sourceepoch、packagehash、manifesthash、入力pinsと各native identityを照合する。
release filesには既存image/zipと`*.deb/*.rpm/*.pkg.tar.zst/*.sha256/*.manifest.json/*.buildinfo.json`を含める。
release本文の導入例は`apt install ./PACKAGE.deb`、`dnf install ./PACKAGE.rpm`、`pacman -U PACKAGE.pkg.tar.zst`。build/形式確認済み、package runtime/remote Actions/release未実施の限界を記載する。

p006の有限否定試験は、実5成果物setのmissing1、duplicated target、truncated package/corrupt metadata、checksum filename traversal、OS/CPU/nativeformat/source commit/source hash/dirty mismatch、extra test appを各1例で拒否確認する。
smoke JSON/PNGが無くても**新contractを満たした実package**は受理し、PNG存在だけでは欠損packageを受理しない。
現行8sourceの最終reviewはp007。追加Cが生じなければC生成を仮定せず、Python/shell/Makefile/YAMLの近傍形式、syntax/manual/git diff --checkを実最終範囲で行う。
GitHub publication/push/remote Actions/releaseとhost installはq585範囲外。
