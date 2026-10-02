# q574 / FreeBSD physical native build and install

Outcome: cleared。WS109はincomplete、F6の最後のユーザー実操作受け入れ待ち。

## Environment and actual commands

- Host awe@10.0.30.3 / ~/zedBSD、FreeBSD15.1-RELEASE amd64、Intel Tiger Lake Iris Xe8086:9a49、既存i915kms/drm66。SSH hostkey変更をユーザー確認済み、専用known_hostsで検証。passwordless sudo。
- `git pull --ff-only origin main`成功、実machine source 54370f9b。開発hostの9ff894ff/54370f9bはWIP、明示承認の非force push成功。最後に結果/docs commitもpush/pullする。Issue/Project公開とは別。
- `sudo pkg install -y gmake python3 meson ninja vulkan-headers vulkan-loader libdrm mesa-dri seatd`成功。Clang19.1.7/base libc、Python3.12.14/Meson1.10.2、GNUmake4.4.1、Mesa26.1.3、libdrm2.4.133。zedBSD toolchain build無し。
- 実 `make -j8 keiland-freebsd` 全build成功、compiler/linker warning0/error0。実 `sudo make keiland-freebsd-install` 成功、/opt/keilandへ配備。初回buildのfull logは開発host `build/ws109-control/physical-build-install.txt`（disposable）。
- 最終 `gmake keiland-freebsd ZEDBSD_CONFIG=/nonexistent` と `make keiland-freebsd` 成功。GNU入口がcross-toolchain realpath -mを誤評価する問題をGNUmakefileのnative-only dispatchで修正し再確認。toolchainそのものの変更なし。
- `gmake -j8 -f userland/desktop/keiland-freebsd.mk header-dependencies` + `python3 plan/tools/keiland-freebsd/native-build-audit.py build/keiland-freebsd /opt/keiland`: PASS 334source memberships/324unique C/630headers/11private shared ABI。native system headers/libc、SONAME/NEEDED、公的headers、publicDRM混入なし。
- 全14 installed executablesはbuild成果物とbyte一致、ldd欠落0。waylandの引数拒否はexpectedexit2で実loader/startup確認、seat/deviceを取得しない。wallpaper/font/AppHome配備済み。
- `pw groupmod video -m awe`、`sysrc seatd_enable=YES`、service起動成功。serviceのpidfileはroot専用なのでsudo onestatusで実pid7864をread-back。SSH起動のstdioを引き継がないようownserviceをstdio/tmp redirectedでrestart、初回SSHは正常終了。/var/run/seatd.sock root:video0660、aweの新login group44確認。
- runtime /home/awe/.cache/keiland-runtime awe:wheel0700。実DRM card0存在。hostdriver/reboot/network設定/GDMは変更せず。

## Full conformance / limits

[最終source全文review](source-review.md)、[最終read-back](physical-final-readback.txt)、[全executable](physical-executable-check.txt)、[environment](physical-environment.txt)。C差分0、追加make/docs全部を全文確認、該当GNU/BSD buildとnative header/ELF検証、git diff --check PASS。QEMU/zedBSD bootをphysical GUI結果に代替せず。GUI/実入力/物理音声/WiFi/userapp操作は今回agent未実施。ユーザーがlocalconsoleでKeilandを使い動けばF6受け入れ合格。GDM起動はユーザー撤回。

## User procedure

実機のコンソールでいったんlogoutしてaweでloginし直す（video group反映）、以下を通常userで実行する。

```sh
env XDG_RUNTIME_DIR="$HOME/.cache/keiland-runtime" \
    WAYLAND_DISPLAY=wayland-keiland \
    /opt/keiland/bin/wayland --session --glass \
    --wallpaper=/opt/keiland/share/keiland/wallpaper.ppm
```

AppHomeのLog Outで通常終了。--sessionなしの既定は150秒期限。コンポジタのgreeter画面は存在するが、認証/ユーザー切替はzedBSD sessiondのchannelが必要でnativebuildには無い。consoleで先に認証し--sessionでdesktopを継続。native直接sessionのpasswordlockもchannelが無いためinactive。この未実装を隠してGUIlogin実施済みとはしない。
