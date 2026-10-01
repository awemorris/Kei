<!-- awesome-plan project=zedbsd record=ws105-p010 -->

# ws105-p010: libkeiland の Linux の backend（wpa_supplicant・Linux の interface・ALSA）

Status: cleared
Disposition: normal
Parent: [WS105](../ws.md)
Queue: q537 / q537-i01
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

cleared（q537-i01）。source b55b6bf0 + Unix cleanup91343cf2 WIP。p002 placeholdersをown wpa control client / Linux interface backend / ALSA controlへ置換、公開keiland.h / exports / 共通source / appは不変。private network-wpa.hをOS内共有、外部source複写なし。commandとATTACH event socketを別に所有し、request/updateはnonblockingのreply状態遷移、各deadline2秒 / service retry1秒。STATUS・scan・profile table検証、SSIDescape decode / strongest24 / exactly-one request completion、JOINはLIST→ENABLE→SELECT、saveはhexSSID / escapedkey / SAVE_CONFIG、ENABLEせずJOIN後SAVEせず。Linux radio up/downはkernel権限の実errorを返す。

gcc14.2 / clang19.1.7 warning0、28ELF（24本体+chain/display/network/audio fixtures4）、makefile-sync、331source header-check、全新C + probeのstyle-check0 / formatter19 / whitespace PASS。ALSA card/list/info/read/write/subscribeを直接使用（alsa-libなし）、Master→PCM→Speaker、min/max/channel、readback、eventのbounded drain、device再接続。feedbackはconnectedで0 / 無音。

Linux QEMU Debian13 / real mac80211_hwsim2radio + hostapd + wpa_supplicant、HDA Master raw0〜74、一般userkei audio/netdev権限。network-probe PASS（実secured AP scan、save直後に未接続、PROFILES→JOIN keiland-test / WIFI CONNECTED+kindWIFI+IPv4、disconnect、wlan0とenp0s5 / MAC / MTU / traffic、savedprofile、DNS10.0.2.3）。audio-probe PASS（available1/reachable1/device1、40%readback、amixer41%=許容±3、別process70%→fd readable / CHANGED_VOLUME /70、mute off/on、feedback0）。解析用cache値の偽装なし。fixture固定192.0.2.2はguest setupだけ、DHCPはsystemに任せる。fake wpa fallback不要。

Settings QMP操作: scanにsecured keiland-test、key欄へkeiland-pass→Join→wpa_stateCOMPLETED / Settings Connected / systembarWiFi icon。Sound slider→amixer74%、systembar slider→amixer23%（Settings rawfloor22%）、PNG目視と当チャット表示。最初の接続3秒PNGはSCAN中の一時状態、7秒後stablePNGとwpaCOMPLETEDを最終証拠とする。最後にSIGTERM frames124/error0/cleanup_failed0。再起動時openvt7がbusyで拒否された元setup logを保存し、未使用VT8で再実行（source障害ではない）。

終了時一時directory4個残留を発見（common compositorはwatch close呼出なし）。Linux/WPA内single event-loop watch registryとlibrary/process destructorで明示closeされないwatchを同じcloseへ渡す補完。新overlayでprobe2本再PASS、Settings正常closeとcompositor SIGTERM、frames58/error0/cleanup_failed0、owned Unix path0 / console復元を再確認。SIGKILL等destructorを実行しない異常終了と多thread共有は未検証・追加対応なし。

zedBSD: Linuxだけの変更なのでPhase指定どおりdisk-image build warning0 PASS、common libkeiland不変。共通回帰一式の再実行は本Phase不要、p011 finalsource全体回帰で実施する。host追加package0、host /opt未install、host画面/input未使用、toolchain変更なし。Linux guest停止 / overlay破棄。実機・realWiFi device未実施、QEMU証拠。GitHub未公開、event/intendedcloseはoutbox pending、pushなし。次は既存p011全文規約・境界L1〜L5・両OS最終回帰・install文書・WS全体acceptance。

証拠 [q537 manifest](../../history/ws105/q537/evidence/SHA256SUMS)。


実装 commit: `91343cf27a4f6d8a5a34c238657227b30aafa03d`（WIP）。終了 UTC: 2026-10-01T11:56:09.219178+00:00。GitHub は未公開、Phase / WS event と intended close は outbox に保持。

## 実装前の接続の具体化（2026-10-01、Q1）

公開keiland.hはrequest/updateがcallerを待たせない契約である。design §6の制御socket応答2秒上限は、watchのcommandをnonblocking送信しupdateでreplyを進めるdeadlineとして実装する。ATTACH用event socketは別に持つ。STATUS/SCAN_RESULTSとJOINのLIST→ENABLE→SELECTを小さな状態遷移で進め、1request / CHANGED_DONEを維持する。save_key/get_savedはdesign §6.2の独立socketでbounded同期requestを行う。サービス消失は所有socket/pathを片付けて1秒後に再試行。4096byte超のdatagramや不正responseは切り、古いreplyと次commandを混ぜない。

`libkeiland/wpa/network-wpa.h`をprivate共有helperに追加する（Unix datagram所有、SSIDescape decode、profile照合）。linux/network-link-linux.cからも同じboundedclientを利用する。公開exports / keiland.h / appは変更しない。Linux stateのwired/WiFiを分類するprivatehelperはnetwork-link-linux.c内、/sysの無線識別とgetifaddrsを使う。public linkに新fieldを足さない。ALSAはcontrolC*をnonblock/CLOEXECで開き、infoの実min/max/channelを使ってvolumeとswitchを写し、eventをboundedでdrainしてstateを再読する。feedbackはconnectedなら0で無音、deviceの再接続は1秒間隔。

追加fileは既存scopeのOS内接続を具体化し、product / publicAPI / 受け入れ条件は不変。新Cは全文規約・formatter19・style-checkとmanualreview、実hwsim/wpa/HDA userkei試験・Settings/volume操作PNG・gcc/clang warning0・ELF/header/sourceとtargetbuildで確認する。共通sourceを変えない予定。

## q537 内部照合（2026-10-01）

実HDA Master raw範囲0〜74を確認。40%書込はnearest raw30、public stateは整数floor40、amixerはround41%（design許容±3内）、external70%はraw52→public70。stateを要求値cacheで偽装せず実readbackを使う。実hwsim+hostapd/wpa、userkeiでnetwork/audio-probe PASS、Settings scan・QMP鍵入力→wpa_stateCOMPLETED、Sound slider74% / amixer74%を観測。最終照合でtext responseのembeddedNUL / 不正STATUS/table headerを拒否、24を超えるscanは最強24を保持、disconnect時は古いWiFi/scanを捨てる補完。範囲・公開API・条件不変、gcc/clang warning0 / style0、最新版で同じprobeとUIを再確認中。共通source / app変更なし。

## q537 cleanup の補完（2026-10-01）

最新版UI scan・key→COMPLETED・Settings74% / bar23%・SIGTERMerror0cleanup0を確認。ただしguest /tmpに4つのprivate directoryが残った。compositor共通network.cは終了時のwatch close APIを持たず、Linuxのpathnameだけが残る。未知の外部依存ではなく本PhaseのUnix path所有を最後まで扱う内部補完として、WPA moduleのsingle event-loop watch registryを追加し、explicit closeで除去、library/process destructorで残るwatchを同じclose関数へ渡す。gcc/clangのreserved __attribute__を用い、共通source/APIを変えない。SIGKILLではdestructorが走らずpath残留可能という一般の異常終了は未対応。新しいguest overlayでprobeとcompositor通常終了を再確認し、private path0を要確認。最初の残留を示すcleanup.logは保存する。
