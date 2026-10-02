# q591候補: ws112-p002の有限scope/verification

Status: mainから準備を依頼された候補、未承認/未実行。Queue正本はmain所有、本文だけでは開始しない。
Observation: 2026-10-02 / q585-i01の後続準備。Parent: [p002](phase.md)、[p001契約](../phase001/phase.md)。
Timebox案: 90分、1Phase。開始時にexact scope/timebox/source/inputと最新承認をmainが保存する。

## scopeと必要出力

- Debian13/Ubuntu26.04の既存QEMU native buildを維持し、必須経路をsource→build guest→production stage→native deb encoder/独立監査→package+3sidecarsにする。
- 第2fresh guest・smoke/vk-chain-test compile/PNGを必須経路から分離。既存runtime手順/証拠は任意入口として保存し、今回実行しない。
- 共通production stage、launcher/session/test除外、全payload manifest、new buildinfo/source input/tool/OS/CPU/dependency証拠を提供。各native debのconffiles/owner/modeを維持。
- 全5jobで同じsource archive hashを得る前提としてouter gzip mtime/filenameを固定し、source allowlist・uid/gid/member epoch/dirty・専用build/lockの既存境界を維持。
- 既存2OS verifierは実deb/native metadata/独立展開/manifest/checksum/source/OS/CPUを確認し、runtime入力無しで受理。new target/encoder用の共通interfaceを定義する。

主な変更範囲候補: `tools/release/keiland-linux-deb/{run.py,build.py,verify-artifacts.py,README.md}`、共通helper追加が必要なら同release tool内、root Makefileの既存入口/help、必要なnative mkのstage拡張、p002/WS設計と証拠。
新3target実装、RPi image取得/rootfs、Fedora/RPM、Arch/makepkg、CI/release source変更は各後続Phaseで行う。
`.internal`/toolchain/HAL/共有build/host /opt、host install、push/remote Actions/releaseを含めない。C追加が必要と分かった場合は実装前にscope/全文規約を再照合する。

## prerequisitesと資源

p001 clearedと確定した[入力](../phase001/survey.md)/[形式契約](../package-contract.md)/[環境](../native-environments.md)を照合する。
開始前にcorrect branch/source revision/cleanliness、mainからのowned roots、他lane guest/resource ownership、空きRAM/disk、QEMU/SSH/cloud-initISO/native query toolsを確認。
Debian/Ubuntuは**順に**buildし8GiB guestを同時に増やさない。共有cacheのwriterをmainが調整、全途中出力はp002専用directoryとする。
既存`KEILAND_DEB_BUILD/KEILAND_DEB_ACCEL`指定を維持またはcompatible alias。全5OS artifact directoryの新default案は旧CLI消失を招かないよう具体化する。
入力は既存pin維持、実image/hash取得はこの後続Queueのexact scopeへ含める。host package不足はhostへ勝手にinstallせずmainへ報告する。

## commandと有限verification（後続Queueのみ）

```sh
KEILAND_DEB_BUILD="$PWD/build/ws112-p002" KEILAND_DEB_ACCEL=tcg make keiland-linux-debian
KEILAND_DEB_BUILD="$PWD/build/ws112-p002" KEILAND_DEB_ACCEL=tcg make keiland-linux-ubuntu2604
python3 tools/release/keiland-linux-deb/verify-artifacts.py --require-clean ARTIFACT_SET debian13 ubuntu2604
dpkg-deb --info PACKAGE.deb
dpkg-deb --field PACKAGE.deb Package Version Architecture Depends Suggests
dpkg-deb --contents PACKAGE.deb
dpkg-deb --raw-extract PACKAGE.deb DEDICATED_EXTRACT
git diff --check
```

ARTIFACT_SETは最終driver/verifierのdirectory契約に合わせ、basenameを上書きするmergeをしない。placeholderを実command/resultに置換して記録する。
guest native buildは既存all/install/install-session、native dpkg-shlibdeps/dpkg-debとreadelf CPU/SONAME/RUNPATH/system symbol version監査。
同一commit/source allowlistを2回snapshotしたSHA256が一致し、Debian/Ubuntuのsource hash/commit/dirty/epochが一致することを確認。
各実debの独立展開で全path/type/hash/mode/owner/config/session/launcherと許可prefix・test/development除外を確認し、buildinfo/manifestのchecksumを照合。
runtime JSON/PNGを与えず実2debを受理、missing/truncated debとCPU/source/checksum差異の代表各1例を拒否する。all5集合の本番否定試験はp006。
Python syntax、shell/Makefile/manualと実最終diff、必要なtool command版を記録する。計画変更をmirrorするだけのunit testは作らない。

## 結果の区別と再開

2実deb+sidecarsと共通interface/形式監査が揃えばp002基準を評価、runtime/remote実行を合格とはしない。
timebox・環境不足・build/依存/形式失敗ならuncleared、commands/results/source SHA/environment/artifact path/実経過/省略/次の具体的再開条件を保存。
clearの場合もp003〜p005/p006/p007は未実行、別exact Queueをmainが同sessionへ送るまで待機する。

2026-10-02 / ws112-q591-p002-candidate: mainが同contextで後続q591/p002のscope/verification準備を指示。上記候補を保存、Queue作成/開始/実装承認はmain所有、p002 planned/attempt noneを保持。
