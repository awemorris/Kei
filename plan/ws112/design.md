# WS112: 5種類のLinuxパッケージとCIリリースの設計

Status: 計画。実装・build・CI実行・release公開は未実施。
正本: [WS](ws.md)、[release方針](../standards/ws112-linux-packages.md)。

## 入力と現状の証拠

2026-10-02 current userが5つのmake target、ad hoc packaging可、FreeBSD source-only、CIで5種類を作成・release添付、CI runtime確認不要を指定。
同日の質問への回答でRaspberry Pi OSは64 bit / arm64に確定。
追加決定「Raspberry Pi OSについては、ビルドが通ればOKです。理由は、QEMUでGPUがエミュレーションできないからです。」を反映。RPiはbuild/deb生成を受け入れとし、GPU/GUI/表示/実機試験を要求しない。QEMU GPU制約はuser判断の理由であり、本計画で新たにGPU能力を実測したという意味ではない。

root `Makefile`にDebian/Ubuntuの2 targetがあり、`tools/release/keiland-linux-deb/run.py`を呼ぶ。
WS108はQEMU内native buildと別fresh guest runtimeを実装済み、completed。`verify-artifacts.py`はamd64/2 distro限定でsmoke JSONとPNGを必須としている。
`.github/workflows/ci.yml`の`keiland-deb` matrixとrelease gateも2 distro限定、release filesは`.deb`と3種類の付随記録。
新しいRPi/Fedora/Arch targetは存在しない。WS111の共通launcherはnative installに追加済み。
現在の実装と将来の受け入れを混同せず、変更は新WSで行う。

## targetとbuild環境

| Target | OS / CPU | Package | Build環境・確定作業 |
| --- | --- | --- | --- |
| `keiland-linux-debian` | Debian 13 / amd64（既存） | deb | 既存pinned QEMU native buildを利用、runtime工程を必須経路から分離 |
| `keiland-linux-ubuntu2604` | Ubuntu 26.04 / amd64（既存） | deb | 同上 |
| `keiland-linux-rpi` | Raspberry Pi OS / arm64（user確定） | deb | OS版/公式入力/kernel/rootfs/起動方式をp001で確定、p003で実build |
| `keiland-linux-fedora44` | Fedora 44 / x86_64（既存amd64方針からの計画案） | rpm | 公式OS入力/依存/compilerとQEMU native buildをp001/p004で具体化 |
| `keiland-linux-arch` | Arch Linux / x86_64（同上） | Arch binary package | rolling snapshot/repositoryの時点とQEMU native buildをp001/p005で具体化 |

Debian/UbuntuのQEMU build指示は保持する。新OSもQEMUで対象OSのnative compiler/system librariesを使う案とし、技術的成立性とboot検証ルールの適用をp001で照合する。
Raspberry Pi OSの実rootfsを使用し、Debian arm64をその名前で出荷しない。board用imageを汎用QEMUでそのまま起動できるとは仮定しない。
arm64のTCG build時間やnative依存不足が判明したら調査範囲内で記録し、対象OS/CPUを勝手に変更せず計画を見直す。
追加CPU版・実機GPU/WiFi/audio・FreeBSD packageは含めない。新OS入力の版/URL/hashは未確認の値を書かず、実装時に公式資料とchecksumを確認して固定する。

## 共通payload・形式

現行WS108 runtime manifestを出発点に、同じsource revisionのproduction executable/private libraries/fonts/notices/IME辞書/wallpaper/apps設定/GDM entryをstageする。
`/opt/keiland/bin/keiland-desktop`を含め、GDM Execはwayland直接起動のまま。既存test/demo/development headersの除外を維持する。
browser/xserver/EGL/GLESを新規に配布対象へ追加しない。WS106の保留ime-probeやWS110/testing、--login実装を依存にしない。
標準libc/system Vulkan等はOS依存として宣言し、ホストのライブラリを同梱しない。各OSのruntime依存名はnative ELF参照と明示的dlopen依存から検証する。

ad hocなmetadata生成とarchive圧縮を許容する。deb/RPM/Archの正式binary形式、version/CPU表記、config保持とowner/mode、dependencies/licenseを各OSの一次資料で確認する。
既存dpkg encoderの利用も可能。RPMはheader/payload等を含む正しい形式を選び、Archはpackage metadataを持つarchiveを選ぶ。
詳細encoderの選択はp001の技術判断。正式devtool一式、source package、配布署名基盤は必須にしない。
package名にOS/CPU/versionを識別できる情報を持たせ、releaseで衝突しない。deb用versionをそのまま他形式へ流用しない。
configの扱いは形式ごとに保持契約を記録する。package scriptsで自動起動やuser data削除をしない。

各成果物には既存方針のchecksum、payload manifest、Keiland JSON buildinfoを添付する。buildinfoはsource revision/dirty/hash、OS/CPU、入力checksum、compiler/toolsを記録する。
一時stage/source転送は専用buildディレクトリ、allowlist方式。共有build/toolchain/host /optを変更しない。ホストtoolchainでなく、各対象OSのnative toolchainを使う。

## CIとrelease

既存zedBSD image/Windows zipのbuildとreleaseは保持し、5 targetのjob/matrixを作る。各jobが対応make targetを呼びOS別artifactをuploadする。
QEMUやCPUの違いはjob設定に表し、CIの制限時間内にbuildできることを評価する。新OSのTCG時間が制限を超える場合はp001/p003で原因・代案を記録する。
全5 package jobの成功をreleaseの依存にし、download後も5つのOS/CPU packageが揃うことを検証する。
`.deb`/`.rpm`/Arch packageとchecksum/manifest/buildinfoをrelease filesに含め、release本文に対象OS/CPU/導入方法と動作未検証の限界を記載する。
artifact欠落・重複・checksum/OS/CPU/source不一致を失敗にする。現行smoke JSON/PNG必須検証を将来のCI契約から外す。
CIではfresh install・upgrade/remove・compositor/GUI/input/GDM起動・GPU実動作を実施も要求もしない。build guestの起動とnative buildは必要な工程であり、package導入後のruntime testと区別する。
既存runtime driverを残す場合は任意実行経路とし、make/package/release成功条件に混入させない。

## 受け入れと調査範囲

5 targetそれぞれで実際のpackageを生成し、標準query/extraction tool等でformat/metadata/CPU/path/mode/依存を独立に確認する。
package install/runtimeは今回のCI受け入れ条件にしない。破損・欠落・別OS artifactでrelease verifierが失敗する絞った試験を行う。
最終Phaseで全WS変更の全文規約/manual review、該当syntax/build/形式検証とCI既存job保持を確認する。
将来のremote Actions実行やrelease公開は実装指示とpublication権限に従う。未実施のremote実行をlocal確認から合格と報告しない。

1 Queueは1 Phase。各Phaseの予定調査上限内で未解決の環境/依存/形式を記録し、endless retryをしない。具体的command/version/input pinsはp001で確定し各Phaseへ反映する。

2026-10-02 / ws112-rpi-build-only-20261002: current userのRPi build-only受け入れを反映。影響するp001/p003/p007・WS/design・release方針を更新、Queue/実装許可は追加しない。GitHub event deliveryは保留。
