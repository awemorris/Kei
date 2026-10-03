<!-- awesome-plan project=zedbsd record=ws131-p002 -->
# ws131-p002: libkeiland-backend の分離と libkeiui の吸収の詳細な移行計画（設計のみ）

Status: in-progress（q628-i01 で初版、q629-i01 で第 2 版を提出。ユーザーのレビュー待ち。clearance は Q1 が判定）
Disposition: normal
Parent: [WS131](../ws.md)

## 範囲（2026-10-03 user「コードを書く前に、しっかりと以降の計画を立ててから、実行しましょう。移行計画はレビューさせてください。」）

code は書かない。成果は `plan/ws131/design.md`（移行計画の正本）と、p003〜p016 の各 `phaseNNN/phase.md`（範囲・受け入れ・検証・所有 path・依存・危険）。

1. 今の source の正確な棚卸し: libkeiland・libkeiui の全 file と全公開 symbol（`exports.map`・header）、利用者（app・compositor・xserver・probe・試験・Linux/FreeBSD の build）を表に。compositor の OS の module（`wayland/linux/`・`gpu-zedbsd.c`・FreeBSD の module・evdev の macro）の全 file。
2. libkeiland-backend の interface を領域ごと（電源・WiFi・network・音声・PnP・seat・入力・KMS/GPU）に設計し、3 OS の tree の配置、build（zedBSD の vmunix.mk 側・`Makefile.linux`・`Makefile.freebsd`）、install、Guardrail の「compositor は libvulkan だけ」と「OS の境界」の改訂案、境界の checker の改訂案。
3. compositor の拡張の protocol（設定の変更と記録、WiFi・network、音量、電源、PnP）の message の案と、libkeiland の client の wrapper の API。compositor の毎秒の stat の除去。WS113 の Settings → compositor の仕組みとの共有。
4. libkeiui の吸収: `kui_` → `keiland_` の全 symbol の対応表、名前の衝突の確認、移行の間の互換の header の方式、KEILAND_VERSION と ABI、段階ごとに既存の app が build・動作し続ける手順。
5. app の骨組みの API（`keiland_app`・`keiland_window`、宣言的な menu・titlebar・glass）の案（p001 の study の案 B を新しい構成で見直す）。
6. 各 Phase の作業の順、並行してよい所・衝突する所（ベータ1 の標準 app の作業、P1・P2 の作業中の file）、各 Phase の回帰の範囲（zedBSD・Linux・FreeBSD）、rollback の方法。
7. ユーザーの判断が要る点の一覧。

design.md ができたら Q1 が design-reviewer で敵対的に review し、ユーザーのレビューに出す。ユーザーの承認まで p003 以降の code は書かない。

## 結果（2026-10-03、q628-i01、P3 generation3）

読み取りの調査と設計だけ（source・build・QEMU は無し）。対象は worktree `agent/p3` = main `c5c59904e`。

成果:

- [design.md](../design.md): 移行計画の正本。§0 要約（構成・Phase の一覧・判断の要約）、§2 棚卸し（libkeiland 22 file・13,018 行・公開関数 111、libkeiui 29 file・16,236 行・名前 341、compositor の OS の module 6,591 行と compositor の内部への結合、利用者の link、試験の参照 73 file）、§3 backend（置き場・静的な内部 library・interface の案・結合の切り離し・GPU の buffer の G2・build と install・Guardrail と checker の改訂案）、§4 拡張の protocol（共通の約束・message の表・設定の記録と stat の除去・`keiland_system_*`・WS113/WS132 との共有）、§5 吸収（置き場・名前の例外・互換 header・exports.map・版の段階・app が動き続ける手順）、§6 骨組みの API、§7 順序・衝突・回帰・rollback、§8 他の WS への影響、§9 判断 D1〜D13、§10 未確認。
- [rename-map.md](../rename-map.md): `kui_`→`keiland_` の全 341 の名前の対応（[tools/rename-map.py](../tools/rename-map.py) が `keiui.h` から生成、衝突の例外 4 と値の同じ定数の一本化 15 を注に）。
- p003〜p016 の各 phase.md（目的と結果・範囲・受け入れ・検証・衝突・危険・rollback・Resume）。

範囲 1〜7 との対応: 1 → design.md §2、2 → §3、3 → §4、4 → §5 と rename-map.md、5 → §6、6 → §7 と各 phase.md、7 → §9。

主な発見:

- compositor の OS の module は `struct zwl_server` の field と `zwl_compose_*`・`zwl_input_*` を直接触る（design.md §2.5 の右の列）。backend へ移すには callback（`struct keiland_backend_host`）への切り離しが中心の仕事で、初案の p004 を p004（seat・入力・session）と p005（表示・GPU）に割った。
- zedBSD の Vulkan の header に `VK_EXT_image_drm_format_modifier` が無く、dma-buf の protocol の処理は zedBSD で compile できない。GPU の buffer の protocol の処理は compositor の「buffer の仕組みの module」に残す案（G2、D2）。
- `kui_`→`keiland_` の衝突は `kui_version`・`KUI_VERSION`・`kui_edit_fn`・`kui_keyboard_inset_fn` の 4 つ。`KUI_EDIT_*`・`KUI_KEYBOARD_INSET_*` の 15 は同名の `KEILAND_*` と値が同じ。`kui_file_chooser*` は KEILAND_VERSION 12〜15 の旧名を再び使う。
- libkeiui が compile する `picture/color-glyph.c` の `keiland_color_glyph`・`keiland_color_image` は、吸収の後に `keiland_*` の一つの glob で出すと外に漏れる。exports.map は領域ごとの明示の列挙にする。
- compositor 自身も `desktop.conf` を書いている（`wayland/volume.c:703-706`）。BUG-125 の原因の毎秒の `stat` の thread は `wayland/preferences.c:237-360`。

未実施・限界: build・試験・QEMU は流していない（設計のみ）。zedBSD の client の socket の uid の取得手段、sessiond の session の中の `POWER`、FreeBSD の passthrough なしの起動は未確認（design.md §10）。

範囲外で Q1 に依頼すること: Guardrail の改訂（design.md §3.7、適用は p005）、Future Work への browser の shell の窓の移行の登録（D7）、WS090・WS113・WS132・WS099 の表への反映（design.md §8）、D10 の移動の例外の正本（`plan/standards/`）。

## 改訂（2026-10-03、q629-i01、P3 generation3）

入力: 2026-10-03 のユーザーの決定（D2: GPU の buffer も backend へ、backend 化は 1 つずつ、名前は `KL_`・`kl_`・`KWL_`・`kwl_`、ABI は変えてよく版も上げない、D14 の Guardrail の範囲）と、design-reviewer の review（Q1 の要約 `plan/ws131/reviews/2026-10-03-design-review.md`、22 項目と事実の誤り）。設計だけで source・build・QEMU は無し、対象は main `dad53b125`。

成果:

- [design.md](../design.md) 第 2 版: GPU の buffer の protocol を backend へ移し、compositor が渡す protocol の host の interface（11 操作、今の 2 つの module が compositor から使う関数と field の実測から）で逆向きの依存を解く（§3.5）。callback の約束（再入しない・PauseDeviceComplete・ResumeDevice の新しい fd・答えの多重化、§3.4）。拡張は `kl_system_*_v1`、`result(applied, saved)`、一度に一つの network の要求、registry での uid の照合、worker の thread（§4）。名前の段と互換の方式（§5）。review への対応表（§11）。
- [rename-map.md](../rename-map.md)（公開の名前 607 → `kl_`・`KL_`、解決の要る衝突 0）と [rename-map-kwl.md](../rename-map-kwl.md)（`zwl_` 494・`ZWL_` 271 → `kwl_`・`KWL_`）を [tools/rename-map.py](../tools/rename-map.py) で生成。
- Phase を p003〜p024 の 22 個に切り直した（backend 7 領域・拡張 2・吸収と改名 3・骨組み 1・app 5・compositor の改名 2・終わり 2、85〜120h）。旧 p003〜p016 の phase.md は新しい内容で置き換え、p017〜p024 を新規に作った。

主な変更: 毎秒の監視の除去を Settings の移行と同じ p011 へ（review 1）、Files・config の 5 か所・FreeBSD の header の表の取りこぼしを p012 へ（review 2）、zedBSD の session の中の電源は unsupported（sessiond は session の socket で UNLOCK と LOGOUT だけを受ける、review 4）、Linux の電源は D-Bus が移る p006 で有効に、log の接頭辞の変更は compositor の改名と別の p022 で試験 201 本と同時。

判断: 決定済みは D2・D14・ABI・名前の規則。残りは D1・D3〜D13・D15（protocol の名前も `kl_*_v1` に）・D16（共有の source の内部の名前と include guard）。

## ユーザーのレビューの反映（2026-10-03、q630、P3 generation3）

2026-10-03 user（p002 第 2 版のレビュー、記録は [ws.md](../ws.md) の末尾）: D1・D3・D5・D6・D10〜D13・D15・D16 は推奨のとおり承認。D4・D8・D9 は次の決定で、design.md の §0.3・§2・§3.6〜§3.8・§5.3・§7.1・§7.2・§8・§9 と p003・p010〜p014・p016〜p020 の phase.md に反映した。D7 は Q1 がユーザーに説明して確認中。

- D4: compositor は touch IME の UI のために libkeiland を link してよい（不要なら link しない）。循環を作らない条件を design.md §9 に書いた: compositor は libkeiland の Wayland の client の部分（`kl_system_*`・`kl_app_*`・`kl_window_*`・file chooser・protocol の wrapper）を使わない、libkeiland は compositor・backend を使わない、拡張の protocol の定数は共有の header、`desktop.conf` の store は compositor の中だけ。checker B2 を「compositor が使う libkeiland の symbol が許可の表の中だけ」に変えた。motion を compositor の source として compile する案と `keiland-motion.h` の分離は取り下げた。
- D8: 単独走行（N=1）で p003〜p024 を番号の順に流す（依存は番号の順で全て満たされる）。p011 の後に Q1 がユーザーに進み具合を報告する区切り。開始はユーザーの承認と P2 の終了の後に Q1 が指示する。
- D9: WS090 の残りは WS131 の完了の後に扱い、WS131 の間は WS090 を動かさない。Settings と Files の窓は p019・p020 のまま。
- p003 の前提「P1 の `network-zedbsd.c` の merge」: 満たされた。P1 の q627 は main `48237bb2a` に統合済みで、q627 は `network-zedbsd.c` に触れていない（最後の変更は `6efb4f2bb`、2026-10-01）。P1 の worktree に libkeiland・`settings/network.c`・`wayland/network.c` の未 commit の変更は無い。p003 の開始の時に Q1 が再確認する。
