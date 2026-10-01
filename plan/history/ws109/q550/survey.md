# WS109 / q550: FreeBSD15 native port survey

Date: 2026-10-02 JST。Source: 681b1566（WS108 completed）。[source inventory](source-inventory.json): Linux make の331 source memberships /321 unique files。調査は実source・公式FreeBSD headers/driver/manualで行った。FreeBSD guestの実行結果ではない。

## 固定候補 / 取得の証拠

FreeBSD **15.1-RELEASE / amd64 / UFS / BASIC-CLOUDINIT qcow2**を15.x内の初期固定targetに選定。公式compressed image664729340bytesのSHA256は `e4ca4db889f8559c9b9dfcacc70405c038476f4b6d41649b152d3809a2ed9e1f`、実download/hash照合PASS。xz展開とqemu-img info PASS、base6GiB / own20GiBoverlay、8GiB RAM /4vCPU /KVM、virtio-vga /USB keyboard-tablet /vtnet /HDA候補。
[image inputs](image-input.json)、[公式checksum](CHECKSUM.SHA256)、[host](host-environment.json)、[未起動guest設定](../../../ws109/guest-plan.json)。base/overlay/seed/keyはown `build/ws109-control/`、秘密鍵はgit/証拠へ入れない。SSH公開鍵のみ、hostfwdは127.0.0.1限定。記録port47969は準備時の値、起動直前に競合を確認する。guest boot/SSH/pkg/native compileは**未実施**。

[FreeBSD source inputs](source-inputs.json)はreleng/15.1のcommit `e38a7085f3a8ecd317c947591fe42b5ca9ab317b`に固定した11headers。image内headersとの同一性は未確認、参照sourceとruntimeを混同しない。drm referenceはtag drm_v6.6.25_13 /commit `11252e8b9074218848abe3195601acad655a2e26`。他の公式参照は[hash台帳](reference-hashes.json)。外部実装はsource treeへ取り込まない。

## Linux → FreeBSD の変更対応表

| Component / source | 現行sourceの事実 | native方針 / 必要な実検証 |
| --- | --- | --- |
| native build / keiland-linux.mk | native cc GNU make rules、systemheaders + isolated compatibility headers、/opt staging。multiarch/dpkg/install -D/GNU digestcmdはLinuxの前提 | 独立keiland-freebsd.mkとpackage Makefile.freebsd。base clang/lld、gmake、/usr/local/include・lib、-pthread、libutil、private /opt。BSD install mkdir明示、sha256 tooling分離。既存Linux/zedBSD規則を再利用includeしない。clean build/warning0/DESTDIR/header closure/ELFの実検証をF2へ |
| renderer / composition / libkeiui | 共通Vulkan calls/paint/layout、OS backend selected by Makefile | 同じsourceをbuild、rendererを複製しない。Vulkan standard public client +3OS affected checks |
| libvulkan-compat/backend.c | absolute dlopen / dlsym、自身のload拒否。RTLD_DEEPBINDはifdefで既に任意 | FreeBSD dlfcn.hにもRTLD_DEEPBINDあり（glibc固有とは限らない）。後段/usr/local/lib/libvulkan.so.1とFreeBSD rtldで実chain/同名symbol recursion否定が必要。headless query aloneはdisplayの証拠でない |
| libvulkan-compat/wsi-swapchain.c、wayland/linux/gpu-linux.c | DMA_BUF import/export sync_file + actual kernel fd。protocolは標準zwp_linux_dmabuf_v1、CPUwaitfallbackも有る | drm-kmod BSD UAPIに同名struct/ioctl、実case handlersがある。native ioctl encodingとheadersを用い、Linux番号の流用禁止。OS sync wrapperをLinux/FreeBSDごとに置く。dma-buf protocol/import共有はmechanism moduleへ。実GPU/driverでfd ownership/actual producer-consumer fence/pixelsを確認 |
| libvulkan-compat/kms.c / wsi-display.c | private compat uses DRM UAPI + standard Vulkan display front | BSD libdrm UAPIを使える候補。表示/master/mmap/ioctlのABI、CRTC restorationを実deviceで照合。QEMUconsole成功からDRM表示成功を推論しない |
| wayland/linux/os-linux.c /seat | Linux KD/VT/logind device ownership、root directとlogind | FreeBSD sys/consio VT/ioctl +libseat/seatd候補。seatdのnonrootopen/disable→fd解放→enable/VT復元とearlyfailcleanupを実検証。Linuxlogind/DBusを持ち込まない |
| wayland/linux/input-linux.c /zwl-evdev.h | evdev device capabilities、clock/grab、timestamp/common input | FreeBSD dev/evdev/input.hにinput_event/EVIOCSCLOCKID/EVIOCGRABがある。native ioctl types/device ownershipをFreeBSD moduleへ。pointer/keyboard/tablet/touchの実event操作をF3へ。include-selectorの細かいOS境界は既存Guardrailに従う |
| libkeiland/linux/audio-linux.c | ALSA controlのみ、PCM無し。volume/mute/enumeration/update/reconnect、feedback silent | FreeBSD sys/soundcard +base libmixer/mixer.hでnative enum/volume/mute。borrowed poll fdの意味を偽らず、mixerがevent非対応ならfd=-1 +periodic updateで通知。F4の列挙/音量/muteを保持。PCM追加は既存F4に無いので初期portに追加しない（Linux parity）。QEMUHDA real ioctl/save-restoreで検証、音の聴感/実機は別 |
| libkeiland/linux/network-link-linux.c | AF_PACKET +/sys counters/WiFi分類、SIOCGIF*、IPv4/DNS、supplicant credentials | AF_LINK/sockaddr_dl/if_data counters、IFT_IEEE80211/nativeflags、sysctl/net80211。IPv4/DNSはpubliccontractを維持、DHCPはFreeBSDsystem serviceの所有。vtnet実state/counter/permissions、nativeWiFi/realadapterでscan/join/disconnect/persist |
| libkeiland/wpa/network-wpa.c | commonwireだがcontrol directory /run/wpa_supplicant を固定、Unix sockaddr layout | FreeBSD /var/run/wpa_supplicant、sockaddr_un.sun_lenの必要性を実socketで確認。OS path/addresshelperをmodule化し共通wire維持。mockparser/control serverの合格をrealradioの証拠にしない |
| terminal/main.c | pty.h / forkpty等と/bin/sh | FreeBSD libutil.h /-lutil。OS選択header/moduleでcommontermparserを共有。実ptykeyboard/fileeffect/terminalchildcleanup |
| Files tags/info/task | sys/xattr.h、*get/set/list/remove xattr。listはNUL名列、user.keiland.tags。copymetadataも対象 | FreeBSD sys/extattr +native adapterが必要。user namespace/lengthprefixed name lists→publicNULlist、fd/link semantics、ENOATTR→既存error、overflow/unsupportedfsを明示。偽成功でtagsを失わせない。UFSのnative roundtrip/copy/linkcontractとLinux/zedBSD回帰 |
| common Unix APIs | accept4/CLOEXEC/NOSIGNAL/stat.st_mtim等 | API名の有無だけでABI互換を主張しない。actual native compile/runtimeをp002以降で確定 |

## 判明した判断 / 未充足F1

**D1 / graphics license:** ports graphics/drm-66-kmod Makefileは `LICENSE=BSD2CLAUSE MIT GPLv2` /multi。WS/F-065のGPL無し構成と矛盾するため、既存system driver利用をユーザーへ質問した。Keiland/source/独自WSIの寛容licenseとsystemdriverlicenseを区別する。GPL無しを維持する場合、別GPU/driver/display方式の実現性を再設計し、同期や描画publiccontractを黙って置換しない。

**D2 / boot method:** AGENTSの例外はLinuxのみ。FreeBSD専用guest設定を準備して、WS109に限定したloopback SSH/QMP PNG例外をユーザーへ質問した。boot未実施、boot-test.shをFreeBSD imageへ無断で置換/拡張しない。承認後にGuardrail/AGENTS/Phase/toolreferencesを記録する。

**D3 / real devices:** 実WiFiのscan/join/disconnect/configはF4の受け入れであり、利用できるFreeBSD15実機/adapterを質問した。QEMUにはrealWiFiを構成していない。graphicsについてもbase virtio_gpu.cはvt_allocate/fb_infoのconsole向けでDRM node/PRIME/exportSync/Vulkandisplayを提供する証拠が無い。headless lavapipe/nativebuildと実表示を分け、Intel/AMD DRM device（license回答次第）等の検証環境を確定してF3を実施する。guest画面だけでF3/F4達成としない。

## dependent verification sequence（まだ実行しない）

1. D2解消→native guest uname/freebsd-version/cc/headers、actual pkg dependency/license、OS version/ABIを保存。SSH接続+QMPPNG確認、PNGをユーザーへ提示。console/serial null。pkg installはown guestだけ。
2. D1/device解消→p001実environmentの対応表を完成、p001を再attempt。p002nativebuild/install/ELF/publicVulkanchain、必要ならscopeをPhaseに分割しoriginalIDsを保つ。
3. p003実display/sync/seat/input/VT/fdrelease。system Vulkan/graphicsの実deviceとlicenseを固定して行う。
4. p004 OSS real mixerとvtnet/native links +actual WiFi。mockはcontractのみ、realWiFi受け入れの代替にしない。D3が無い場合は受け入れを勝手に軽くしない。
5. p005全WSsource/C全文review（WS106/107移動例外を使わない）、affectedLinuxclean/native +zedBSDbuild/bootPNG、nativeFreeBSD主要app操作。未実施/制限/3OS結果を記録しWS自身F1〜F5を判定。

## Commands / failures / limitations

`make -f userland/desktop/keiland-linux.mk print-sources` exit0。curl公式image/checksums/headers、SHA256 Python hash照合、xz展開、qemu-img info/ownedoverlay作成、xorrisoNoCloudseed作成 exit0。初期drm header path include/uapi/linux/dma-buf.hは404、その後tagtreeを実取得してBSD UAPIの正しいpath linuxkpi/bsd/include/uapi/linux/dma-buf.hを確認。source membershipsには共通重複10があり、全331がuniqueとの仮定を捨て321uniqueとして台帳を保存。

Production source/config、toolchain、CI、otherWSの実装は変更無し。make check禁止を保持。guest/native compile/Vulkan表示/audio/WiFi/zedBSD再bootは未実施。phase調査をport完了としない。必要な質問の回答が無いのでF1環境/ライセンス/実検証条件は未確定。GitHub publication/comment/Projectはlocal outbox pending。

## Primary references

- [15.1 official VM images](https://download.freebsd.org/releases/VM-IMAGES/15.1-RELEASE/amd64/Latest/)
- [15.1 native loader manual](https://man.freebsd.org/cgi/man.cgi?query=dlopen&sektion=3&manpath=FreeBSD+15.1-RELEASE)
- [15.1 mixer API](https://man.freebsd.org/cgi/man.cgi?query=mixer&sektion=3&manpath=FreeBSD+15.1-RELEASE)
- [15.1 cloud seed API](https://man.freebsd.org/cgi/man.cgi?query=nuageinit&sektion=7&manpath=FreeBSD+15.1-RELEASE)
- [drm fixed release](https://github.com/freebsd/drm-kmod/releases/tag/drm_v6.6.25_13)
- [ports DRM license](https://github.com/freebsd/freebsd-ports/blob/main/graphics/drm-66-kmod/Makefile)
- [native Wayland permissions guidance](https://docs.freebsd.org/en/books/handbook/wayland/)

References/headeravailability do not establish guestbehavior. Record actualcommands/results after permission and device prerequisites are resolved.
