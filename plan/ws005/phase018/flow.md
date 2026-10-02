<!-- awesome-plan project=zedbsd record=ws005-p018-flow -->
# ws005-p018: WiFi の利用者の流れ（調査と契約の案）

2026-10-02、P1（q596-i01）。main `a768b804c` の source を読んで書いた。source は変えていない。
「（読み）」は source を読んだだけの結論で、走らせて確かめていない。

## 0. 読んだ範囲

`userland/base/init/services/{networking,networkd}`、`userland/base/etc/{rc.conf,group}`、`userland/base/net/main.c`（startup・wifi・set-key）、
`userland/base/net/wifi-store.c`（store の場所）、`userland/base/networkd/main.c`（WiFi の request・owner・自動の接続・再接続・radio の列挙と選択）、
`userland/base/networkd/managed-wlan.c`・`managed-lan.c`、`userland/base/dhcpc/main.c`（route・resolver）、`src/kern/net/route.c`（default route の重複と探索）、
`userland/desktop/settings/network.c`、`userland/desktop/libkeiland/zedbsd/network-zedbsd.c`・`network-link-zedbsd.c`、`userland/desktop/wayland/network.c`、
`userland/desktop/sessiond/`（login の時に network の操作があるかを grep）。

## 1. 現在の流れ

### 1.1 起動

1. init が `networkd`（daemon、root）を起こす（`userland/base/init/services/networkd`）。socket は `/run/networkd.sock`（root:network、0660）。
   `kei` は `network` group（`userland/base/etc/group:2`）。
2. init が `networking`（oneshot、root）で `/sbin/net startup` を走らせる（`userland/base/init/services/networking:1-4`）。
3. `startup_command`（`userland/base/net/main.c:2006`）は LAN の方針を送り、別 process で `NETWORKD_OP_WIFI_ENABLE` を送る（`main.c:2018`）。
4. networkd の `wifi_request_enable`（`userland/base/networkd/main.c:3155`）は送り手の euid（root=0）の store を読み（`load_policy(peer->euid, ...)`、
   `main.c:3164`。root の store は `/etc/wifi.conf`、`userland/base/net/wifi-store.c:17-18`。無ければ ENOENT を受け入れる）、
   **方針の所有者を root にする**（`networkd_managed_wlan_enable(&managed_wlan, peer->euid)`、`main.c:3183`）。状態は auto-searching。
5. 自動の接続（`main.c:1798-1830` 付近）と切れた後の再接続（`main.c:1686-1689`）は、どちらも **所有者の store**（`load_policy(managed_wlan.owner_uid, ...)`）だけを読む。

### 1.2 login した利用者（kei）が Settings で鍵を入れて join

1. `se_network_join_key`（`userland/desktop/settings/network.c:238`）が `keiland_network_save_key` で **利用者の store** `~/.wifi.conf` に `auto` 付きで書く
   （`userland/desktop/libkeiland/zedbsd/network-link-zedbsd.c:193`、`wifi_store_set_key_for_effective_user(..., 1, ...)`）。
2. `PROFILES` を送る（`NETWORKD_OP_WIFI_PROFILES_CHANGED`、`network-zedbsd.c:287-288`）。networkd の `wifi_profiles_changed`（`main.c:3612`）は
   **送り手が所有者でなければ何もせず成功を返す**。所有者が root なので kei の通知は捨てられる（読み）。
3. 成功の返事で Settings は `JOIN` を送る（`settings/network.c:404-409`、`NETWORKD_OP_WIFI_CONNECT`）。`process_wifi_request` は CONNECT に
   `owner_allowed(peer)` を課す（`main.c:3339-3341`）。`owner_allowed`（`main.c:5021`）は root と、状態が disabled のときと、所有者だけを通す。
   **状態は auto-searching、所有者は root、kei は root でない → EPERM**（読み）。
4. Settings は EPERM に「Wi-Fi was turned on by another account. Turn it off and on to join」を出す（`settings/network.c:424`）。
5. しかし kei の「off」（`NETWORKD_OP_WIFI_DISABLE`）も `owner_allowed` で EPERM（同じ理由）。スイッチは状態が auto-searching の間「on」に見え
   （`network-zedbsd.c:881-888`、system bar も `wayland/network.c:615-617` で on のとき off を送る）、利用者は ENABLE を送る操作を持たない。
   **結論（読み）: 起動直後の利用者は、Settings・system bar から自分の鍵で join できず、案内の「off して on」も off で拒まれる。** B1 を満たさない。
   - 抜け道（読み）: CLI の `net wifi enable` を kei として打つと、ENABLE は所有者の検査を受けないので（`main.c:3339-3341`）所有者が kei に移り、kei の store で動く。
6. 偽の networkd を使った ws035-p013・ws089-p003 の試験は所有者を模していないので、この不一致を捉えない。

### 1.3 system bar（`userland/desktop/wayland/network.c`）

- WiFi の on/off（`:615-617`）、scan、保存済みの profile への join（`:621`）、disconnect。**鍵の入力は無い**（鍵の無い SSID の join は ENOENT で
  「no saved profile」、`:182-183`）。それ以外の失敗は `strerror` の文（`:184-186`）。所有者の不一致の EPERM も「Could not join (Operation not permitted)」になる。
- Settings と同じ libkeiland の request（`network-zedbsd.c:272-288`）と同じ状態の名前を使う。理由の文言が両者で違う（Settings は ENOENT/EPERM/その他を区別）。

### 1.4 再起動の後（B2）

- 利用者の鍵は `~/.wifi.conf` にだけある。起動の ENABLE は root が送るので root の store（`/etc/wifi.conf`）しか読まれず、login の時に利用者として ENABLE を
  送る仕組みは無い（sessiond・compositor に WiFi の操作は無い、grep）。**再起動の後に利用者の network へ自動で戻らない**（読み）。

### 1.5 有線と WiFi が同時（B3）

- 有線の名前の無い interface は既定で DHCP（`managed-lan.c:586-611`、`wlan*` は除く）。WiFi は接続の後に networkd が dhcpc を走らせる。
- dhcpc は lease の router を **metric 無しの default route** として `SIOCADDRT`（`dhcpc/main.c:380-389`）。kernel は network・netmask・device が同じ
  ものだけを置き換え（`src/kern/net/route.c:118-123`）、別 device の default は並ぶ。探索は最長一致で、同じ長さなら表の先の slot（`route.c:311` の `>`）。
  → **どちらの default が使われるかは追加の順と slot の再利用で決まり、方針が無い**（読み）。
- resolver は dhcpc が `/etc/resolv.conf` を rename で**後勝ち**で書く（`dhcpc/main.c:1052`）。WiFi の切断は、自分が書いた内容と byte が同じときだけ
  resolv.conf を消す（`managed-wlan.c:376-387`）。WiFi が後に書いていた場合、WiFi を外すと resolv.conf が消え、有線の DNS は次の更新まで無い（読み）。
- 有線を抜いたときの default route は device の消滅で消える見込み（未確認）。resolver の持ち主は記録されない。

### 1.6 二つの radio（AX211 と RTL8822BU）

- networkd は kernel の interface の順（安定）で WLAN を列挙し（`main.c:4644`）、手動の join は目当ての SSID が見えた最初の radio を使う
  （`select_manual_radio`、`main.c:5285`、速さで選ばない）。名前 `wlan0/wlan1` は attach の順（AX211 は起動時の PCI、USB は後）。
- 利用者には SSID だけが見え、どの radio かは Settings の interface の一覧（links）で見える。選ぶ操作は無い。ベータ1 では今の規則で足り、文書に書くだけでよい。

### 1.7 本物の radio での試験と秘密の扱い（B1〜B4 の証拠）

- RTL8822BU: `plan/ws004/tests/run-rtl8822bu-passthrough.py`（USB passthrough）。AX211: `plan/ws004/tests/run-intel-ax211-vfio-qemu.sh`（BUG-134 の解決待ち）。
- 試験の AP の SSID/鍵は ws005-p009 の Credential handling に従う: plan・source・image・log・commit に書かない。guest には実行時に serial か SSH の
  対話で入れ、画面の PNG に鍵が写らないようにする（Settings の鍵の field は伏せ字か要確認）。retained の証拠は SSID を伏せる。

## 2. B1〜B4 ごとの足りない点

| # | 足りない点 | 根拠 |
| --- | --- | --- |
| B1 | 起動の root の enable の後、利用者の join（Settings・system bar）は EPERM。案内の off も EPERM。利用者の `PROFILES_CHANGED` は捨てられる | 1.2 |
| B1 | system bar には鍵の入力が無い（鍵の無い network は Settings で入れる、で足りるかはユーザーの判断） | 1.3 |
| B2 | login の時に利用者の store を有効にする仕組みが無い。再起動の後は root の store だけ | 1.4 |
| B3 | 二つの default route の優先の方針が無い。resolver は後勝ちで、WiFi の切断で消えうる | 1.5 |
| B4 | 理由の文言が Settings と system bar で違う。所有者の不一致の文言は正しい操作を案内しない。鍵の誤り・AP の不在は networkd の stage（`unknown Wi-Fi profile`・`Wi-Fi SSID not visible`・`wifi connect`・`DHCP transaction`）が errno に潰れて画面に届く | 1.2、1.3、`main.c:3245-3310` |

## 3. 案の比較（利用者の鍵と再起動の後の自動の再接続）

| 案 | 内容 | 利点 | 欠点・権限・秘密 |
| --- | --- | --- | --- |
| A | login の時に session（sessiond の子か compositor）が利用者として `ENABLE` を送り、所有者を login した利用者にする。logout で root の方針へ戻す（または disable） | 今の「方針の所有者は一人」の設計のまま。秘密は利用者の 0600 の store から出ない。利用者ごとの network | login するまで WiFi は root の store だけ（login 前の更新・時刻合わせは有線か root の profile）。複数の login・急速な user 切替の順の扱いが要る。logout の戻しを足す |
| B | Settings の join が system の store（`/etc/wifi.conf`）に書く（`network` group の利用者に書く権限を networkd 経由で与える） | 起動の root の方針がそのまま使い、login 前から自動で繋がる。B2 が一番単純 | 一人の利用者の鍵が機械の全員の鍵になる。`/etc/wifi.conf` を利用者が書く経路（networkd に鍵を渡す新しい request、または setuid の helper）は秘密の境界を変える（今の設計は「networkd に鍵を送らない」、`docs/reference/managed-wlan.md`） |
| C | networkd が所有者の store と root の store の両方を読む（root の profile を全員の既定、利用者の profile を上に重ねる） | login 前は root、login 後は両方 | store の優先・同じ SSID の衝突・所有者の意味が複雑。結局 login の時に誰の store を足すかを決める仕組み（A の一部）が要る |
| A' | A の最小形: 所有者の不一致のとき Settings・system bar の「on」と join が `ENABLE` を先に送り所有者を取る（root の方針は利用者の操作で上書きできる、今の `ENABLE` の許可と同じ）。login 時の自動の ENABLE は A のとおり | 変更が小さい（libkeiland と UI の順、networkd は不変か小さい） | 所有者を黙って奪うので、別の利用者の接続を切る。単一利用者の laptop（ベータ1 の想定）では問題が小さい |

推奨（P1）: ベータ1 は **A（login 時に利用者として ENABLE、logout で root に戻す）＋ A' の UI の順（join の前に必要なら ENABLE）**。秘密の境界（networkd に鍵を送らない、
利用者の 0600 の store）を保ち、単一利用者の laptop で B1・B2 を満たす。B は秘密の境界を変えるのでベータ1 では選ばない。C は A の後で要れば。

B3 の推奨: 有線を優先する。WiFi の接続が有線の default route があるときは default を足さない（または有線を先の slot に保つ）、resolver は
「最後に default を持つ interface の lease」を networkd が書き、片方を外したらもう片方の lease で書き直す。dhcpc に「route/resolver を書かない」mode を足し、
networkd が一つの所で決める形が筋（WS033 と共有）。

## 4. p019 の実装の範囲（案、A＋A' を選んだ場合）

| 変更 | file | 試験 |
| --- | --- | --- |
| login の時に利用者として `ENABLE`、logout で root の `ENABLE`（または所有者の放棄） | `userland/desktop/sessiond/session.c`（子を利用者の uid で起こす所）、または libkeiland に `keiland_network_take_ownership` | QEMU（本物の networkd、偽の `/sbin/wifi` の子）で: 起動 → kei の login → 所有者が kei、`~/.wifi.conf` の auto profile へ自動 connect が走る |
| join の前に所有者でなければ `ENABLE` を送る順（Settings・system bar 共通） | `userland/desktop/libkeiland/zedbsd/network-zedbsd.c`、`settings/network.c`、`wayland/network.c` | 同上: root 所有の auto-searching から Settings で鍵を入れて join が通る（今は EPERM になるはずの回帰の試験） |
| 失敗の理由を一つの表に（stage → 文言）、Settings と system bar で共有 | libkeiland（`keiland_network_get_request` の errno に stage を足すか、networkd の SHOW の terminal 行を読む）、両 UI | 鍵の誤り・SSID 不在・off・切断の 4 つの文言を偽の子で出す |
| owner を SHOW に出す（診断） | `userland/base/networkd/main.c` の状態の表示 | host か QEMU |
| B3 の route/resolver の方針 | `dhcpc/main.c`、`networkd/main.c`・`managed-lan.c`（WS033 と調整） | QEMU の usb-net＋偽の WiFi で default と resolv.conf の持ち主、片方を外す |

試験の器: 本物の networkd と本物の `net`/libkeiland、`/sbin/wifi` だけを偽の子（scan の一覧と connect の成否を返す）にした QEMU の image。鍵は試験用の
固定の偽の値（実在の AP の鍵ではない）なので記録してよい。本物の radio（RTL8822BU の passthrough）は p020。

## 5. 確かめたこと・未検証

- 疑い（1.2 の EPERM）は **未検証**（source の読みだけ）。2026-10-02 の Q1 の一時停止（main の build/ の整理のため新しい build・QEMU を止める）で、
  本物の networkd と偽の `wifi` の子の QEMU の試験はこの Queue では行わなかった。手順は 4 の最初の 2 行の試験。CLI での簡易の確かめ（kei で
  `net wifi set-key S K auto` → `net wifi connect S` が EPERM、`net wifi enable` の後は通る）でも足りる。
- host の試験は走らせていない（`owner_allowed` は networkd の `main.c` の static で、host で単独に build する器が無い）。
- B3 の route の振る舞い（2 つの default の並び、device の消滅で route が消えるか）は kernel の source の読みだけ。

## 6. ユーザーの判断が要る点

1. 利用者の鍵と再起動の後の自動の再接続の方式（3 の A／A'／B／C）。P1 の推奨は A＋A'。
2. system bar に鍵の入力を足すか（足さないなら「鍵の要る network は Settings で」と案内する）。
3. 有線と WiFi が同時のときの優先（P1 の推奨は有線優先）。
4. 複数の利用者がいる機械で、利用者の WiFi の操作が他の利用者の接続を奪ってよいか（A' の前提）。
