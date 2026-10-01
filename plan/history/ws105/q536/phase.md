<!-- awesome-plan project=zedbsd record=ws105-p009 -->

# ws105-p009: gdm と logind（seat-logind・最小の D-Bus・pause と resume・`keiland.desktop`）

Status: cleared
Disposition: normal
Parent: [WS105](../../../ws105/ws.md)
Queue: q536 / q536-i01
依存: p008、p005（logind fdの修復）
実行者: phase-runner（high）。**始める前に [design.md](../../../ws105/design.md) の §5.1・§5.5・§5.6・§7.2（gdm）を読む**。`plan/tools/keiland-linux/` の変更は main が merge

## 目的

決定 D11・D12。gdm の session の一覧から Keiland を選ぶと、gdm が `/opt/keiland/bin/wayland` を利用者（root でない）の session として起動する。compositor は
logind から DRM と入力の device の fd を受ける（利用者には device を直接開く権限が無い）。Log Out で gdm に戻り、VT の切り替え（pause・resume）で画面が戻る。

## 手本

- `survey/dbusprobe.py`: host の system bus で通った D-Bus の client（NUL の byte、`AUTH EXTERNAL`、`NEGOTIATE_UNIX_FD`、`Hello`、method の call の marshal）。
- design §5.5 の `TakeDevice` の byte の並び（host で通った物）。

## 作る・変える file

| file | 中身 |
| --- | --- |
| `wayland/linux/dbus-linux.c`・`dbus-linux.h` | design §5.5 の最小の D-Bus の client。外に出す関数の例: `int dbus_open_system(struct linux_dbus *bus)`、`int dbus_call(struct linux_dbus *bus, const char *destination, const char *path, const char *interface, const char *member, const char *signature, const struct dbus_arg *args, size_t count, struct dbus_reply *reply)`（同期。返事を待つ間に来た signal は queue に溜める。上限 5 秒）、`int dbus_add_match(struct linux_dbus *bus, const char *rule)`、`int dbus_fd(const struct linux_dbus *bus)`、`int dbus_dispatch(struct linux_dbus *bus, dbus_signal_fn callback, void *data)`（readable のときに signal を読む）。型は `s`・`o`・`g`・`u`・`b`・`h` とそれらの struct だけ。message の最大の大きさ（例 64 KiB）を超える物・壊れた物は error で切る |
| `wayland/linux/seat-logind-linux.c` | design §5.5 の手順 1〜6（`GetSession`・`AddMatch`・`TakeControl`・`TakeDevice`・`ReleaseDevice`・`PauseDevice`・`PauseDeviceComplete`・`ResumeDevice`）と pause・resume の扱い |
| `wayland/linux/os-linux.c` | seat の選択に `logind` を足す（design §5.1 の規則）。`zwl_os_poll_count`・`fill`・`done` で D-Bus の fd を main loop に入れ、signal を seat-logind に渡す。compositor の log に `ZWL SEAT logind session=<path>` の 1 行 |
| `wayland/linux/input-linux.c` | logind の device の open・close、pause の device を読まない、resume の fd に替える |
| 共通: `zwl.h`・`display.c`（`zwl_schedule`）・`main.c` | design §5.6 の p009 の行（`os_paused`、paused の間は合成しない、resume で swapchain を作り直す、paused の間は入力の再走査をしない）。**zedBSD では `os_paused` は常に 0** |
| `wayland/linux/keiland.desktop` | design §5.5 の内容に `--wallpaper=/opt/keiland/share/keiland/wallpaper.ppm` を足した物 |
| `userland/desktop/keiland-linux.mk` | `install-session`: `install -D -m 0644 userland/desktop/wayland/linux/keiland.desktop $(DESTDIR)/usr/share/wayland-sessions/keiland.desktop`（`/opt/keiland` の外の唯一の file、D11） |
| `plan/tools/keiland-linux/guest.sh`（main） | command `gdm-setup`: guest で AccountsService の session を設定（design §7.2: `busctl call ... SetSession s keiland` と `SetSessionType s wayland`）し、`systemctl restart gdm` |

## guest での手順

```
make -j64 keiland-linux && make keiland-linux-install keiland-linux-install-session DESTDIR=$PWD/build/keiland-linux/stage
make -j64 keiland-linux CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang
sh plan/tools/keiland-linux/build-guest.sh $PWD/build/keiland-linux/guest-gdm gdm
export GUEST_DIR=$PWD/build/keiland-linux/guest-gdm GUEST_RUN=$PWD/build/keiland-linux/run-gdm SSH_PORT=2227
G=plan/tools/keiland-linux/guest.sh
sh $G start && sh plan/tools/keiland-linux/install-guest.sh && sh $G gdm-setup
sleep 30; sh $G screenshot $PWD/build/keiland-linux/p009-session.png
sh $G ssh 'ps -o user,pid,args -C wayland; loginctl list-sessions; cat /proc/$(pidof wayland)/environ | tr "\0" "\n" | grep -E "XDG_SESSION|XDG_RUNTIME"'
sh $G ssh 'journalctl -b --no-pager -u gdm | tail -20'
```

VT の切り替え（design §5.5: Ctrl+Alt+Fn は使えない）:

```
VT=$(sh $G ssh 'loginctl show-session $(loginctl list-sessions --no-legend | awk "\$3==\"kei\"{print \$1}") -p VTNr --value')
sh $G ssh 'busctl call org.freedesktop.login1 /org/freedesktop/login1/seat/seat0 org.freedesktop.login1.Seat SwitchTo u 3'    # pause
sleep 5; sh $G screenshot $PWD/build/keiland-linux/p009-vt3.png
sh $G ssh "busctl call org.freedesktop.login1 /org/freedesktop/login1/seat/seat0 org.freedesktop.login1.Seat SwitchTo u $VT"   # resume
sleep 5; sh $G screenshot $PWD/build/keiland-linux/p009-back.png
sh $G ssh 'chvt 3'; sleep 5; sh $G ssh "chvt $VT"; sleep 5; sh $G screenshot $PWD/build/keiland-linux/p009-back2.png   # force
```

## 完了の条件

1. build（gcc・clang）、`elf-check: PASS`、`makefile-sync: PASS`、`header-check: PASS`。
2. gdm の自動 login で Keiland の session が起動する: `ps` の `wayland` の user が `kei`、compositor の環境に `XDG_SESSION_TYPE=wayland` と `XDG_SESSION_ID`、compositor の log に
   `ZWL SEAT logind session=...`、screenshot に wallpaper と system bar（PNG をユーザーに見せる）。design §10 の V6。
3. 利用者の session の中で app が起動する（App Home から Terminal）。入力（key と pointer）が届く。
4. Log Out: gdm の greeter に戻る（自動 login を切って確かめる: guest の `/etc/gdm3/daemon.conf` の `AutomaticLoginEnable=false` にして `systemctl restart gdm`、greeter で kei を選び
   password `kei` を QMP の key で打って login → Keiland → Log Out → greeter の screenshot）。
5. VT の切り替え: `SwitchTo`（pause）と `chvt`（force）の両方で、戻った後に Keiland の画面が戻り（`p009-back.png`・`p009-back2.png`）、pointer と key が届く。compositor の log に pause と resume の行。
   design §10 の V11 の結果（pause で何が起きたか）を記録。
6. text console から root での起動（p006 の `seat-direct`、`KEILAND_SEAT=direct`）が今も動く（base の guest で）。
7. 共通の file を変えたので zedBSD の回帰（design §9.2、[WS104 の commands.md](../../../ws104/commands.md) の §1・§4・§5）。

## 結果

uncleared（q534-i01、未達の前提で停止）。sourceはWIP commitに保存。D-Bus AUTH EXTERNAL / UNIX_FD / Hello、bounded frame / 16FD / signal queue / method deadline5s、logind control/device/pause/resume、backend dispatch、common os_paused / old poll snapshot guard、gdm session install / guest setupを実装。gcc14.2 / clang19.1.7 build warning0、26ELF、source-sync、331header、4Linux Cのstyle-check0 PASS。

gdm専用guestを作成し、psはuser kei / XDG session、logには正しいescaped session pathを確認。しかし最初のdisplay acquireでresult=-3、frames0 error5となり自動loginが繰り返されたためgdmを停止。試験用LD_PRELOAD observerでlibraryのDRM_IOCTL_SET_MASTER（0x641e）がerrno13/EACCESと確認した。Linux drm_auth.c の drm_master_check_perm はlogindが開いた共有fdのSET_MASTERに現在processの権限を要求する。現在のkms.cは渡されたmaster fdにも無条件SET_MASTERを行い、root専用のp005試験では発見できなかった。未知の前提を発見したためこのattemptを終了し、修正をp005で選定する。p009のVT / app / LogOut / rootdirect / target回帰は未実施、clear免除なし。

証拠 original build/ws105-p009/{gcc2,clang,elf,sources,headers,session-check,drm-observe,stop-loop}.log / session-first.png（画面が出ない、受け入れFAIL）。gdm停止、Linux専用guestは修正確認のため稼働、hostにpackage追加0、host gdmは触れていない。試験observerはguest overlayだけで本番には入れない。再開条件: p005がlogindから渡されたmaster fdをSET_MASTER不要で扱えることを検証、root direct / CRTC復元の回帰PASS。その後p009同じ基準で再試行。GitHub未公開、pushなし。


実装 commit: `3400a098a66f8ae10875ca458b95d00aff01ac6d`（WIP）。終了 UTC: 2026-10-01T10:42:07.310406+00:00。GitHub は未公開、Phase / WS event と intended close は outbox に保持。

## 開始前の実装接続の具体化（2026-10-01、Q1）

実際のp006 sourceではseat-linux.hの共通Linux helperをseat-direct-linux.cが全て定義し、mainはOS dispatchより前にevdevを読む。p009のlogindを接続するため、root helperの実体をzwl_linux_direct_*へrenameし、既存zwl_linux_*のseat選択dispatchをos-linux.cへ置く。seat-linux.hに両backendのprivate関数を宣言し、Makefile.linuxにD-Bus / logind sourceを追加する。compositorの公開API、共通OS境界、rootのdeviceの扱いは不変。新しいproduct・方針の決定ではなく、既存D11/D12のOS内接続を具体化する。

PauseDeviceの処理をevdevの読みより先に行うようmainのOS poll doneを移す。inputのpaused fdはcommon入力recordのfd=-1でpollから外し、実fdとTakeDeviceの所有はlogindのrecordに残す。古いpoll snapshotからfd=-1を読まないguardをmainに追加。ResumeDeviceは新fdを同じ入力recordへ戻し、旧fdを閉じる。goneと通常closeではReleaseDevice/所有を一度ずつ返す。DRM pauseはos_pausedを立ててoutputを閉じ、windowed=0でresume後に既存zwl_schedule→enter_window_modeを通して再生成する。queued signalはsocketのreventsが0でもdispatchする。

D-Busは64KiB message / 16FD / bounded signal queue、readableになってからでもMSG_DONTWAITで読む。recvmsgは固定header16bytesとそのmessageの残りだけを読み、次のmessageのfdを混ぜない。同期callは5秒、signalを保持し、壊れたmessageとoverflowでは所有fdを閉じて失敗。device fdはCLOEXEC。Linux新moduleは全文規約で作り、実gdmのuid/VT pause/force/resume/LogOutとrootdirectの回帰で確認。

Pause の実装は既存 `zwl_compose_quiesce(server)` で in-flight frame の buffer / callback の保持を終えてから `zwl_compose_output_close` を呼ぶ。古い poll snapshot の frame event は complete 済みの状態で処理し、destroyしたswapchainの保持を残さない（既存APIの利用）。

## q534 後の再開設計

q534 の gdm 起動で kms.c の無条件 SET_MASTER が logind 共有fdに errno13 を返した。p005の「seatが渡すmaster fd」という出力を未達と判断してcurrent clearanceをinvalidated / uncleared（q528/q530は当時の結果を保持）。Q1のWS105完了までの委任された技術判断で、caller supplied fdはAUTH_MAGIC magic0の非破壊probe（current masterだけEINVAL）で検証し、SET_MASTER/DROP_MASTERはlibrary自身のdirect master取得だけに限定する。libraryはsupplied fdのdupとCRTCの復元だけを所有し、masterの制御はseat/logindへ返す。compositorにDRM ioctlは追加しない。影響はkms.cとprivate compat_displayのownership field。product/API/受け入れ目標は不変。

再検証: gcc/clang warning0・ELF/source/header・host chain/interpose/Wayland。guest rootのseat fd/direct 3色、oldSwapchain・console復元。q534のLinux logind sourceをfixture contextとして一般user gdm起動が表示できることを確認（p009のapp/VT/LogOut受け入れは次attempt）。偽の非masterfdはacquireで拒否し、callerfdはcloseされない。p009はp008と修復p005を依存として、同じ全基準で再実行。p006〜p008のroot経路で検証した受け入れは維持し、p011でfinalsourceを再確認。

[changed p005](../../../ws105/phase005/phase.md)。既存実装を保持、検証fixtureのLD_PRELOADを外して再開する。

## 再開の結果

cleared（q536-i01）。p005のlogind fd修復5012d324を前提に、p009 source3400a098と入力lease補正8b0c6ee4で全7基準を検証。gcc14.2 / clang19.1.7 build warning0、26ELF（24本体+2fixture）、makefile-sync、331source header-check、変更Linux C / DBus fixture style-check0 PASS。公開keiland/OS API不変、compositor DRM ioctl0、toolchain変更なし。

Linux QEMU Debian13 gdm専用guest: 自動loginはuser kei、XDG_SESSION_TYPE=wayland / ID171 / RUNTIME/run/user/1000、escaped logind session path、wallpaper / systembarをPNGで確認。HomeからTerminal起動とecho入力PASS。SwitchTo・chvtの両方で同一PID7648を維持、DRMと4evdevの5leaseすべてのPauseDevice/ResumeDevice、復帰画面 / pointer / echo switch-ok・chvt-ok PASS。kernel revokeがD-Bus通知より先に届く入力ENODEVはleaseを保持しEAGAINとする補正、後のResumeDevice fdへ交換を実測。途中の補正前検証は原ログに保持。

V11の実観測: このsystemd257の両コマンドはtype=force（SwitchToをcooperative pauseと捏造しない）。DRM revocation後のCRTC restoreにPermissionDeniedが記録されるが、quiesce/output閉鎖→resume/swapchain再生成はPASS。pause-type ACK分岐はsource確認のみ、実guest通知未実施。外部deviceの実機hotplug / systemd再起動は未実施。

自動loginを切り、QMPでkei/passwordを入力→Keiland userkei PID8493→Home LogOut→gdm greeterへ戻るPNG PASS。最初のpassword入力はUI遷移待ち不足で拒否、focus後同じpasswordで成功（元PNGとlogを保持）。gdm guest停止、overlay破棄。baseguestも最新版stageをinstallし、root KEILAND_SEAT=direct / --session --glassを起動、wallpaper / systembar、Home Terminal echo keiland-direct-ok PASS。SIGTERM frames62/error0/cleanup_failed0、console復元、guest停止。

D-Bus実production clientの独立wire fixture: byte分割、call中2signal queue、各frameのSCM_RIGHTS分離/CLOEXEC/payload、返信serial、fd所有移管、missing right / 64KiB超過 / partialheader+right EOF / ancillary17fd truncationの拒否と全fd回収 PASS。同じ5caseのASan/UBSanも全PASS。

zedBSD: disk-image warning0、OS boundary / GPU V1、dedicated-host / gpu-zedbsd-host ordinary+sanitize、boot-test loginPNG PASS。C1/C2/C9は全13PASS。forge-guest PASS（3imports / 120frame）、fence-guest PASS（600fences / 600frame / generation1）。全target-regression PASS。

証拠 [q536 manifest](evidence/SHA256SUMS)。PNG目視済み、代表画面は当チャットに表示。host追加package0、host画面/入力を使用せず、host /opt installなし。QEMUと実機を区別、実機未実施。BUG-125 / BUG-127は未修正tracking、前のq532 FAILを維持。GitHub publication / close はoutbox pending、pushなし。次は既存p010 network/ALSA、その後p011全文規約とWS最終受け入れ。


実装 commit: `8b0c6ee4c6a26445503b67310d1bdc9f21fefe0b`（WIP）。終了 UTC: 2026-10-01T11:23:22.269889+00:00。GitHub は未公開、Phase / WS event と intended close は outbox に保持。

## p005修復の依存確認（2026-10-01）

q535 / source5012d324でKMS借用fdをSET_MASTERなしで扱い、gdm user keiの表示、root両経路の全3色 / console復元とnonmaster拒否 / callerfd維持をPASS。再開の前提を満たす。[p005](../../../ws105/phase005/phase.md)、[q535証拠](../q535/evidence/SHA256SUMS)。p009は全7基準を変更せず再実行。q534は当時のunclearedを保持。

## q536 内部補正: revoke通知より先に届くread（2026-10-01）

SwitchToの最初の検証は画面復帰PASSだが、kernel EVIOCREVOKEのENODEVがD-Bus PauseDeviceより先にreadへ届き、common入力recordがclose / ReleaseDeviceしていた。既存「pause中はleaseを保持しresumeのfdへ交換」の基準を満たすため、Linux入力moduleは既知logind所有fdのENODEVをprivate seat helperへ渡し、入力record fd=-1 / pausedを先に記録しEAGAINとしてreadを止める。後のPauseDevice/ResumeDevice/goneが同じleaseを扱う。directのENODEVは従来どおりclose。private helperはLinux内だけ、publicOS境界・common source・受け入れ・Queue scopeは不変。再検証は同じSwitchTo/chvt、各inputのpause/resume行、KEY/POINTER、PID継続。DRMのSwitchTo通知がforceという実観測を保持し、pause扱いを捏造しない。

## q536 checkpoint（2026-10-01）

source `8b0c6ee4 WIP`、gcc/clang warning0 / 26ELF / source-sync / 331header / changedC style0 PASS。D-Bus wire fixture普通+ASan/UBSan全5case PASS。gdm自動と手動userkei login、HomeTerminal、SwitchTo/chvtの5lease pause/resume / PID維持 / 復帰入力、LogOut→greeter、baseguest rootdirect / Terminal / SIGTERMerror0cleanup0とconsole復元PASS、両guest停止済み。PNG目視し当チャットへ表示。共通変更のtarget回帰は現在8/13PASS、forge/fence後にterminal判定。q536はactive、未だclearを記録しない。実SwitchTo/chvtはいずれもforce、cooperative ACKは実guest未観測。証拠 [manifest](evidence/SHA256SUMS)。


## 2026-10-01 p011 の記録整理による検証 tool の移動

継続利用する `dbus-wire.c / dbus-wire.py` を `plan/tools/keiland-linux/` へ移した。内容は同一、公開 API・依存・受け入れ・過去の Queue outcome は不変。最終 source の再検証は [p011](../q538/phase.md) / [q538](../../queue-q538.md) に記録する。WS105 完了後はこの Phase の archive と [Linux tools](../../../tools/keiland-linux/README.md) が再開・参照先。
