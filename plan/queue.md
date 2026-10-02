<!-- awesome-plan project=zedbsd record=queue -->

# Queue / all-agent index

Active Queues: q607（P1）、q609（P2）、q610（P3）、q606（P4）。
Status: active
Main executor / plan writer: Q1（単一 Claude Code セッション、[protocol](agents/protocol.md)）。サブエージェント P1〜P8、N=2。
Last reconciled Queues: [q584](history/queue-q584.md)〜[q589](history/queue-q589.md)。過去の Queue は [Past Log](history/index.md)。次の未予約 ID は q611。

| Queue / attempt | Agent | Phase | Exact scope | State | Approval / checkpoint |
| --- | --- | --- | --- | --- | --- |
| q590 / q590-i01 | P1 | [ws004-p051](ws004/phase051/phase.md) | BUG-134: passthrough 4 回とも再現せず、実機試験 image を作成 | finished / uncleared（ユーザーの実機試験待ち） | user 2026-10-02、P1 5a9da8080 → main 487372080 |
| q594 / q594-i01 | P1 | [ws129-p009](ws129/phase009/phase.md) | デモの image を CI の設定を土台に、boot-test の screendump timeout の調査、3h | finished / cleared（Q1 が full image の build と boot-test PASS で確定） | user 2026-10-02「デモのイメージはCI設定をベースに変更しましょう。」、[lane](agents/P1/queue.md)、P1 22efda2be → main eeecec752 |
| q595 / q595-i01 | P4 | [ws127-p001](ws127/phase001/phase.md) | Files の棚卸し（回帰の取り直し、spec との照合、QEMU の実使用、候補の一覧。source は変えない）、3h | finished / cleared | 継続 dispatch（user 2026-10-02「N=4で週次利用制限に達するまで作業してください」）、[lane](agents/P4/queue.md)、P4 2142b1fc6 → main 3381b12ab（[requirements](ws127/requirements.md)、候補 14 件はユーザーの採否待ち） |
| q596 / q596-i01 | P1 | [ws005-p018](ws005/phase018/phase.md) | WiFi の利用者の流れの調査と契約（source 不変）、3h | finished / cleared（調査と契約案。方式の判断は p019 の前提） | 継続 dispatch（user 2026-10-02）、[lane](agents/P1/queue.md)、P1 3019d674b → main 7cbbd3fc5 |
| q597 / q597-i01 | P3 | [ws115-p001](ws115/phase001/phase.md) | 素の GTK4 の zedBSD 移植の契約（版・依存・libc の不足・host 道具・libwayland ABI・renderer・demo app）、3〜4h | finished / cleared | user 2026-10-02「まずは素のGTK4を移植してください」＋継続 dispatch、[lane](agents/P3/queue.md)、P3 be587f8ae → main 408a31586（[port-contract](ws115/port-contract.md)、判断 D-VER/L1/L2/R1/P1 はユーザーへ） |
| q598 / q598-i01 | P1 | [ws033-p001](ws033/phase001/phase.md) | USB の LAN の後挿し・抜去・carrier・networking.wait を QEMU で通す、3h | finished / uncleared（2026-10-02 user の WiFi 優先の指示で中断、q599 の後に再投入） | 継続 dispatch（user 2026-10-02、ネットワーク優先）、[lane](agents/P1/queue.md) |
| q599 / q599-i01 | P1 | [ws005-p019](ws005/phase019/phase.md) | BUG-138: system bar の WiFi menu の on/off・AP 選択→鍵→接続、利用者の owner（A＋A′）、有線優先の route/DNS、AX211 passthrough で確認、4h | finished / uncleared（networkd の owner の変更は permission system が "Security Weaken" として拒否、ユーザーの明示の承認待ち。system bar の鍵の入力と待ちの slot は実装済み） | 2026-10-02 user「実装をお願いします。優先度高いです。」、[lane](agents/P1/queue.md)、P1 c26fb2c78 → main b67aa0f88 |
| q600 / q600-i01 | P3 | [ws115-p004](ws115/phase004/phase.md) | meson の cross 契約（cross file の生成）と host 道具（gperf 3.3）。版に依らない範囲、4h | finished / cleared | 継続 dispatch（user 2026-10-02「まずは素のGTK4を移植してください」）、[lane](agents/P3/queue.md)、P3 503b94c80 → main 81f77738d |
| q602 / q602-i01 | P3 | [ws115-p005](ws115/phase005/phase.md) | libffi・pcre2・glib 2.84.4 | finished / cleared（QEMU の guest probe PASS・boot-test PASS、license は手での分類） | P3 958b6c060 → main |
| q603 / q603-i01 | P4 | [ws095-p013](ws095/phase013/phase.md) | BUG-139: IME の変換中の文字を本文と同じ大きさで inline に、4h | finished / cleared（QEMU。実機の目視はユーザー） | 2026-10-02 user「テキスト本文と同じ文字の大きさにしたいです。ここはUI/UXの完成度の要」、[lane](agents/P4/queue.md)、P4 12bc714ea → main c0449523a |
| q604 / q604-i01 | P4 | [ws095-p005](ws095/phase005/phase.md) | IME の候補の窓、右上の通知領域の IME の status（A／あ）、key repeat、ime-p004.sh の kill の修正、4h | finished / cleared（QEMU、実機の目視はユーザー） | user 2026-10-02「IMEのステータスを画面右上の通知領域に追加してください」＋継続 dispatch、[lane](agents/P4/queue.md)、P4 4ca0b3e80 → main 5634eea24 |
| q605 / q605-i01 | P3 | [ws115-p006](ws115/phase006/phase.md) | libpng・freetype・harfbuzz（libcxx）・fontconfig、4h | finished / cleared（QEMU の fc-list・text-probe・boot-test PASS） | 継続 dispatch（user GTK4 移植）、[lane](agents/P3/queue.md)、P3 cbbc1607f → main 3c33fb259 |
| q606 / q606-i01 | P4 | [ws129-p010](ws129/phase010/phase.md) | desktop の全 app と試験でない base の全 program を CI と試験の config へ（CI に試験は入れない）、3h | in-progress | user 2026-10-02「CIもテスト用も、desktopのアプリはすべてコンフィグを追加」「テストでないbaseはすべて入れてください」、[lane](agents/P4/queue.md) |
| q608 / q608-i01 | P3 | [ws115-p007](ws115/phase007/phase.md) | pixman・cairo・fribidi・pango、4h | finished / cleared（QEMU で pango の日本語の PNG・boot-test PASS） | 継続 dispatch（user GTK4 移植）、[lane](agents/P3/queue.md)、P3 03f00921d → main db8f21553 |
| q609 / q609-i01 | P2 | [ws099-p023](ws099/phase023/phase.md) | BUG-136 Gears・Notes のタイトルバー、BUG-137 Terminal の遅れ、直す前から落ちる C3/C4/C8/C9 の試験、C9 ×5、4h | in-progress | user 2026-10-02 の実機の指摘＋継続 dispatch、[lane](agents/P2/queue.md) |
| q610 / q610-i01 | P3 | [ws115-p008](ws115/phase008/phase.md) | gdk-pixbuf・jpeg・tiff・graphene・libepoxy（SONAME の patch）・libxkbcommon、4h | in-progress | 継続 dispatch（user GTK4 移植）、[lane](agents/P3/queue.md) |
| q607 / q607-i01 | P1 | [ws005-p024](ws005/phase024/phase.md) | 起動時の enable で保存済み AP に自動再接続（実装と QEMU の試験。実 AP の試験は承認待ち）、3h | in-progress | 2026-10-02 user「networkdが有効になって、net wifi enableされたとき…コンソール起動でもWiFi接続は自動」「WiFiの自動接続のテストもお願いします。」 |
| q601 / q601-i01 | P1 | [ws118-p001](ws118/phase001/phase.md) | 5320 の遠隔 log 用 image（sshd、USB LAN、3 種、手順書）、QEMU で SSH まで、実機は使わない、3h | paused（q599-i04 の割り込み） | 継続 dispatch（user 2026-10-02、5320 は「SSHDを起動してリモート実機でログを取れるようなイメージを作成」）、[lane](agents/P1/queue.md) |
| q599 / q599-i04 | P1 | [ws005-p019](ws005/phase019/phase.md) | 実際の join（2.4/5GHz）・DHCP・B3 の切替えを 5330 の AX211 passthrough で、2h | finished / uncleared（permission system が「Third-Party Attack」として拒否、ユーザー本人の直接の承認か permission rule 待ち。guest に入力なし、host 復元済み） | user が試験用 AP の資格情報を chat で提供（記録には書かない） |
| q599 / q599-i02 | P1 | [ws005-p019](ws005/phase019/phase.md) | BUG-138 の続き: networkd で network group の利用者に WiFi の制御を許可、desktop からの on/off・join を AX211 passthrough で確認、有線優先の route/DNS、4h | finished / uncleared（permission system が relayed approval を受け付けず再拒否、ユーザーの直接の許可待ち） | 2026-10-02 user（明示の承認）:「WiFiの制御は、networkグループに入っているユーザには許可する、でどうですか？」、[lane](agents/P1/queue.md) |
| q599 / q599-i03 | P1（generation2） | [ws005-p019](ws005/phase019/phase.md) | networkd で network group の利用者に WiFi の制御を許可、desktop からの on/off・join、AX211 passthrough、有線優先 route/DNS、4h | finished / uncleared（実装と QEMU・passthrough の on/off・一覧・鍵の field まで。実際の join は試験用 AP の資格情報待ち） | 2026-10-02 user「WiFiの制御は、networkグループに入っているユーザには許可する」「networkdについて承認しますので、権限確認は表示されたら再度承認します。」、P1 c662d6e33 → main 364ea5f22 |
| q591 / q591-i01 | P2 | [ws099-p020](ws099/phase020/phase.md) | BUG-125 の原因特定と compositor の修正（phase.md の範囲）、4h | finished / uncleared（3 症状の原因と修正、p076 は harness 失敗を除き 19/19。C3・C4・C8・C9 p072 は直す前から FAIL で残る） | user 2026-10-02「作業を開始しましょう。」（Q1 提案の P2/P3/P4 の最初の Queue）、[lane](agents/P2/queue.md)、P2 8bc057b36 → main e90d816c2 |
| q592 / q592-i01 | P3 | [ws114-p007](ws114/phase007/phase.md) | CSD/SSD の残り5点（新 attempt、phase.md の範囲）、3h | finished / cleared | user 2026-10-02「作業を開始しましょう。」（Q1 提案の P2/P3/P4 の最初の Queue）、[lane](agents/P3/queue.md)、P3 597841d2a → main 3b985ae4c |
| q593 / q593-i01 | P4 | [ws095-p012](ws095/phase012/phase.md) | IME の辞書を千語に拡張し held-out で測る（phase.md の範囲）、4h | finished / cleared | user 2026-10-02「作業を開始しましょう。」（Q1 提案の P2/P3/P4 の最初の Queue）、[lane](agents/P4/queue.md)、P4 c9d9187ad → main c7bbbf06a（held-out A 64→109/125、B 47→81/110、host 203 tests） |

## 直近の終了 Queue の残り（再開の候補、承認ではない）

| Queue | Phase | 結果 | 残り |
| --- | --- | --- | --- |
| [q584](history/queue-q584.md) | [WS074 p172](ws074/phase172/phase.md) | uncleared | browser 全文 review 97/209、残 112。p172 が後続 browser Phase の前提 |
| [q585](history/queue-q585.md) | [WS112 p001](ws112/phase001/phase.md) | uncleared | D1（Fedora/Arch の boot 適用）のユーザー回答待ち |
| [q586](history/queue-q586.md) | [WS113 p001](ws113/phase001/phase.md) | uncleared | D-ATOMIC 未決 |
| [q587](history/queue-q587.md) | [WS114 p007](ws114/phase007/phase.md) | uncleared | CSD/SSD 実装済み、最終 runtime・clipboard・Textedit・boot 等 |
| [q588](history/queue-q588.md) | [WS094 p007](ws094/phase007/phase.md) | 部分 cleared / whole uncleared | 実機・guest・boot 等 |
| [q589](history/queue-q589.md) | [WS099 p017](ws099/phase017/phase.md) / BUG-125 | uncleared | 低 overhead 診断の準備のみ、guest 未実施 |

## Upcoming Work Outlook

2026-10-02 追加の予約: P2 → q591 の後 ws099-p023（BUG-136 Gears・BUG-137 Terminal のタイトルバー）→ ws099-p021。P4 → q595 の後 ws095-p013（BUG-139）→ ws095-p005（IME の status を右上の通知領域に）→ ws129-p010（全 desktop app を CI と試験の config へ）→ ws089-p010。P1 → q599 の後 ws033-p001 を再投入 → ws118-p001。

継続の dispatch（2026-10-02 user「N=4で週次利用制限に達するまで作業してください」「作業を開始しましょう。」）: Q1 は Master の Outlook の fg019 の planned Phase を各担当の線に順に投入する。ユーザーの判断が要る Phase（planning）は投入しない。

担当の線は Master の [Upcoming Work Outlook](master.md#upcoming-work-outlook)。予約（承認は投入時に確認）: P1 → デモの image を CI 設定の土台へ → ws005-p018 → ws033-p001 → ws118-p001。P2 → ws099-p021 → ws094-p014。P3 → ws115-p001（素の GTK4 移植の契約）→ ws115-p004〜。P4 → ws127-p001 → ws089-p010 → ws095-p005。
