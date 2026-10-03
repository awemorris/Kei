<!-- awesome-plan project=zedbsd record=ws131-p011 -->

# ws131-p011: Settings を拡張の経路へ、毎秒の監視の除去、libkeiland の OS を 0 に

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p010 cleared。D8 の単独走行で番号の順（p010 の次）。判断 D6（承認済み）。**この Phase の cleared の後に、Q1 がユーザーに進み具合を報告する区切り**（D8）
目安: 4〜5h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/settings/`（`look.c`・`network.c`・`sound.c`・`page-home.c`・`page-input.c`・`page-network.c`）、`userland/desktop/wayland/preferences.c`（監視の除去）、`userland/desktop/libkeiland/`（`system-compat.c`・`preferences.c` の除去、`keiland.h`、exports.map、Makefile 3 本）、`plan/ws131/`。（Q1 の委任が要る: Q1 が Phase の前に委任を記録する）: `plan/ws089/tests/settings-p007.sh`・`host-network.c`・`host-slot.c`・`host-preferences.*`、`plan/ws100/tests/host-audio.*`、`plan/tools/keiland-linux/network-probe.c`・`audio-probe.c`・`lib-smoke.c`

## 目的と結果

Settings の設定・WiFi・音量を `kl_system_*` に替え、**同じ Phase で** compositor の毎秒の `stat` の監視（`wayland/preferences.c:237-360`、BUG-125 の原因）を除く（review 1）。libkeiland から旧 `keiland_network_*`・`keiland_audio_*`・`keiland_preferences_*` と backend の同居を除き、compositor が libkeiland から使う物を描画の層と motion だけにする（D4、link は残してよい）。

## 範囲

1. Settings: `look.c` → `kl_system_settings_*`、`network.c`・`page-network.c` → `kl_system_network_*`（詳細は `query_details` の非同期）、`sound.c`・`page-home.c`・`page-input.c` → `kl_system_audio_*`（`service`）。Keiland 以外の compositor では該当の頁を「この desktop では使えない」に（design.md §4.1 の 6）。
2. compositor: 監視の thread と `PREFERENCES_CHECK_MS` を除く。`settings-p007.sh` を「拡張で変えて数秒で適用」に書き換える（委任）。
3. libkeiland: 旧 API・`system-compat.c`・`preferences.c` を除く。backend の同居を外す。B3 を強める。
4. 道具の向け直し（review 6、委任）: `network-probe.c`・`audio-probe.c` は backend の API を直接使う形に、`lib-smoke.c` は `kl_system_*` の有無に、`host-network.c`・`host-slot.c`・`host-preferences.*`・`host-audio.*` は backend と compositor の store の試験に。
5. 書き手の Guardrail の文を Q1 に渡す（design.md §3.7）。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- app に `keiland_network_`・`keiland_audio_`・`keiland_preferences_` の呼び出しが 0。compositor が使う libkeiland の symbol が D4 の許可の表の中だけ（B2、3 OS の `nm -u`）。`preferences.c` に周期の監視が無い。
- zedBSD: `settings-regress.sh`、書き換えた `settings-p007.sh`、`volume-p004.sh`・`volume-p005.sh`、C9 の `zdesktop-p076.sh` を単独で 20 回（結果は Q1 が BUG-125 に追記）、boot-test。Linux: Settings の起動と WiFi の状態の表示（hwsim）、向け直した `network-probe`・`audio-probe`・`lib-smoke`。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: WS089（`settings/page-*.c`）・P1（`settings/network.c`）と取り合う。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
