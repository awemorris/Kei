<!-- awesome-plan project=zedbsd record=ws129-p011 -->
# ws129-p011: 試験の QEMU を KVM に統一し、image の build の並列の数を 16 に揃える

Status: in-progress（q646、2026-10-03、P1）
Disposition: normal
Parent: [WS129](../ws.md)

## 範囲

1. 2026-10-03 user「はい、すべてのテストで統一して、kvmを使いましょう。」
   - QEMU を直に起動する script を、guest.py と同じ条件にする。
     - `/dev/kvm` に読み書きできれば `-accel kvm -cpu host`。
     - できなければ今まで通り TCG。
     - `QEMU_NO_KVM=1`（guest.py の `--no-kvm` 相当）で TCG にできる。
   - amd64 の guest だけが対象。i386（PC-98・PC/AT BIOS）と raspi4b は TCG のまま。
2. 2026-10-03 user「-j32のビルドが複数かぶると、I/Oが追いつかなくてシステムがいっぱいになる気がします。…フルビルドは-j16にして、複数かぶってもいいようにできませんか？」
   - image と試験の build の script の並列の数を、共通の既定 16（`ZEDBSD_JOBS`、環境で上書き）に揃える。

引数と出力は変えない（T1・T2 が使っている形のまま）。

## 共通の helper

- `plan/tools/guest/qemu-accel.sh`（sh で source する）
  - `qemu_accel_mode` は `kvm` か `tcg` を出す。
  - `qemu_accel_args [MODEL]` は KVM なら `-accel kvm -cpu host`、TCG なら `-cpu MODEL` を出す（既定は max、`""` なら `-cpu` を付けない）。
- `plan/tools/guest/jobs.sh`（repository の root から source する）: `ZEDBSD_JOBS=${ZEDBSD_JOBS:-16}`。
- `plan/tools/guest/guest.py`: `QEMU_NO_KVM=1` を `--no-kvm` と同じに扱う。

## KVM に変えた script

- TCG だった物（KVM が使えれば KVM に）:
  - `plan/tools/boot-test.sh`（uefi-nvme・uefi-usb。bios-ide・raspi4b は TCG のまま）
  - `plan/ws033/tests/ssh-host-to-guest.sh`
  - `plan/ws004/tests/` の usb-overlay-boot-stress・run-legacy-hcd-qemu・run-legacy-hcd-concurrent-hotplug-qemu（本走行だけ。`-S` の parser の preflight は guest を走らせないので変えない）・qemu-nvme-io・qemu-nvme-gpt・qemu-nvme-admin
  - `plan/ws013/tests/run-uefi-zedbsd-config-ovmf.sh`（TCG の時は今の `tcg,thread=multi`）
  - `plan/ws013/tests/run-pcat-zedbsd-config-qemu.sh`（x86_64 の 2 本だけ。i386 は TCG）
  - Noct の 3 本: `plan/ws001/tests/credential-vfs-qemu.noct`、`plan/ws004/tests/qemu-usb-cdc-ecm.noct`（本走行だけ。`-S` の parser は変えない）、`plan/ws005/tests/wifi-credential-native-qemu.noct`。TCG の時は今の `tcg,thread=single`。
- KVM が必須だった物（KVM が無い host では TCG で動き、`QEMU_NO_KVM` で外せるように）:
  - `plan/tools/guest/amd64-serial.sh`
  - `plan/ws005/tests/bug149-check.sh`
  - `plan/ws073/tests/p015-hybrid.sh`
  - `plan/ws118/tests/qemu-ssh-check.sh`
  - `plan/ws005/phase018/owner-check.sh`、`phase019/prefer-check.sh`、`phase019/owner-check.sh`、`phase024/session-check.sh`
  - `plan/ws035/tests/boot-hda.sh`
  - `plan/ws100/tests/audiod-qemu.sh`
- 変えない物（理由）:
  - PC-98・i386・raspi4b の物（`pc98-boot.py`・`rpi4-serial.sh`・ws005 の peercred・ws007 の pc98・ws013 の pc98）は TCG が必須。
  - GPU・passthrough の物は KVM と host の資源が前提で、TCG では意味が無い。venus-qemu.py・i915-qemu.py（と、それを使う wayland・vkdemo・mview の qemu.py）、`-cpu host,host-phys-bits-limit=39` の hdmi/h4・bug085・run-parity-vk・ws005 の wifi-desktop、VFIO の ax211・rtl8822bu。
  - latency の 2 本は KVM の時間を測る物。
  - `keiland-linux/guest.py` は Linux の guest（WS105）。
  - `ws049 qemu-acpi-dump.py` は firmware の表を読むだけで、guest を走らせない。
  - ws029・ws075・ws099 の `*-hw.sh`・host-*.sh は QEMU を起動しない（pgrep・遠隔）。

## 並列の数を 16 にした script（37 本）

- `make -j"$(nproc)"` と `-j48` を `-j"$ZEDBSD_JOBS"` にし、root への `cd` の直後で `. plan/tools/guest/jobs.sh` を読む。
- 対象は plan/tools の build-files・build-forge・build-ssh・libm/browser-js・libm/guest-test・titlebar/build-menu、ws031 vkloop-hw、ws035 の build-*-image 5 本、ws046 bug027、ws068、ws073、ws074、ws075/demo、ws079 の 4 本、ws081 の 2 本、ws089、ws090、ws095 の 2 本、ws099 の 2 本、ws100 の 3 本、ws101 の 4 本、ws102、ws115。
- ws035 の baseline-build・refactor-build は、もとから `JOBS` の既定が 16 なので変えない。
- 例の command（`plan/tools/guest/config-amd64-ssh.mk` の注釈、`plan/tools/keiland-linux/README.md`・`zedbsd-commands.md`）を `-j16` に。
- 変えない物（報告だけ）:
  - top-level の `build-jobs.mk`（`ZEDBSD_BUILD_JOBS`、LLVM・Noct の cmake の `--parallel 64`）、`toolchain/`、`userland/packages/` の package の内部。
  - 他の WS の guide・design・`plan/standards/automation.md`・`userland/desktop/LINUX.md` の `-j64`・`-j$(nproc)` の例（担当外）。

## 確認

- `sh -n`・`bash -n`: 変えた全ての shell script で通った。Noct の 3 本は `noct --compile` が rc 0（`zedbuild.noct` を並べて）。`pushQemuAccel` の組み立ては小さな Noct で確かめた（tcg では `-accel tcg,thread=single`、kvm では `-accel kvm -cpu host`）。
- `boot-test.sh` の引数（QEMU を stub にして確かめ、QEMU は起動していない）:
  - uefi-nvme は `-accel kvm -cpu host`（`QEMU_NO_KVM=1` では `-cpu max`）。
  - bios-ide は KVM の有無に関わらず `-cpu max`。
- make の引数（`make` を stub にして確かめた）:
  - build-zdesktop-image は `-j16`。
  - build-settings-image は `ZEDBSD_JOBS=4` で `-j4`。
  - build-ssh-image（eval）は `-j16`。
- QEMU の試験（boot-test の KVM での PASS、代表の guest 試験）は T1・T2 に予約（結果待ち）。
