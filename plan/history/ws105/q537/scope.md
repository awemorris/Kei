<!-- awesome-plan project=zedbsd record=ws105-p010 -->

# ws105-p010: libkeiland の Linux の backend（wpa_supplicant・Linux の interface・ALSA）

Status: planned
Disposition: normal
Parent: [WS105](../ws.md)
Queue: なし
依存: p008（Settings が Linux で動く）
実行者: phase-runner（high）か phase-runner-mid。**始める前に [design.md](../design.md) の §6 を読む**。`plan/tools/keiland-linux/` の script は main が merge

## 目的

決定 D14・D15。p002 の仮の backend を本物にする。`keiland.h` の API と意味は変えない（app は変えない）。

| file | 実装（design の節） |
| --- | --- |
| `libkeiland/wpa/network-wpa.c` | §6.1: wpa_supplicant の制御 socket で状態・scan・接続・切断・on/off |
| `libkeiland/linux/network-link-linux.c` | §6.2: `getifaddrs`・`/sys/class/net`・`/etc/resolv.conf`・鍵の保存（`ADD_NETWORK`・`SET_NETWORK`（ssid は 16 進）・`SAVE_CONFIG`、`ENABLE_NETWORK` はしない） |
| `libkeiland/linux/audio-linux.c` | §6.3: ALSA の control の device の ioctl で音量と mute、event、`keiland_audio_available`（手本 `survey/asnd.c`） |

wpa_supplicant の制御 socket の client は自分で書く（`wpa_ctrl.c` を読んで形を知るのはよいが複写しない）: `socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC)`、自分の側を
`/tmp/keiland-wpa-<pid>-<n>` に `bind`、相手（`/run/wpa_supplicant/<ifname>`）に `connect`、command を `send`、応答を `poll` + `recv`（上限 2 秒）。`ATTACH` した別の socket で
`<N>CTRL-EVENT-...` を受ける（`keiland_network_update` で読む。待たない）。終わりに自分の socket の file を `unlink`。権限: socket の directory の group は `netdev`。
`keiland_network_open` は socket の directory（`/run/wpa_supplicant/`）の中の最初の socket（`p2p-` で始まる物を除く）に接続する。

## 試験（guest）

### 試験用の WiFi（`plan/tools/keiland-linux/wifi-setup.sh`、guest の中で root で走らせる）

```
modprobe mac80211_hwsim radios=2                    # wlan0・wlan1
cat > /tmp/hostapd.conf <<'EOF2'
interface=wlan1
driver=nl80211
ssid=keiland-test
hw_mode=g
channel=6
wpa=2
wpa_key_mgmt=WPA-PSK
rsn_pairwise=CCMP
wpa_passphrase=keiland-pass
EOF2
hostapd -B -P /tmp/hostapd.pid /tmp/hostapd.conf
cat > /tmp/wpa.conf <<'EOF2'
ctrl_interface=DIR=/run/wpa_supplicant GROUP=netdev
update_config=1
EOF2
wpa_supplicant -B -i wlan0 -c /tmp/wpa.conf -P /tmp/wpa.pid
sleep 2; ls -l /run/wpa_supplicant/
```

（guest の image では `wpa_supplicant.service` を disable してある、design §7.2。）

### library の直接の試験（`plan/tools/keiland-linux/network-probe.c`・`audio-probe.c`、我々の libkeiland に link、guest で kei の利用者で走らせる）

`network-probe`:

1. `keiland_network_open` → update を 10 秒まで回して `reachable = 1`・`wifi` が `DISCONNECTED` か `SEARCHING`。
2. `KEILAND_NETWORK_REQUEST_SCAN` → `CHANGED_SCAN` を 20 秒まで待ち、scan に `keiland-test`（`secured = 1`）。
3. `keiland_network_save_key("keiland-test", "keiland-pass")` → `REQUEST_PROFILES` → `REQUEST_JOIN "keiland-test"` → 30 秒まで待って `wifi = CONNECTED`・`ssid = keiland-test`。
4. `REQUEST_DISCONNECT` → `DISCONNECTED`。
5. `keiland_network_get_links` に `wlan0`（WiFi）と有線の interface（`enp0s*`）、`get_saved` に `keiland-test`、`get_dns` に `10.0.2.3`。
6. `network-probe: PASS`。

`audio-probe`:

1. `keiland_audio_available()` が 1。`keiland_audio_open` → update で `reachable = 1`・`device = 1`。
2. `keiland_audio_set_volume(audio, 40, 40, 0)` → update で state の `left = right = 40`。`amixer -c 0 get Master` の割合が 40% 前後（ALSA の raw の値と割合の写しの差で ±3 を許す）。
3. 他の process（`amixer -c 0 set Master 70%`）の変更が `keiland_audio_fd` の readable と update の `CHANGED_VOLUME` で届く（`left` が 70 前後）。
4. mute の on・off（`amixer -c 0 get Master` の `[off]`・`[on]`）。
5. `keiland_audio_feedback` が 0 を返す（鳴らさない、design §6.3）。`audio-probe: PASS`。

### 手順

```
make -j64 keiland-linux && make keiland-linux-install DESTDIR=$PWD/build/keiland-linux/stage
make -j64 keiland-linux CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang
sh plan/tools/keiland-linux/elf-check.sh build/keiland-linux/stage && sh plan/tools/keiland-linux/makefile-sync.sh && sh plan/tools/keiland-linux/header-check.sh
for p in network-probe audio-probe; do
  cc -std=gnu17 -Wall -Wextra -Werror -o build/keiland-linux/stage/opt/keiland/bin/$p plan/tools/keiland-linux/$p.c -Iuserland/desktop/keiland \
     -Lbuild/keiland-linux/lib -l:libkeiland.so -Wl,-rpath-link,build/keiland-linux/lib -Wl,-rpath,/opt/keiland/lib
done
G=plan/tools/keiland-linux/guest.sh
sh $G start && sh plan/tools/keiland-linux/install-guest.sh
sh $G put plan/tools/keiland-linux/wifi-setup.sh /tmp/wifi-setup.sh && sh $G ssh 'sh /tmp/wifi-setup.sh'
SSH_USER=kei sh $G ssh 'timeout 120 /opt/keiland/bin/network-probe'
SSH_USER=kei sh $G ssh 'timeout 60 /opt/keiland/bin/audio-probe'
```

app での確かめ（compositor の上、p006 の起動の手順。PNG をユーザーに見せる）:

- Settings の Network の頁: `keiland-test` が一覧に出る screenshot、鍵を入れて接続する（QMP の `type` で打つ）→ 接続の表示の screenshot。
- system bar の network の icon が接続を表す。音量の icon で音量を変える（`click`）→ `amixer -c 0 get Master` の値が変わる。Settings の Sound の頁の slider も同じ。

## 完了の条件

1. build（gcc・clang）、`elf-check: PASS`、`makefile-sync: PASS`、`header-check: PASS`。
2. `network-probe: PASS`・`audio-probe: PASS`（guest）。design §10 の V7。
3. app での確かめの screenshot。
4. zedBSD の回帰: libkeiland の共通の file を変えていなければ build だけ（[WS104 の commands.md](../../ws104/commands.md) §1）。変えたなら design §9.2 と `settings-regress.sh`・`host-audio.sh`。
5. `mac80211_hwsim` が使えない場合は、wpa_supplicant の制御 socket の偽物の server（試験の道具、Python で書いてよい）を相手に 2 の network の部分を確かめ、そう記録する。

## 結果

（実行の後に書く）

## 実装前の接続の具体化（2026-10-01、Q1）

公開keiland.hはrequest/updateがcallerを待たせない契約である。design §6の制御socket応答2秒上限は、watchのcommandをnonblocking送信しupdateでreplyを進めるdeadlineとして実装する。ATTACH用event socketは別に持つ。STATUS/SCAN_RESULTSとJOINのLIST→ENABLE→SELECTを小さな状態遷移で進め、1request / CHANGED_DONEを維持する。save_key/get_savedはdesign §6.2の独立socketでbounded同期requestを行う。サービス消失は所有socket/pathを片付けて1秒後に再試行。4096byte超のdatagramや不正responseは切り、古いreplyと次commandを混ぜない。

`libkeiland/wpa/network-wpa.h`をprivate共有helperに追加する（Unix datagram所有、SSIDescape decode、profile照合）。linux/network-link-linux.cからも同じboundedclientを利用する。公開exports / keiland.h / appは変更しない。Linux stateのwired/WiFiを分類するprivatehelperはnetwork-link-linux.c内、/sysの無線識別とgetifaddrsを使う。public linkに新fieldを足さない。ALSAはcontrolC*をnonblock/CLOEXECで開き、infoの実min/max/channelを使ってvolumeとswitchを写し、eventをboundedでdrainしてstateを再読する。feedbackはconnectedなら0で無音、deviceの再接続は1秒間隔。

追加fileは既存scopeのOS内接続を具体化し、product / publicAPI / 受け入れ条件は不変。新Cは全文規約・formatter19・style-checkとmanualreview、実hwsim/wpa/HDA userkei試験・Settings/volume操作PNG・gcc/clang warning0・ELF/header/sourceとtargetbuildで確認する。共通sourceを変えない予定。
