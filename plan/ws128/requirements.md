<!-- awesome-plan project=zedbsd record=ws128-requirements -->

# WS128 標準アプリの棚卸しとベータ1 の改善の候補（ws128-p001、2026-10-03）

[ws128-p001](phase001/phase.md) の結果。採否はユーザーが選ぶ（Q1 が聞く）。この一覧は計画であり、実装の許可ではない。
Text Editor の Replace・Open Recent（p003）と Notes の Open・Save As（p002）は q621 で済み（cleared）。

## 1. 基準の回帰（A1、QEMU の Venus と host）

source は変えていない（直したのは試験 `plan/ws079/tests/demo-s8-s9.sh` の待ちだけ、下の注）。main は `6df847d54`（userland の source は
`52c1d37ef` との差が q621 の notes・textedit だけ）。image は worktree の `build/` に作った:

| image | config | sha256（先頭 16） | 作り方 |
| --- | --- | --- | --- |
| `build/p1-main` | `plan/ws089/tests/config-amd64-settings.mk`（Files の image + titlebar の menu + Settings。textedit・imageview・notes・pdfviewer・terminal 入り） | `2b7cac0edb54d391` | 新しい directory で `build-settings-image.sh build/p1-main`（main `6df847d54`） |
| `build/p1-main-volume` | `plan/ws100/tests/config-amd64-volume.mk` | `2ffdbd7f7d1e42b9` | 新しい directory で make と audiod-feedback |
| `build/p1-demo-notes` | `plan/ws079/tests/config-amd64-demo.mk`（pen・touch の注入） | `9e04f312c31b2cd7` | `52c1d37ef` + q621 の notes・textedit（= main の userland） |
| `build/p1-settings-touch` | `plan/ws089/tests/config-amd64-settings-touch.mk`（touch の注入） | `e377646d9a340882` | スクリーンキーボードの試験の kernel と touchinject。compositor・libs・textedit は試験の install で `build/p1-main` から入れた |

| アプリ | command | 結果 |
| --- | --- | --- |
| Text Editor | `sh plan/tools/textedit/host-core.sh build/p1-te/host-core` | `host-core: 53/53` |
| Image Viewer（host） | `sh plan/tools/imageview/run-host.sh` | `host-imageview: PASS` |
| Image Viewer（guest） | `BIN=build/p1-main plan/tools/imageview/imageview-guest.sh OUT install home empty fit next zoom wheel portrait rotate alpha gif pixels broken swipe fullscreen chooser`（`build/p1-main`） | 1 回目: install の `put libwayland-client.so` が timeout（同じ時刻に volume の image を build していた）→ install の後の複写が止まり `ZWL HOME opened MISSING`、他の 15 手順は全て ok（image 自身の imageview で）。負荷を止めて `install home` を流し直し → `installed`・`ZWL HOME opened ok` |
| PDF Viewer（host） | `sh plan/ws079/tests/run-pdf-render.sh` → `sh plan/ws079/tests/run-pdfviewer-host.sh` | `run-pdf-render: ok`、`run-pdfviewer-host: ok` |
| Notes（host） | `sh plan/ws079/tests/run-notes-host.sh` | `run-notes-host: ok` |
| Notes・PDF Viewer（guest、S8・S9） | `plan/ws079/tests/demo-s8-s9.sh build/p1-demo-notes/hdd-image.img OUT`（待ちを直した版） | 1 回目 FAIL（`scroll_turn_ms=203`、限度 200。同じ時刻に host で libpdf の fuzz 試験を流していた）→ 負荷無しで **PASS**（`scroll_turn_ms=127 page_frame_ms=139 page_turn_ms=440 limit=200`） |
| Terminal | `plan/tools/titlebar/menu-p003.sh OUT`（Terminal の menu: Edit・Select All・Copy・F10 の key・Session・Paste・View・Text Size・Help・docked・2 つ目の窓）（`build/p1-main`） | **PASS** |
| 音量 | `plan/ws100/tests/volume-p004.sh build/p1-main-volume/hdd-image.img OUT`、`volume-p005.sh` 同 | **PASS**、**PASS** |
| スクリーンキーボード | `BIN=build/p1-main plan/ws102/tests/osk-guest.sh OUT install pointer edges flick close qwerty extra tools history touch`（`build/p1-settings-touch`） | history の最後の file の読み出しだけ SSH の timeout（`Connection timed out during banner exchange`）、他の手順は全て ok（history の `items=3`・`paste index=1` も ok）。`install history` だけの流し直しは、冷えた guest で Text Editor の起動が固定の 5 秒に間に合わず `items=3 MISSING`（F-062 の種類）。`install tools history` で **PASS** |

未実施: スクリーンキーボードの `send`・`hand`（ime-probe が image に無い）、`roll`（pen の image）、`large`（1920x1080）。Image Viewer の touch（`touch-guest.sh`）。
WS102 の C9 など他の WS の回帰。実機（5330、p007）。

**試験の直し（F-062）**: `plan/ws079/tests/demo-s8-s9.sh` は guest を起こした後に固定で `sleep 20` を待っていて、この host では起動が間に合わず
全ての手順が MISSING になった（q621 で観測）。SSH の応答を最大 3 分待つ（`guest.py wait --timeout 180`）形に替えた（commit `0a5e8ae42`）。
試験の内容は不変。直した版で 2 回流し、1 回目は上の負荷による 203 ms、2 回目 PASS。

## 2. menu の全項目と働き（2026-10-03 の main）

印: ✓ = 既存の回帰が働くことを確かめた、△ = 回帰が手順に無い（source と画面の確認）、— = 無い。

| アプリ | menu の項目 | 確かめ |
| --- | --- | --- |
| Text Editor | File（New・Open…・Open Recent・Save・Save As…・Close・Quit）、Edit（Undo・Redo・Cut・Copy・Paste・Select All・Find…・Find Next・Find Previous・Replace…）、View（Line Numbers・Word Wrap・Bigger・Smaller・Actual Size）、Help（About） | ✓ host-core 53・textedit-p003（Replace・Open Recent・Save）・osk の send/extra/tools（入力・Select All・copy・paste）。View・About は △ |
| Image Viewer | File（Open…・Close・Quit）、View（Fit・Actual Size・Zoom In/Out・Rotate Right/Left・Play Animation・Full Screen）、Go（Previous・Next・First・Last）、Help（About）、context menu | ✓ imageview-guest（fit・next・zoom・wheel・rotate・gif・fullscreen・chooser・swipe・壊れた file の説明）。First・Last・Rotate Left・About は △ |
| PDF Viewer | File（Open…・Annotate in Notes・Close・Quit）、View（Page Thumbnails・Continuous Scroll・Single Page・Fit Width・Fit Page・Zoom In/Out・Reset Zoom）、Go（Previous・Next・First・Last） | ✓ S9（scroll・page・double tap・pinch）、host-pdfviewer、notes-p014 の Annotate。Thumbnails・First・Last は △。**Help（About）が無い** |
| Notes | File（New Page・Open…・Save・Save As…・Close）、Edit（Undo・Redo）、Page（Previous・Next）、Tool（Pen・Marker・Eraser）、View（Fullscreen） | ✓ S8・notes-p002（Open・Save As）・run-notes-host。**Help（About）が無い、頁を消す項目が無い** |
| Terminal | Shell（New Window・New Tab・Close Tab・Close Window）、Edit（Copy・Paste・Select All）、View（Zoom In/Out・Normal Size・Text Size・Fullscreen）、Session（Send Interrupt・Send End of File・Clear Screen・Reset Terminal）、Help（About） | ✓ menu-p003（Edit・Session・Text Size・About・docked・2 つ目の窓）。New Tab・Close Tab は △（S10 は WS086・087 で済み）。**検索が無い** |
| 音量（system bar） | icon の click の popup（slider・mute）、wheel | ✓ volume-p004（A1〜A5）・volume-p005（Settings と双方向） |
| スクリーンキーボード | flick・QWERTY・手書きの面、右の列の道具（前の app・Del・編集・履歴）、角の gesture | ✓ osk-guest（pointer・edges・flick・close・qwerty・extra・tools・history・touch）。絵文字の面は未実装（WS102 p022） |

### 不具合の表

**重い・中は 0**。軽い物も新しくは見つからなかった（画面: 下の §6）。試験の基盤で見つかった物（product の不具合ではない）:

| # | 内容 | 重さ | 扱い |
| --- | --- | --- | --- |
| T1 | `demo-s8-s9.sh` の固定の `sleep 20` | 試験 | 直した（上） |
| T2 | `osk-guest.sh` の history は Text Editor の起動を固定の 5 秒で待つので、冷えた guest では copy が取りこぼされる | 試験 | F-062 の対象（WS102 の file、所有の外）。Q1 へ |
| T3 | Settings の試験の image に ime-probe が無く、osk の send・hand を流せない | 試験 | 必要なら config に ime-probe を足す（WS089/WS102 の判断） |

## 3. 未完成の UI（A3）

`grep -rn -i "not available\|not yet\|unsupported\|later version\|not supported"`（textedit・imageview・notes・pdfviewer・terminal、comment を除く）:

| file | 文言 | 扱い |
| --- | --- | --- |
| `imageview/image.c:342` | "Interlaced PNG images are not supported" | 機能の制限（interlace の PNG を開けない）。候補 I5 |
| `imageview/image.c:436` | "The JPEG image is damaged or of a kind not supported" | progressive・CMYK などの制限（説明として妥当） |
| `notes/main.c:630` | "The PDF uses features Notes cannot read yet. Started a new note" | libpdf の未対応の機能（説明として妥当） |

「Opening from Notes is not available yet」は p002 で無くなった。**UI に出ている未完成の機能は 0**（上は形式の制限の説明）。

## 4. 改善の候補

推奨: **入** = ベータ1 に入れる、**任** = 時間とユーザーの好み次第、**外** = 入れない。目安はエージェントの時間（試験を含む）。

| ID | アプリ | 項目 | 価値 | 目安 | 危険 | 依存・判断 | 触る file | 推奨 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| C1 | 全体 | Help > About の揃え（PDF Viewer・Notes に Help menu と About の dialog） | menu の一貫性（デモで目に入る） | 0.5h | 低 | なし | `pdfviewer/menu.c`・`main.c`、`notes/menu.c`・`main.c` | **入** |
| C2 | Notes | Page > Delete Page（undo で戻る） | 頁を足せるが消せない欠け | 1.5h | 低〜中（model に `notes_document_remove_page`・`insert_page` と journal の記録はある。undo の履歴に「頁の削除」の種類を足す） | なし | `notes/document.c`・`journal.c`・`main.c`・`menu.c` | **入** |
| C3 | Text Editor | Edit > Go to Line…（Ctrl+L、Replace と同じ panel の作り） | 長い file の移動 | 1h | 低 | なし | `textedit/main.c`・`app.c`・`menu.c` | 任 |
| C4 | 全体 | 設定を覚える: Text Editor（文字の大きさ・折り返し・行番号）、Terminal（Text Size）、PDF Viewer（文書ごとの頁と拡大）、Notes（道具・色・太さ） | 起動のたびに戻らない | 3h（4 app） | 低〜中 | `keiland_preferences_*`（既存）。Terminal は WS090 p015 と file が重なる | 各 app の `main.c` | 任（Text Editor と PDF Viewer だけなら 1.5h） |
| C5 | Image Viewer | Move to Trash（Delete キー、取り消しの chip） | 写真の整理 | 2.5h | 中 | Trash の仕組みは Files の中（`files/`）だけ。libkeiland に共有の API を足すか（main の許可、KEILAND_VERSION）、Image Viewer に複製するか | `imageview/`、libkeiland | 任（API の判断が要る） |
| C6 | Image Viewer | Slideshow（View > Slideshow、3 秒ごと、Esc で止める） | デモで写真を流せる | 1h | 低 | なし | `imageview/main.c`・`menu.c` | 任 |
| C7 | Image Viewer | Open With…（他の app で開く） | 編集の app へ渡す | 2h | 中 | 開ける app の一覧の仕組みが無い（App Home の `apps.conf` に MIME が無い） | `imageview/`、apps.conf の形式 | 外 |
| C8 | Image Viewer | 画像の Copy（clipboard へ PNG） | 他の app へ貼る | 2h | 中 | libkeiui の clipboard は text だけ（image/png の offer が要る、libkeiui の変更は WS090 と調整） | `imageview/`、libkeiui | 外 |
| C9 | Image Viewer | interlace の PNG（Adam7） | 開けない形式を減らす | 1.5h | 低 | なし（libpng-compat か imageview の decoder） | `imageview/image.c` か `libpng-compat` | 任 |
| C10 | PDF Viewer | 文字の検索・選択・copy（= p004） | PDF の基本の機能 | 6h 以上（複数 Phase） | 高 | libpdf に文字の抽出が無い（ToUnicode・CMap・font の encoding の逆引きが要る。`userland/base/libpdf` に `ToUnicode` の参照が無い）。WS127 p004 と libpdf で直列 | libpdf、pdfviewer | 外（ベータ2） |
| C11 | Terminal | scrollback の検索（Edit > Find） | 長い出力の確認 | 2.5h | 中 | WS090 p015（Terminal の scroll の移行）と同じ file | `terminal/` | 任（WS090 p015 の後） |
| C12 | Notes | 頁の画像の書き出し（File > Export Page as PNG） | 共有 | 1.5h | 低 | libpng-compat は読むだけ（`read.c`）。PNG の writer が要る | `notes/`、libpng-compat | 外（PDF で足りる） |
| C13 | スクリーンキーボード | 絵文字の面（= WS102 p022、planned） | 入力の幅 | 3h | 中 | WS102 の担当、compositor の `keyboard.c`（P2 の compositor の作業と調整） | `wayland/keyboard.c` | 任（WS102 で） |
| C14 | 音量 | 5330 の HDA で鳴るか（= WS100 p006 案） | 実機のデモ | ユーザー 10 分 | — | 実機（p007 の回に） | — | 入（p007 で） |
| C15 | Text Editor | 文字の encoding の選択（Shift_JIS など）・印刷 | — | — | — | encoding 変換・印刷の仕組みが無い | — | 外 |

計画エージェントの推奨のまとめ: **C1・C2 を入れる**（計 2h）。C3・C4（Text Editor・PDF Viewer の分）・C6・C9 は時間次第の小物。C5 は libkeiland の
API の判断（Q1・ユーザー）。C10 は p004 としてベータ2。C11 は WS090 p015 の後。C13 は WS102、C14 は p007。

## 5. 既存の WS の残りとの照合（ws.md の表の確かめ）

各 WS の ws.md の Phase の表（2026-10-03 の main）と照らした。**ws.md の表と食い違いは無い**:

- WS079: p001〜p016 は全て cleared、WS は incomplete（残りはユーザーの S8・S9 の 5330・Windows の QEMU の確かめと L3 の実機のペン）。
- WS090: p007・p009・p010・p012 は planning、p015 は（案）planning。他は cleared。
- WS100: p006（案）planning、他は cleared。
- WS102: p010・p011・p012・p013・p022・p014 は planned、他は cleared（p020 も済み）。

## 6. 代表の画面（9 枚、worktree の `build/ws128-p001/reps/`、まとめ `build/ws128-p001/reps-sheet.png`）

`01-imageview-fit.png`（01-splash.png、絵の中の spinner は画像の一部）、`02-imageview-broken.png`（壊れた file の説明）、`03-imageview-chooser.png`、
`04-pdfviewer.png`（S9 の A4 の文書）、`05-notes-pen.png`（S8 の pen の線）、`06-terminal-edit.png`、`07-osk-qwerty.png`、`08-osk-flick.png`、
`09-volume-bar.png`（Settings の Sound と system bar の popup が 30%）。
