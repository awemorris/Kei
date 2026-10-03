<!-- awesome-plan project=zedbsd record=ws089-p010 -->

# ws089-p010: 現在の main での回帰・S7（QEMU）・棚卸しと候補

Status: cleared（q617-i01、P1 generation4、2026-10-03。QEMU の Venus と host。5330 の S7 は p011（未実施）。結果は末尾）
Disposition: normal
Parent: [WS089](../ws.md)
Queue: q617 / q617-i01（Q1 の dispatch、2026-10-03。承認: user「Settingsも重点的にしましょう」と自走の指示。時限 3 時間）
依存: なし
目安: 3h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `plan/ws089/`（試験の期待が古いときの試験の直しと `beta1-candidates.md`）。product の source は変えない

## 範囲

1. [guide.md](../guide.md) §3.1 のとおり: §5.1 の host、§5.2 の `settings-regress.sh`（8 本）、§5.3 の `volume-p005.sh`、boot test。FAIL が 1 回なら流し直し、2 回続けば原因を調べ、試験の期待の古さなら試験を直す（WS089 の file）。
2. 全 23 頁の通し（QEMU の Venus、1280x800 と 1920x1080）: 頁ごとの画面（PNG）と、崩れ・読めない文字・働かない操作の不具合の表（重さ: 重い／中／軽い）。
3. ブラッシュアップの候補の一覧 `plan/ws089/beta1-candidates.md`: ws.md の「後回しの候補」の表と 2 の気づきを、価値・規模（h）・危険・依存・触る file で並べ、計画エージェントの推奨（ベータ1 に入れる／入れない）を付ける。準備中の頁のうち 5330 で意味のある物（Battery の残量など）の backend の有無も調べる。

## 受け入れ

- 1 が全て PASS（S-B1 の基準）。画面（`p004`・`p008`・`p009` の PNG）を Q1 経由でユーザーに見せる。
- 2 の不具合の表と 3 の候補の一覧がある。重い・中の不具合は Bug Board の候補として Q1 に渡す。
- ユーザーの選択（Q1 が聞く）は p013〜p017 の Status に反映する（計画の更新）。回答待ちは cleared の妨げにしない。

## 検証の方法と範囲

QEMU の Venus。console・serial の log では判定しない。 やっていない確認は「未実施」と書く。

## 未決の判断

候補の採否（ユーザー）。

## Event

2026-10-02 / ws089-beta1-plan: fg019 の計画で新設。

## q617-i01 の結果（P1 generation4、2026-10-03 03:10〜04:10、base main `416145ddd`）

source は変えていない。変えたのは `plan/ws089/` の試験と記録だけ。

### 1. 回帰（S-B1 の基準）

| 確かめ | command | 結果 |
| --- | --- | --- |
| host | `sh plan/ws089/tests/host-build.sh`、`sh plan/ws089/tests/host-preferences.sh` | `built build/ws089-host/settings-render`（exit 0）、`host-preferences: PASS` |
| image | `sh plan/ws089/tests/build-settings-image.sh build/p1-settings` | exit 0、log の warning 0 |
| guest（IME 無し、試験の image の既定） | `GUEST_RUNTIME=$PWD/build/p1-p010-run sh plan/ws089/tests/settings-regress.sh build/ws089-p010/settings-regress3` | **`settings-regress: PASS`**（8 本、新しく起動した guest） |
| guest（IME 入り、BUG-146 の確かめ） | `SETTINGS_CONFIG=plan/ws089/tests/config-amd64-settings-ime.mk sh plan/ws089/tests/build-settings-image.sh build/p1-settings-ime` → `settings-regress.sh build/ws089-p010/settings-regress-ime` | **`settings-regress: PASS`**（8 本）。zdesktop の log で `ZWL IME started ... client=1`、Settings は `ZWL MAP client=2` |
| 音の頁（WS100） | `sh plan/ws100/tests/host-audio.sh`、volume の image（`build/p1-volume`）で `sh plan/ws100/tests/volume-p005.sh build/p1-volume/hdd-image.img build/ws089-p010/volume-p005` | `host-audio: 14/14 passed`、**`volume-p005: PASS`** |
| boot test | `OUTPUT=build/ws089-p010/boot-test bash plan/tools/boot-test.sh build/p1-settings/hdd-image.img` | PASS（`build/ws089-p010/boot-test/login.png`、GPU の無い QEMU なので console の login） |

画面（worktree の `build/ws089-p010/`）: S7 の 3 つ（`settings-regress/p004/wallpaper-aurora.png`・`wallpaper-default.png`・`appearance-85.png`、
`p008/search-wi.png`・`search-dns-open.png`、`p009/*.png`）、Wi-Fi の鍵の入力（`p003/wifi-key.png`）、Sound（`volume-p005/*.png`）。
S7 の 5330（passthrough）の分は**未実施**（p011。5330 の host は朝まで使えない）。

経緯（流し直しの記録）:
- 1 回目（`build/ws089-p010/settings-regress`）は p006 ほかが FAIL。原因は前の世代の P1 の試験（`build/p1-s/`）が**同じ runtime（`build/p1-settings-run`）の
  同じ guest で同時に走っていた**こと（zdesktop の log で、片方が起動した Settings の窓をもう片方の試験が撮っていた）。両方を止め、runtime を
  `build/p1-p010-run` に替えて流し直した → p002 以外 PASS。p002 は `zdesktop: ERROR lines` で FAIL、単独の流し直しで PASS（ERROR の行は
  次の試験が log を上書きして不明。候補の表の D6）。
- 試験を直した後の 2 回目（`settings-regress2`）は p002・p004・p007 が FAIL。同じ時刻に IME 入りの image の build と 1920x1080 の 2 つ目の guest を
  同じ host で動かしていた（Settings の窓が 5 秒で map されない `window at 0,0`、key の後の頁の行が出ない）。直後の p002 の単独は PASS、
  新しく起動した guest で他を止めて流した 3 回目（`settings-regress3`）は 8 本 PASS。負荷の下での待ちの不足は F-062（固定の待ち）と同じ種類。

### 2. 試験の直し（`plan/ws089/tests/`）

- **BUG-146**: 試験の image（`config-amd64-settings.mk`）は Files の image（`plan/tools/files/config-amd64-files.mk`）を include するので、
  既に IME を外していた（zdesktop の log `ZWL IME none`）。それに加え、試験が client の番号に頼らないように直した: `settings-wait.sh` に
  `find_window`（zdesktop の log で `ZWL CLIENT client=N ... ime=1` の client を除いた最後の `ZWL MAP` から窓と client を決める）を足し、
  `settings-p002`〜`p006`・`p008`・`p009` の `grep 'ZWL MAP client=1 '` を置き換えた。p002 の titlebar の control は `find_window` の client で探し、
  App Home から起動した窓の確かめは `client=[0-9]+`。IME 入りの image でも 8 本 PASS（上の表）。
- 確かめ用の config `config-amd64-settings-ime.mk`（IME 入り）と、`build-settings-image.sh` の `SETTINGS_CONFIG`（config の差し替え）。
- 各試験の `zdesktop: ERROR lines` で、その ERROR の行（最初の 5 行）を log に出す（D6 の原因を次に残すため）。
- 新しい `settings-pages.sh WIDTHxHEIGHT [OUTDIR]`: 指定の大きさ（guest は `VENUS_SIZE`）で Home と 23 頁を順に撮る（p006 の手順 2 と同じ。
  1920x1080 の通しに使った）。

### 3. 23 頁の通しと不具合の表

- 1280x800: `settings-p006.sh`（regress の中）の `settings-regress3/p006/00-home.png`〜`23-about.png`。1920x1080: `VENUS_SIZE=1920x1080` の guest で
  `settings-pages.sh 1920x1080 build/ws089-p010/pages-1920x1080` → `settings-pages: PASS`（24 頁の行、ERROR 0）。host の描画（窓の中だけ、偽の network）:
  `build/ws089-host/settings-render --size=WxH --page=WORD` を 24 頁 × 2 の大きさ（`build/ws089-p010/host-pages/`）。
- 崩れ・読めない文字・働かない操作は見つからなかった。不具合は全て軽い（重い・中は 0）。表は [beta1-candidates.md](../beta1-candidates.md) §1
  （D1 Wi-Fi の scan の待ちの間の操作が捨てられる（実 radio では中になりうる）、D2 network group の外の利用者に「service not running」、
  D3 左の pane が既定の窓に入りきらず scroll の手がかりが無い、D4 出力の無い Sound の slider と語、D5 Network Activity の期間の説明、D6 試験の 1 回の ERROR）。
- Bug Board の候補（Q1 へ）: 重い・中が無いので必須は無い。D1 は 5330 で再現したら ticket にする候補。

### 4. ベータ1 の候補の一覧

[beta1-candidates.md](../beta1-candidates.md) §2（C1〜C17、項目・価値・目安・危険・依存・触る file・推奨）。推奨は C1（Wi-Fi の操作の待ちの slot、1h）・
C5（= p012、2h）・C4（文言と無効の見た目、0.5h）を入れる。C2・C3・C6（= p013）・C8・C9 はユーザーの好み次第、C7（= p016）以下は外す。
日本語の UI（p015）と accent・dark（p017）はベータ1 に入れない決定済み（F-068）。§3 は準備中の頁の backend の調べ（Battery の backend は無い:
ACPI の EC・AML の評価と診断用の `/dev/acpi` はあるが、利用者向けの電池の interface・daemon が無く、system bar の電池も絵だけ）。
WiFi の頁と ws005-p019・p024 の整合: 鍵は利用者の store（`keiland_network_save_key`）→ PROFILES → JOIN の順で、p019 の explicit な join（要求者の
store で profile を探す）と合う。p024 の login・logout の通知は sessiond の物で、Settings の変更は要らない。食い違いは D2（group の外の案内）だけ。

### 未実施・残り

- 5330 の S7（p011）と実 radio での Wi-Fi の頁（p014）は未実施。
- 候補の採否（ユーザー、Q1 が聞く）と p013〜p017 の Status への反映（計画の更新、Q1）。
- 試験の待ちの不足（負荷の下で Settings の map・key の反映を固定の `sleep` で待つ、F-062）は残る。負荷の無い guest で流すこと。
