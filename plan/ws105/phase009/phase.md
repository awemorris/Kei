<!-- awesome-plan project=zedbsd record=ws105-p009 -->

# ws105-p009: gdm と logind（seat-logind・最小の D-Bus・pause と resume・`keiland.desktop`）

Status: planned
Disposition: normal
Parent: [WS105](../ws.md)
Queue: なし
依存: p008
実行者: phase-runner（high）。**始める前に [design.md](../design.md) の §5.1・§5.5・§5.6・§7.2（gdm）を読む**。`plan/tools/keiland-linux/` の変更は main が merge

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
7. 共通の file を変えたので zedBSD の回帰（design §9.2、[WS104 の commands.md](../../ws104/commands.md) の §1・§4・§5）。

## 結果

（実行の後に書く）

## 開始前の実装接続の具体化（2026-10-01、Q1）

実際のp006 sourceではseat-linux.hの共通Linux helperをseat-direct-linux.cが全て定義し、mainはOS dispatchより前にevdevを読む。p009のlogindを接続するため、root helperの実体をzwl_linux_direct_*へrenameし、既存zwl_linux_*のseat選択dispatchをos-linux.cへ置く。seat-linux.hに両backendのprivate関数を宣言し、Makefile.linuxにD-Bus / logind sourceを追加する。compositorの公開API、共通OS境界、rootのdeviceの扱いは不変。新しいproduct・方針の決定ではなく、既存D11/D12のOS内接続を具体化する。

PauseDeviceの処理をevdevの読みより先に行うようmainのOS poll doneを移す。inputのpaused fdはcommon入力recordのfd=-1でpollから外し、実fdとTakeDeviceの所有はlogindのrecordに残す。古いpoll snapshotからfd=-1を読まないguardをmainに追加。ResumeDeviceは新fdを同じ入力recordへ戻し、旧fdを閉じる。goneと通常closeではReleaseDevice/所有を一度ずつ返す。DRM pauseはos_pausedを立ててoutputを閉じ、windowed=0でresume後に既存zwl_schedule→enter_window_modeを通して再生成する。queued signalはsocketのreventsが0でもdispatchする。

D-Busは64KiB message / 16FD / bounded signal queue、readableになってからでもMSG_DONTWAITで読む。recvmsgは固定header16bytesとそのmessageの残りだけを読み、次のmessageのfdを混ぜない。同期callは5秒、signalを保持し、壊れたmessageとoverflowでは所有fdを閉じて失敗。device fdはCLOEXEC。Linux新moduleは全文規約で作り、実gdmのuid/VT pause/force/resume/LogOutとrootdirectの回帰で確認。

Pause の実装は既存 `zwl_compose_quiesce(server)` で in-flight frame の buffer / callback の保持を終えてから `zwl_compose_output_close` を呼ぶ。古い poll snapshot の frame event は complete 済みの状態で処理し、destroyしたswapchainの保持を残さない（既存APIの利用）。
