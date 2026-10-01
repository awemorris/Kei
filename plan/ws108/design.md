# WS108 パッケージ・CI 設計案

## 現状と対象

.github/workflows/ci.yml は ubuntu-latest 上で zedBSD toolchain/image/nightly を作り、Linux .deb job は無い。
WS105 の make keiland-linux / keiland-linux-install は独立 native build と DESTDIR /opt/keiland を提供する。
Debian13 と Ubuntu26.04 は別々に target 環境を固定し、同じ commit から別 artifact を作る。
amd64/runtime1 package を出発案とする。test package/devel package 分割、version命名、収録範囲は p001 で固定する。

## 内容と build の責務

- runtime: Linux 対応済み compositor/libwayland/libkeiland/libvulkan-compat、必要な compat libs/fonts/data、主な production app。
- userland/tests の app は runtime の既定 manifest から分離する案。gpudemo を配布デモに収録するかは purpose と manifest で決める。source root と配布種別は別。
- public header の devel 用途と独立 client 検証を用意。system Vulkan/DRM、WPA/ALSA、gdm/logind 等の runtime deps と optional 機能の扱いを明記。
- staging/install 後の SONAME/RUNPATH/DT_NEEDED、private compat と system の ABI、system Vulkan を後段として load する経路を検証。
- 基本の layout は /opt/keiland を保持。Debian の /opt の扱いと既存方針が衝突するなら例外/影響を設計で説明し、勝手に /usr へ移さない。
- package hooks が必要な session 登録/解除だけを対象とし、install 時の自動セッション開始や利用者データの削除を避ける。
- distro の native compiler/libc で build、source tarball/externals の hash/provenance/notice と dependency versions を記録。

## CI と確認

build job の target は Debian13/Ubuntu26.04 と明記。container image の入手/固定 digest、GitHub runner と action の version は実装時に確認。
dpkg-buildpackage/debhelper を利用する案。既存 nightly の publish を .deb artifact の release と混同しない。
fresh target で install/upgrade/remove/public header client/CLI/依存を確認し、CI artifact に .deb/buildinfo/checksum と対象 distro/architecture を載せる。
container では実 GUI/DRM/seat は未検証。session の必要な確認は disposable target guest の実 PNG と app/seat 証拠を設計する。
Ubuntu の guest 利用方法など未許可の環境について WS105 の Debian 許可を自動で広げず、p001 で扱う。

## 参照（2026-10-01 確認）

- [Debian packaging workflow](https://www.debian.org/doc/manuals/debmake-doc/ch06.en.html): dpkg/debhelper の標準の入力・build 手順。
- [Ubuntu26.04 の公式配布](https://releases.ubuntu.com/26.04/): target release の確認。CI image/tag の利用可能性まではこのページで確定しない。
