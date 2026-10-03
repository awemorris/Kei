<!-- awesome-plan project=zedbsd record=ws005-p020 -->
# ws005-p020: RTL8822BU の USB passthrough でベータ1 の desktop の通し

Status: cleared（2026-10-03 user の判断。鍵の要る試験と P3 の統合後の libkeiland の join の確認は未実施のまま [WS133](../../ws133/ws.md) へ移管。q631-i02〜i04 の結果は下）
Disposition: normal
Parent: [WS005](../ws.md)
Focused goal: fg019（ベータ1）
Queue: q626 / q626-i01、q627 / q627-i01（Q1 の dispatch、2026-10-03。各時限 4 時間）、q631 / q631-i01（user 承認 2026-10-03、時限 6h、p024 と共に）
目安: 2〜3h

## 範囲

TP-Link Archer T3U Nano（RTL8822BU、`2357:012e`）または T3U Plus（`2357:0138`）を QEMU の xHCI に USB passthrough し（`plan/ws004/tests/run-rtl8822bu-passthrough.py` を使う／直す）、
現在の main（p019 を含む）の image で:

1. CLI の回帰: `net wifi set-key`・`enable`・`list`・`connect`・DHCP・`fetch`・`disconnect`・`disable`（ws005-p009/p008 の流れ、2026-09 以降の変更で壊れていないか）。
2. desktop の B1〜B4: graphical login の後、Settings と system bar から on・scan・鍵の入力・接続・`fetch`、guest の再起動の後の再接続、off・鍵の誤り・AP の不在の表示。
   有線の usb-net を同時に付けて B3。
3. 見つかった不具合は、WS005 の範囲（networkd・net・libkeiland の network）なら直し、driver なら WS004 へ、画面なら WS089/WS035 へ Bug として返す。

鍵は実行時にだけ渡し、image・log・plan・commit に残さない（ws005-p009 の Credential handling）。鍵を入れた guest の disk は試験の後に消す。

## 受け入れ

- 1 と 2 の各項目の PASS/FAIL を QMP の `screendump` の PNG と guest の disk の log（console/serial log は使わない）で記録し、PNG をユーザーに見せる。
- QEMU（passthrough）の証拠であって実機の証拠ではないと書く。

## 検証

上記の通しと、修正したときはその領域の host の試験と `plan/tools/boot-test.sh`。

## 所有 path

`plan/ws005/phase020/`、`plan/ws005/tests/`、修正するときは p019 と同じ source。`plan/ws004/tests/run-rtl8822bu-passthrough.py` を直す必要があれば main に依頼する。

## 依存

p019。ユーザーの確認: Archer がどの host（centris か 5330 か）に挿してあるか、試験用 AP の SSID と鍵を実行時に渡す方法。

## 未決の判断

上記のユーザーの確認。

## 2026-10-03 / q626: ユーザーの指示で前提が解消

ユーザー（本人の発言、Q1 経由）:「WiFiは5330ではなく、ホストに接続されたRealtek USB WiFIをアタッチして利用してみてください。」→ 開発機（centris）の
USB の TP-Link 802.11ac NIC（`2357:0138`、RTL8822BU）を local の QEMU に usb-host で渡す。「Archer の置き場所・試験用 AP の確認待ち」はこれで解消。
資格情報の利用と自動再接続はユーザーの明示の承認（Q1 の dispatch に引用）。5330 と AX211 は使わない。

## q626-i01 の結果（P1 generation4、2026-10-03 08:30〜08:45、base main `fc34e1428`）

### 用意（`plan/ws005/phase020/`）

- [config-venus-rtl.mk](config-venus-rtl.mk): ws005-p024 の Venus の desktop（本物の networkd・sessiond・Settings）+ `rtl8822b-firmware`。
  [build-rtl-image.sh](build-rtl-image.sh) で `build/p1-rtl`（worktree）に作った（exit 0）。
- [rtl-guest.sh](rtl-guest.sh): zdesktop-guest.sh と同じ Venus の guest に、別の xHCI で `usb-host`（その時の bus・address を sysfs から探す）。
  QEMU を利用者で動かすため、その device の node だけを run の間 0666 にし、stop で 0664 に戻す（戻ったことを確かめた）。host 側に
  driver は bind されていなかった（rtw88 を外す必要は無かった）。host の他の USB・network には触れていない。
- 鍵: 結果に出ないよう、guest の `net` の対話（readline、`wifi set-key` の行は history に入らない）に SSH の stdin で渡し、`net` の出力は
  捨てた（`net >/dev/null 2>&1`）。一時 file は worktree の外の `/tmp` の下の mode 700 の directory の mode 600 の file で、終わりに消した。
  鍵を入れた guest の disk（`build/p1-rtl-run/disk.img`）も消した。記録・commit・log に鍵は書いていない。

### 観測（QEMU の passthrough。実機ではない）

| 確かめ | 結果 |
| --- | --- |
| device の認識・firmware | `wlan0` が出る（MAC は記録しない）。firmware は `/lib/firmware/rtw88/rtw8822b_fw.bin` |
| scan | 25〜29 の BSS。2.4GHz の AP（channel 1、rssi -29〜-30）と 5GHz の AP（channel 40、rssi -43）が見える。2.4GHz の AP の RSN の security の印は `0x1b5`（2026-09 の q069 の evidence の `privacy+WPA2+CCMP+PSK+SAE+PMF-capable` と同じ値、WPA3 専用と判定されていない） |
| 鍵の保存（root、system の store） | `/etc/wifi.conf`（mode 600）に 2 件 |
| `net wifi enable` の自動接続 | `auto-searching` と `connecting` を繰り返して接続しない |
| `net wifi connect`（2.4GHz） | **association と 4-way handshake は通る**（`connect state=6 authorized=1`）が、その直後に `net: DHCP transaction: Input/output error (5)` で `manual-disconnected`。`wlan0` の TX は 0、RX は errors だけ増える。gdb の breakpoint で `rtl8822bu_frame_transmit` は 4 回呼ばれ全て 0 を返し、`rtl8822bu_transmit` は呼ばれない（data は `wlan_station_transmit` → `frame_transmit` の経路なので、DHCP の frame が出たかは未確定） |

### 止めたこと（権限）

DHCP の失敗の位置を確かめるため、gdbstub で wlan の送信の関数（`wlan_station_transmit`・`wlan_l2_build_data`）に breakpoint を置き、
終わりに QMP の `cont` で guest を必ず再開する trace を流そうとしたところ、**auto mode の権限の確認（「Modify Shared Resources」）に
拒否された**。指示どおり別の経路で同じ結果を作らずに止めた（その前の gdb の trace が時間切れで guest を paused に残したので、QMP の `cont` で
再開してから、guest を止めて鍵の入った disk を消した）。

### 再開の条件と次の手

- ユーザー本人が、local の QEMU の guest に対する gdbstub の breakpoint の trace と QMP の `cont`（guest の再開）を承認すること。
- 次の手: DHCP の EIO の位置（dhcpc の送信・受信、`wlan_station_transmit` の戻り、RX の復号）の特定。dhcpc は進み具合を console にだけ出す
  （`report_event` が /dev/console に書く）ので、console を読まずに追うには gdb か、dhcpc の診断を file に出す変更が要る。
- 5GHz の接続、desktop（system bar・Settings）の通し、B3、ws005-p024 の自動再接続は、DHCP が通ってから。

2026-10-03 user（明示の承認）:「local の QEMU の guest に gdbstub でブレークポイントを置き、追跡する」「最後に QMP の `cont` で guest を再開する」「これらを明示的に承認します。P1をラップアップして再起動し、権限を行き渡らせてください。」

## q627-i01 の結果（P1 generation5、2026-10-03 08:50〜10:10、base main `2b7195e7c`、commit `0c9cf7178`・`865c8d536`・`d2c2bc1d2`）

承認: このセッションのユーザー本人の発言（Q1 の起動指示に引用）「local の QEMU の guest に gdbstub でブレークポイントを置き、追跡する」
「最後に QMP の `cont` で guest を再開する」「これらを明示的に承認します。」、「WiFiは5330ではなく、ホストに接続されたRealtek USB WiFIを
アタッチして利用してみてください。」、「WiFiの資格情報を利用することを明示的に許可します。」「WiFi自動再接続を明示的に承認します。」

すべて **QEMU の証拠**（開発機の TP-Link 2357:0138 を usb-host で渡した local の QEMU、image `build/p1-rtl`）。実機の確認は未実施。

### 直したこと（4 件、どれも driver ではなく共通の層）

| # | 症状 | 原因（gdbstub で特定） | 修正 | commit |
| --- | --- | --- | --- | --- |
| 1 | authorized=1 の直後に DHCP が EIO、wlan0 の TX 0 | dhcpc の 255.255.255.255 の broadcast（SO_BINDTODEVICE wlan0）が `ipv4_output_common` で ue0 の default route の gateway 10.0.2.2 を next hop にし、`arp_resolve` が失敗（EAGAIN）、driver に届かない（`udp_sendto`→`ipv4_output_common`→`arp_resolve(rsi=0x0a000202)`=6 の trace） | 指定 device の route だけを引く `route_lookup_device_ref`（`src/kern/net/route.c`・`include/kern/net/route.h`）を `ipv4_output_common` が使う。limited broadcast は gateway に送らない（`src/kern/net/ipv4.c`） | `0c9cf7178` |
| 2 | B3: 有線（usb-net）を抜いても WiFi に default route・resolver が戻らない | networkd が有線の device の RTM_IFINFO_REMOVAL で network preference を決め直さない | `lan_l3` に ifindex を持たせ、removal で lease を忘れて `lan_work_due`（→ `apply_network_preference`）（`userland/base/networkd/main.c`） | `865c8d536` |
| 3 | 5GHz の AP への join が時々 timeout（scan の channel が 40 と 44 の間で揺れる） | `wlan_frame_parse_bss` は DS parameter が無いと受信した channel を使う。5GHz の beacon は DS parameter を持たず、隣の channel の beacon を聞くと誤った channel を記録 | DS parameter が無ければ HT Operation（IE 61）の primary channel、無ければ受信の channel（`src/kern/net/wifi/wlan-frame.c`）。修正後は 5GHz の AP が常に channel 40 | `d2c2bc1d2` |
| 4 | 接続中に別の AP へ join すると（Settings・CLI とも）成功の直後に切れ、もとの AP に自動で戻る | 成功の直後に `rtl8822bu_close`（= `ifconfig wlan0 down`、hbreak の caller）。networkd の有線の policy の `lan_snapshot` が wlan0 も有線として持ち、前の association の carrier down で `lan_take_down("wlan0")` していた | `lan_snapshot` で SIOCGWLANSTATUS に答える interface と既知の radio を除く（`lan_interface_is_radio`、`userland/base/networkd/main.c`） | `d2c2bc1d2` |

調査の道具（`plan/ws005/phase020/`）: [trace.sh](trace.sh)・[trace.py](trace.py)（gdbstub で関数の入口と戻り値を記録、`TRACE_HW=1` で hbreak、終わりに必ず
[qmp-cont.py](qmp-cont.py) で guest を再開）、[qmp-cmd.py](qmp-cmd.py)（usb-net の `device_del`/`device_add`）、[b3-observe.sh](b3-observe.sh)（guest の中で route・resolver・
`fetch` を 8 秒ごとに記録）、[qmp-type-stdin.py](qmp-type-stdin.py)（鍵を stdin から QMP で打つ）。`rtl-guest.sh` に `GUEST_CPUS`、`build-rtl-image.sh` に
`ZEDBSD_GRAPHICAL_BOOT=y`（config-venus の継承元が console 起動のため）。注意: 4 vCPU で software breakpoint を多数の hit で出し入れすると guest が panic した
（KVM の int3 の競合、hal_cpu_park）。1 vCPU か hbreak を使う。

### 確認（QEMU、RTL8822BU passthrough）

| 項目 | 結果 | 証拠 |
| --- | --- | --- |
| CLI 2.4GHz（root、`net wifi connect`） | PASS: lease、AP の gateway に ping | guest の command の出力 |
| CLI 5GHz | PASS（修正 3 の前は 1 回目 timeout・2 回目成功、修正後は切替えでも成功し安定、ping 2/2） | 同上 |
| 2.4→5GHz→2.4GHz の切替え | PASS（修正 4 の後、30 秒の間 connected のまま） | 同上 |
| system bar から join（kei、鍵の入力） | PASS: "Connected to elecom-84a0c3"、owner=1000 store=1000、ping 2/2。保存は `~/.wifi.conf`（mode 600、kei）に `auto` 付き | `build/p1-rtl-shots/d2-menu.png`・`d3-keyfield.png`・`d5-joined.png`・`e1-bar-joined.png` |
| Settings の Wi-Fi の頁から 5GHz に join | 鍵の入力と保存（`auto` 付き、"Saved"）は PASS（`s4-5g-key.png`・`s6-joined.png`）。join そのものは修正 3・4 の前の image で切れて 2.4GHz に戻った（`s5-joining.png`・`e3-settings-5g-joined.png`）。**修正後の image での Settings からの join は未実施**（同じ切替えを CLI で確認） | 同上 |
| B3（有線と WiFi） | PASS: usb-net の `device_del` で default が `192.168.2.1 wlan0`・resolver が WiFi の DNS・外部の `fetch http://example.com/` 成功、`device_add` で ue0 に戻る（修正 2 の後と修正 4 の後の 2 回） | `b3-observe.sh` の出力 |
| QMP `set_link ecm off` | usb-net の carrier が guest に伝わらない（ue0 は RUNNING のまま）。B3 の確認には `device_del` を使った | 同上 |
| build | `build-rtl-image.sh build/p1-rtl` rc=0、warning 0（`-Wall -Wextra -Werror`）を各修正で | `build/p1-rtl-trace/build*.log` |
| boot test | `plan/tools/boot-test.sh build/p1-rtl/hdd-image.img` → PASS（GPU の無い QEMU なので greeter が終わり console の login） | `build/p1-rtl-boot/login.png` |

- 既定の確認: UI（system bar・Settings）の保存は `auto` 付き。root の `net wifi set-key SSID KEY` は `auto` を付けないと `manual`（usage の `[auto]` のとおり）。
  q626 で `net wifi enable` が自動接続しなかったのは、鍵を `auto` 無しで入れたため（DHCP の EIO も重なっていた）。
- 観察: AP の SSID `elecom-84a0c3` は 2.4GHz・5GHz の各 band に 2 つの BSS を持つ（`0x1b5` = WPA2/SAE 混在と、`0x395` = WPA3 専用・PMF required）。
  AX211 の q611 で見えた `0x395` は WPA3 専用の BSS の方の可能性が高い（BUG-145 に追記）。networkd は RTL では互換の BSS を選んだ。
- 観察: 前の QEMU を接続中のまま止めた直後の最初の connect は `Connection reset by peer`（AP の reset）で、2 回目から成功した。
- 観察（WS035 の範囲、記録のみ）: system bar の network の menu の下端の「Disconnect from ...」が画面（1280x800）の外に切れる。
- 片付け: QEMU を止め、`/dev/bus/usb/001/005` の mode を 0664 に戻した（stop の表示と `stat` で確認）。鍵の一時 file（`/tmp` の下の mode 700 の
  directory）と鍵を入れた guest の disk（`build/p1-rtl-run/disk.img`）を消した。host の他の USB・network には触れていない。SSID の写る PNG は
  worktree の `build/` の中だけ（git に入れない）。記録・commit・log に鍵は書いていない。

### 未実施・残り（再開点）

1. **ws005-p024 の自動再接続の通し**（起動時の system の store、login で kei の store、logout）: Q1 のラップアップ指示（ユーザー「P1はDHCPの修正を終えたら、
   ラップアップして終了」）で未実施。手順の案: system の store に AP1 を `auto` で、kei の store に AP2 を `auto` で入れた disk を残して再起動し
   （guest.py は起動ごとに image を複写するので、鍵の入った disk を image として渡す。終わったら消す）、起動直後の `store=0`、kei の login の `sessions=1000`、
   App Home の Log Out の後の切替えを `net wifi list` と画面の PNG で見る。
2. 修正後の image で Settings から別の AP への join（修正 4 の UI からの確認）。
3. 実機（AX211）の確認はユーザー。修正 1（DHCP、有線と同時の時）・3（5GHz の channel、driver が受信の channel を hint に渡す場合）・4（AP の切替え）は
   共通の層で AX211 にも効く見込み。AX211 の q611 の「device_del の後も ECONNRESET（AP の disassociation reason 8）」は修正 1 では説明できず、未解決。

Phase の判定: 受け入れ（1 と 2 の各項目の PASS/FAIL）のうち、自動再接続（B2 の再起動後の再接続）・off・鍵の誤り・AP の不在の表示が未確認のため **uncleared**。

## 2026-10-03 user: 残りの試験の指示

「自動再接続の試験（p024）も行いましょう。Settings からの AP の切り替えも試験して修正してください。再起動後の再接続、off の状態、誤った鍵、AP が無いときの表示も試験して修正します。実機の AX211は最後に私がやるので後回しでいいです。実機を使っていないときに教えてください。また、Realtekで完璧にしてから、AXドライバの調整だけにしたいので、本当に最後に回します。」→ RTL8822BU（usb-host passthrough）で ws005-p024 の自動再接続、Settings からの AP の切り替え、再起動後の再接続、off、誤った鍵、AP 不在の表示を試験して直す。AX211 の実機は Realtek で完全にした後の最後に、ユーザーが行う（driver の調整だけ）。

2026-10-03 user の判断（q631 の途中で P1 が問うた点）: (1) WiFi の off を再起動の後も覚えるか →「覚える（推奨）」（networkd が off を /var/db に覚え、net startup が従う）。(2) 新しく打った鍵が AP に拒否された時に store から消すか →「消す（推奨）」（libkeiland の forget の API は WS131 p003 の統合の後に新しい path で）。

2026-10-03 user:「待って、鍵を消すnet wifiコマンドはないね。」→ q631 に `net wifi forget SSID`（root は system の store、一般の user は自分の store、networkd の forget の操作）を足す。拒否された鍵の削除も同じ経路。`set-key` が鍵を引数で受ける問題（ps・履歴に残る）は、stdin から読む案を P1 が返し、承認の後に実装。

2026-10-03 user（鍵の command の形、上の forget の依頼を置き換え）:「net wifi add SSID password [auto|manual|指定なし=auto] / net wifi delete SSID / net wifi modify SSID [auto=yes|no] [password=xxx] こんなので作り替えるのがいいです。」→ q631 で `set-key` を除いて add・delete・modify に作り替え、tree の中の利用者を直す。

2026-10-03 user（最終の形、上の位置引数の形を置き換え）:「じゃあ、net wifi add SSID --password password --auto yes|no この形にしましょう。--passwordが指定されない場合、パイプか標準入力で受け取りましょう。」→ `add SSID [--password P] [--auto yes|no]`（auto の既定 yes、--password を省けば stdin。tty は echo を切った prompt、pipe は 1 行）。Q1 の補い: `modify SSID [--password P|-] [--auto yes|no]`（省けば変えない、`-` で stdin）、`delete SSID`。

2026-10-03 user（add・modify の後の接続）:「net wifi enableされている状態で、つまりすでにwlan0がupしている状態で、登録済みSSIDがないためconnectしていないとき、net wifi addされたら、バックグラウンドでconnectできるかのチェックと実際の接続が走るようにしてほしいです。modifyも同様です。ここは十分に試験しましょう。」→ on かつ未接続の時、auto=yes の add、または鍵か auto=yes の modify の成功で networkd が background で scan → 見えれば join・DHCP。command はすぐ返る。接続中の時は今の接続を保つ。試験: 2.4/5GHz、誤った鍵 → modify、auto=no → modify、見えない SSID、接続中、連続、root と一般の user、繰り返し。

## q631-i01 の結果（P1 generation6、2026-10-03 10:45〜12:00、base main `5ee632781`、commit `51b798274`・`be8822ec4`）

承認: 上の 2026-10-03 user の指示（p024 の自動再接続、Settings からの AP の切替え、再起動後の再接続、off、誤った鍵、AP の不在の表示）。途中の
user の決定（Q1 経由、2026-10-03）: (1) Wi-Fi の off は再起動の後も保つ（networkd が /var/db に覚え、net startup が従う）、(2) 新しく打った鍵が AP に
拒否されたら store から消す、(3) `net wifi set-key` を `net wifi add SSID [--password P] [--auto yes|no]`・`modify SSID [--password P|-] [--auto yes|no]`・
`delete SSID` に作り替える（--password を省けば stdin・tty から読む）、(4) on・未接続の時の add・modify（auto=yes）で background の接続を走らせる。
user の指示（host の仮想メモリの状態）でラップアップ。すべて **QEMU の証拠**（開発機の TP-Link 2357:0138 を usb-host で渡した local の QEMU、
image `build/p1-rtl`）。実機は未実施。

### 鍵の要らない試験の結果（main 5ee632781 相当の image）

| 項目 | 結果 | 証拠（worktree の build/、git に入れない） |
| --- | --- | --- |
| off: system bar・Settings の表示 | PASS: "Wi-Fi is off"、wlan0 down、`state=disabled` | `build/p1-q631-shots/a2-off-menu.png`・`a5-settings-wifi-off.png` |
| off の間に join しない | PASS: system の store に auto の profile がある状態で 48 秒 `disabled` のまま | guest の `net wifi list` |
| 誤った鍵（Settings） | FAIL→修正: 文言が長い一覧の下端で見えない、鍵が `auto` で保存されたまま残る | `b3-wrongkey-4.png`・`b4-wrongkey-msg.png` |
| 誤った鍵（自動接続） | FAIL→修正: system の store の誤った鍵で connecting/auto-searching を 5〜10 秒ごとに無限に繰り返す（networkd は詰まらない） | `b1-settings-wrongkey-auto.png` |
| 誤った鍵の errno | FAIL→修正: kernel が AP の deauth を ECONNRESET で返し、他の切断と区別できない | `net wifi connect` の出力 |
| AP の不在 | FAIL→修正: 見えない SSID の join が ENOENT で、Settings が "has no saved key" と誤る経路 | 同上 |
| hidden の SSID | FAIL→修正: 全 NUL の SSID が "????????????????" と一覧に出る | `a1-menu.png` |

### 直したこと

| commit | file | 要点 |
| --- | --- | --- |
| `51b798274` | `src/kern/net/wifi/wlan.c` | 4-way の msg2 を送った後（engine が MESSAGE_2_TX・MESSAGE_3）の deauth/disassoc を EACCES（鍵の拒否）にする。他は従来の ECONNRESET |
| `51b798274` | `src/kern/net/wifi/wlan-frame.c` | 全 NUL の SSID を hidden として長さ 0 に（UI は空の SSID を出さない） |
| `51b798274` | `userland/base/networkd/main.c` | 拒否された (store の uid, SSID) を最大 8 件覚え、自動接続の候補から外す（無限の再試行を止める）。その store の PROFILES_CHANGED・その SSID の明示の join の成功・Wi-Fi off で解除。明示の join が前の接続を切った後に失敗したら auto-searching に戻す（もとの AP に戻る）。見えない SSID は ENETUNREACH。join の失敗の stage "Wi-Fi key refused" |
| `51b798274` | `userland/desktop/settings/{network.c,page-network.c,settings.h}` | EACCES "X did not accept the key. Check the key and try again."、ENETUNREACH "X is not in reach."、key の欄が開いている時は message をその直下に出す、scan が空で返った時は "No networks in reach." |
| `be8822ec4` | `userland/base/net/{wifi-conf.c,wifi-conf.h,wifi-store.c,wifi-store.h}` | `wifi_conf_add`（既存は EEXIST）・`wifi_conf_modify`（無ければ ENOENT、NULL の鍵・-1 の mode は保つ）・`wifi_conf_delete`、store の locked rewrite を edit（SET/ADD/MODIFY/DELETE）で一般化した `wifi_store_update_at`・`wifi_store_edit_for_effective_user`。`wifi_store_set_key_for_effective_user`（libkeiland の save_key が使う）は不変 |
| `be8822ec4` | `userland/base/net/main.c` | `net wifi set-key` を除き `add`・`modify`・`delete` に（usage・help・対話 console の help と history の除外も）。鍵は stdin・tty（echo を切り「Password: 」）から読める。成功の後に PROFILES_CHANGED |
| `be8822ec4` | `userland/base/net/protocol.h`・`networkd/main.c`・`net/main.c` | off の保持: 明示の DISABLE の成功で `/var/db/wifi-off` を作り、ENABLE の成功で消す。`net startup` はそれがあれば enable しない |
| `be8822ec4` | `userland/base/networkd/main.c` | 接続中の network をその store から delete したら、event loop で確かめて切断し探索に戻る（`recheck_connection_profile`）。`net wifi list` の状態の行に `ssid=HEX|-`・`rejected=UID:HEX,...|-`。失敗した Wi-Fi の制御の操作を stage 付きで stderr に 1 行（鍵・SSID は出さない） |
| `be8822ec4` | `userland/desktop/settings/network.c` | その他の join の失敗の文言に strerror を付ける |
| `be8822ec4` | `plan/ws005/tests/wifi-conf-model-test.c`・`wifi-store-test.c` | add・modify・delete の host 試験 |

### 鍵の要る試験と確認（修正の後の image、QEMU）

| 項目 | 結果 |
| --- | --- |
| build | `build-rtl-image.sh build/p1-rtl` rc=0、warning 0（`51b798274` の時点）。`be8822ec4` は net・networkd・settings を個別に build（warning 0）して guest に入れ替えて試験。**image の全 build は `be8822ec4` では未実施** |
| hidden の SSID | PASS: "????" の行が消えた（`c6-msg.png`） |
| 誤った鍵（root・kei の CLI） | PASS: `net: Wi-Fi key refused: Permission denied (25)` を 3/3 と kei で 1/1、直後に auto-searching。自動接続は拒否を覚えて再試行しない（networkd の log の "refused by the network; not tried again"） |
| 誤った鍵（Settings の join） | **FAIL（原因特定、P3 の p003 待ち）**: Settings の message は EIO。networkd は EACCES を返している（guest の disk の /var/log/networkd.err で確認）。原因は libkeiland の `network_readable`: request の後の `shutdown(SHUT_WR)` で kernel の poll が POLLERR を返す（`src/kern/net/unix-socket.c` の stream の poll、`socket.c` も同じ設計）ので即「読める」と判定し、2 秒の timed read が答えの前に時間切れ → EIO。2 秒を越える request（join）は全部 EIO。libkeiland だけの試験 client（`build/p1-q631-tools/jointest.c`）で kei から i=0・err=5 を再現 |
| CLI の add・modify・delete（root、誤った/架空の鍵） | PASS: add（--password・pipe）、重複の EEXIST と modify の案内、空の pipe・stdin 無し・短い鍵・`--password -`（add）・不正な --auto・未知の option・二重の option・値の無い option・SSID 無しの拒否（rc 1/2）、modify の --auto だけ・pipe の鍵と --auto、無い SSID の modify・delete の ENOENT、delete に option の拒否 |
| host 試験 | `wifi-conf-model-test`（新しい試験を含む）PASS。`run-wifi-conf-store-test.sh` は `test_concurrent_writers` の子が 1 回失敗（line 600、自分の版）。main の版との比較は中断（未確定。再開時にまず確かめる） |

### 未実施（再開点）

1. **P3 の p003 の統合を待つ項目**（Q1 の指示で P1 は触らない）: libkeiland の `network_readable` に MSG_PEEK|MSG_DONTWAIT の確認を P3 が入れる。統合後に新しい path で `jointest` と Settings からの誤った鍵・正しい鍵の join を確かめる。libkeiland の forget（delete）の API と Settings・system bar からの拒否された鍵の削除（user の決定 (2)、`wifi_store_edit_for_effective_user` の DELETE を使う）も統合の後。
2. kernel の poll の挙動（自分側の SHUT_WR で POLLERR）の記録: Q1 の指示で kernel は直さず ticket に記録する（新しい Bug の候補。sessiond の greeter.c・service の zsv1-client.c も SHUT_WR の後に読む — 同じ形を踏むかは未確認。net の CLI は blocking read なので踏まない見込み、未確認）。
3. `be8822ec4` の image の全 build と boot test（`plan/tools/boot-test.sh build/p1-rtl/hdd-image.img`）。
4. store の host 試験の `test_concurrent_writers` の失敗が自分の変更によるかの確認（main の `5ee632781` の版を同じ absolute path で走らせて比べる）。
5. 鍵の要る試験（未実施）: kei の CLI（`ssh kei@`、試験の guest だけ root の authorized_keys を kei に複写）と root で add の 2.4GHz・5GHz、誤った鍵の add→拒否の印→modify で正しい鍵→接続、`--auto no` の add→接続しない→`modify --auto yes`→接続、見えない SSID の add、接続中の add（今の接続を保つ）、連続の add・modify、各回の lease・ping・`ssid=`。Settings からの AP の切替え（2.4↔5GHz）、誤った鍵で別 AP へ join→もとの AP に戻る、guest の再起動後の再接続、off の保持（off で再起動→off のまま、on で再起動→on）、delete で接続中の network を消す→切断、p024（起動時の system の store、login の kei の store、logout）。鍵は stdin（`awk` で行を組み立てて `net` に pipe、または `qmp-type-stdin.py`）で渡し、store の中身を表示しない。
6. 観察（未対処）: Settings の一覧が scan の到着で並び替わり、クリックの位置の行が動く。鍵の保存の無い時も "Looking for a saved network" と出る。system bar の menu の高さ（BUG-148、wayland/network.c は P3 の移動対象）。

再開の手順: worktree `/home/awe/zedBSD-worktrees/p1`（branch agent/p1、`be8822ec4`）で `git merge main`（P3 の p003 の統合を含む）→ `sh plan/ws005/phase020/build-rtl-image.sh build/p1-rtl` → `GUEST_RUNTIME=$PWD/build/p1-rtl-run sh plan/ws005/phase020/rtl-guest.sh start build/p1-rtl/hdd-image.img` →
上の 1・3・4・5 の順。鍵は Q1 から受け取り、mode 600 の一時 file（worktree の外）に置き、終わりに file と guest の disk を消す。

片付け（2026-10-03 12:00）: QEMU を止め `/dev/bus/usb/001/005` を 0664 に戻した（stop の表示で確認）。鍵の一時 file と鍵を入れた guest の disk（`build/p1-rtl-run/disk.img`）を消した。記録・commit・log に鍵は書いていない。SSID の写る PNG は worktree の `build/` の中だけ。

## q631-i02 の途中経過（P1 generation7、2026-10-03 12:15〜、base main `c0ca07df4`、merge `ae55fdb46`）

承認: q631 の既存の承認（上の 2026-10-03 user の指示と決定 (1)〜(4)）と、2026-10-03 user「N=2に上げて、P1も再開します。」の再開の指示（Q1 経由）。

| 再開点 | 結果 |
| --- | --- |
| 3. image の全 build と boot test | PASS: `sh plan/ws005/phase020/build-rtl-image.sh build/p1-rtl` rc=0、warning 0（`be8822ec4` と main `c0ca07df4` を含む `ae55fdb46`、log は worktree の `build/p1-q631b/build.log`）。`OUTPUT=build/p1-q631b-boot bash plan/tools/boot-test.sh build/p1-rtl/hdd-image.img` → PASS（GPU の無い QEMU なので greeter が終わり console の login、`build/p1-q631b-boot/login.png`）。注意: `BOOT_TEST_WORK` を worktree の `build/` にすると QMP の screendump が 30 秒を越えて時間切れになる（boot-test.sh の注記のとおり、2 回）。既定の `/tmp` で PASS |
| 4. store の host 試験の `test_concurrent_writers` | 再現せず、自分の変更による証拠なし（修正なし）。自分の版: `run-wifi-conf-store-test.sh` 全体 3/3 PASS（build の負荷の下）、子の errno と診断を出す計装版 ordinary 40/40・sanitizer 20/20 PASS。main `5ee632781` の版: 5/5・計装版 40/40 PASS。`be8822ec4` は `wifi_store_set_key_at` を edit 経由にしたが lock（5 秒の deadline）と rewrite の経路は不変。gen6 の 1 回の失敗は host の仮想メモリの圧迫の時で、lock の deadline の超過が最もありそう（未確定） |
| 2. kernel の SHUT_WR 後の POLLERR | [BUG-149](../../bugs/BUG-149.md)（Q1 が記録、tracking、kernel の修正は未予約）。読んだ範囲: libkeiland（EIO、QEMU で再現済み）、`service/zsv1-client.c`（non-blocking、答えまで busy loop の見込み）、`sessiond` の `sessiond_greeter_release`（blocking の read で GREETER_RELEASE_MS が効かない見込み）、net の CLI（SO_RCVTIMEO で上限が効く、影響なし）。後の 3 つは読みからの判断で未観測 |
| 5. 鍵の要る試験 | 未実施（user から鍵を受け取るまで待つ） |
| 1. libkeiland の修正を使う join・forget の API | 未実施（P3 の ws131-p003 の main への統合待ち） |

## clearance（2026-10-03 Q1、user の判断）

2026-10-03 user「あまりセキュリティ回避を行うとアカウントが削除される可能性があるので、いったんclearedにして先に進めましょう。あとで手作業で確認する作業としてWSを立てておいてください。」
- q631-i02（P1 generation7）: 全 image の build warning 0、boot-test PASS、store の host 試験の失敗は再現せず（修正なし）、kernel の SHUT_WR の POLLERR を [BUG-149](../../bugs/BUG-149.md) に記録。統合 01f4c5e05。
- q631-i03・i04（generation8・9）: 鍵の複写の片付けと `wifi-key.sh`（統合 880f625c1）。鍵の要る試験は権限の判定（Credential Materialization）で着手できず。guest に鍵は入っていない。
- 検証済みでない受け入れの項目（鍵の要る項目の全て、P3 の ws131-p003 の統合の後の libkeiland の join・forget の確認）は未実施のまま [WS133](../../ws133/ws.md) の手作業の確認へ移す。この clearance はそれらの PASS を主張しない。
