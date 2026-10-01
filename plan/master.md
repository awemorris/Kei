<!-- awesome-plan project=zedbsd record=master -->

<!-- awesome-plan-current:start -->
Active Queue: なし（q551 finished、WS109 p001 cleared）
Current Focused Goal: fg016 — WS109 FreeBSD15 native Keiland移植。fg015達成、fg010/fg013の未達を保持。
Next（2026-10-02 に更新）: WS109 native guest/ABIを確認しbuild/backend移植を先行。実機はユーザー準備、GPU/WiFi最終関門を保持。
IME（WS095）は一時的に人間が作業中（エージェントに割り当てない）。WS074 は 2026-09-30 のユーザー指示「Run ws074」で再開し、p099まで cleared。
優先順位（2026-09-30 夜 ユーザー）: 最優先の WS103 は 2026-10-01 に完了。2026-10-01 ユーザー指示で WS104 → WS105 を次の目標にし、q538で完了した。その後の既存候補順は WS099・WS079・WS090・WS089・WS094・WS100・WS078・WS102、WS074 はデモ critical の中位。
<!-- awesome-plan-current:end -->

# zedBSD Master

[GitHub Project](https://github.com/users/awemorris/projects/2) ·
[Queue](queue.md) · [Guardrail](guardrail.md) · [Future Work](future-work.md) ·
[Bug Board](known-bugs.md) · [Past Log](history/index.md) · [設定](config.md)

## 目的・利用者・最終成果

- **目的**: 寛容なライセンスで企業が自由に使える UNIX 互換 OS を、GPL の Linux kernel に依存せずに作る。
- **利用者**: OS を組み込んで独自のディストリビューションを作る開発者・企業と、デスクトップ・ラップトップ・SBC で使う個人。
- **最終成果**: Linux/Android を置き換えられる水準のカーネルとユーザランド、最小の HAL による移植契約、用途別に構成・配布できる仕組み。
- **範囲**: kernel、HAL、driver、libc、base の userland、デスクトップ（zdesktop）、外部 package のクロスビルド、インストーラ、文書。
- **範囲外**: Linux の kernel ABI・DRM の互換、Mesa 流の user mode driver、正式な UNIX 認証・Vulkan CTS 認証の取得（主張しない）。
- **制約**: HAL の変更は差分ごとの事前承認（[Guardrail](guardrail.md)）。独立実装とライセンスの境界（[設計方針](master-design-policy.md)）。

## Objectives

- **O1**: 寛容なライセンスで企業が自由に使いやすい UNIX 互換システムを、GPL の Linux kernel に依存せず、Linux/Android を置き換え可能な水準で提供する。
- **O2**: デスクトップ、ラップトップ、SBC、タブレット、モバイルなど様々な規模で動くカーネルとユーザランドを提供し、開発者が独自ディストリビューションを自由にカスタマイズ・リブランディング・配布できるようにする。
- **O3**: UNIX/BSD/Linux の遺産から現代のシステムに必要なエッセンスを抽出し、networkd、netconf、service などをシンプルで一貫した仕組みとして再実装する。
- **O4**: ページベース MMU を備える 32bit/64bit コンピュータへ UNIX 互換 OS を確実に移植できる、明確で最小限の HAL を定義し、人類の共有知とする。
- **O5**: AI 時代の OSS のあり方を、大規模な AI 活用開発を通じて探索し、成果・失敗・人間の判断を再利用可能な知見として共有する。

## Milestone Goals

Milestone の達成は所属 WS の完了数ではなく、到達点の証拠で判定する。現時点で completed の Milestone は無い。

| Milestone | Objective | 受け入れの核 | 進捗 | Primary WS |
| --- | --- | --- | --- | --- |
| **MG001** 継続開発できる基盤 | O4, O5 | 文書化した環境で build でき、設計境界・規約・試験・制限を追跡できる | toolchain（WS021）・build tool（WS010）・x86 HAL の規約（WS023）は完了。文書（WS009）と試験資産の整理（WS026）が残る。テスト配置の整理はWS106で計画。vmunix の LTO（WS053）は完了 | WS009, WS010, WS021, WS023, WS026, WS047, WS053, WS106 |
| **MG002** UNIX アプリケーションの実行基盤 | O1 | process・memory・libc・loader/TLS の対応範囲を互換性台帳と代表アプリで確認できる | TLS（WS022）と外部 package の導入（WS032）は完了。base の utility の POSIX 化（WS043）は完了。POSIX 台帳（WS001）、アプリ導入（WS034）、sh（WS042）が進行中 | WS001, WS022, WS032, WS034, WS042, WS043, WS045, WS046, WS061 |
| **MG003** 対象機へ導入して単独起動 | O2, O4 | 合意した機種・媒体でインストール後の単独起動と login を確認できる。実機と QEMU の証拠を分ける | インストーラ（WS019）と Intel Mac（WS020）は完了。4 機種の実機受け入れ（WS028）が残る | WS003, WS004, WS019, WS020, WS028 |
| **MG004** データの保持とメモリ/ストレージの実用 | O1, O2 | 永続化、低メモリ時の進行、媒体世代、既定構成の性能を確認できる | swap（WS016）、UFS（WS024）、I/O・cache（WS025）は完了。実機の性能の一部は未測定。UFS の directory は 12 block まで育つ（WS054、完了） | WS016, WS024, WS025, WS054, WS057, WS058, WS059, WS060 |
| **MG005** 一貫したネットワーク/サービス管理 | O1, O2, O3 | networkd・netconf・service の責務・設定・操作が一貫し、永続化と失敗後の復旧を確認できる | サービス（WS002）、net console（WS011）、service console（WS012）は完了。有線 LAN の常駐管理（WS005・WS033）が残る | WS002, WS005, WS011, WS012, WS033 |
| **MG006** グラフィカルな操作環境 | O2 | 入力・描画・ウィンドウ・端末・GUI ツールの一連の操作を確認できる | 入力（WS006）、Noct/BeUI（WS008）、標準 Vulkan（WS030）、即時起床（WS041）は完了。**Wayland デスクトップ（WS035）が fg010 の中心**。WS104 の OS 境界は A1〜A6 と全体回帰で完了、LinuxのWS105はL1〜L9・全文規約/両OS最終回帰でcompleted（fg012達成、既知resizeはユーザー許可のtracking）。デモ実機を含むMG006全体は未完了 | WS006, WS007, WS008, WS014, WS017, WS029, WS030, WS031, WS035, WS037〜WS039, WS041, WS068, WS104, WS105, WS107, WS109 |
| **MG007** 用途別の独自ディストリビューション | O1, O2 | 第三者が用途別に構成し、独自ブランドで build・配布できる | 担う作業は一部だけ（WS013・WS015 は Future Work に保留）。WS105独立/opt build・installとWS108の2OS native deb/QEMU検証・CI/release定義をverified、MG007全体は未充足 | WS013, WS015, WS108 |
| **MG008** 最小 HAL の移植契約と異種機での実証 | O4 | HAL 契約・移植手順と異種/レトロ機での実証を公開する | source の所有の整理（WS018）と時間の単位（WS040）は完了。他 platform への反映（WS036、aarch64 を含む）と PowerPC（WS027）、rpi4 の開発環境（WS044）が残る | WS018, WS027, WS036, WS040, WS044 |
| **MG009** AI 活用 OSS 開発の知見の公開 | O5 | 設計権限・レビュー・変更追跡・失敗からの回復の事例と根拠を公開する | 担う作業が未定義 | なし |

## Current Focused Goals

| Goal | 当面の成果 | Milestone | 担当 | 出典 |
| --- | --- | --- | --- | --- |
| **fg010** | **2026-10-17 の Open Source Conference Tokyo Fall のデモに向けて、Kei Operating System を仕上げる**: Dell Latitude 5330 の実機（内蔵 LCD、USB boot。HDMI の touch LCD は 2026-09-29 に外した）で graphical login から Keiland のデスクトップ、demo critical のアプリ（Image Viewer・Text Editor・Files・Settings・Notes・PDF Viewer・ブラウザ（amazon.co.jp）・terminal）が動く | MG006 | [WS099](ws099/ws.md)（Keiland のデモの仕上げ、WS035 の後継）、[WS075](ws075/ws.md)（i915）、[WS089](ws089/ws.md)（Settings）、[WS091](ws091/ws.md)・[WS092](ws092/ws.md)・[WS093](ws093/ws.md)（画像・text・Files からの起動）、[WS079](ws079/ws.md)（Notes・PDF Viewer）、[WS074](ws074/ws.md)（ブラウザ）、[WS081](ws081/ws.md)（touch） | 2026-09-24 ユーザー指示、2026-09-29 のデモ critical の追加（画像 viewer と text editor）、2026-09-30 に記述を更新 |

| **fg016** | Linux共通描画を再利用しFreeBSD15 native compositor/主要appとaudio/network/WiFi backendを実検証する（WS109 F1〜F5） | MG006 | [WS109](ws109/ws.md) | 2026-10-02 user「WS108の完了後、WS109の実行をお願いします。」。WS109自身の既存目標をfocusとする |

デモの platform は amd64 の実機（Dell Latitude 5330、HDMI + USB、2026-09-28 ユーザーの回答）。開発の試験は QEMU（amd64）で行い、実機の証拠と分けて書く。以前の focus（fg004 インストーラの実機、
fg005 有線 LAN、fg007 HAL の可読性、fg009 PowerPC）は定義を残すが、現在は優先しない。

### 達成した Focused Goal（2026-10-01）

| Goal | 成果 | Milestone | 担当 | 出典/結果 |
| --- | --- | --- | --- | --- |
| **fg012** | Keiland の OS の境界を zedBSD の上で整理し、既存の振る舞いを保ってから Linux の build と OS module を実装する。達成は WS104 の A1〜A6、WS105 の既存の受け入れ条件による | MG006 | [WS104](ws104/ws.md) → [WS105](ws105/ws.md) | 2026-10-01 ユーザー「WS104とWS105が、次のあなたの目標です」。同日の WS104 完了までの自律実行指示。WS104 A1〜A6とWS105 L1〜L9/最終conformance verified、2026-10-01達成（q538）。優先順位と同じ指示から反映、fg010 は保持 |
| **fg013** | WS106 の対象テスト・見本を userland/tests に集約し、既存の選択・実行・デモを保つ | MG001 | [WS106](ws106/ws.md) | 2026-10-01 ユーザー「WS106を実行してください。」。WS106 の既存目標だけをfocusとし、既存fg010は保持 |
| **fg014** | libbrowserがengineを所有し、標準Vulkanと抽象入力で独立clientから利用できる | MG006 | [WS107](ws107/ws.md) | 2026-10-02 ユーザー「WS107を実行してください。」。B1〜B5 verified/q544で達成、WS106/fg013の未達を保存 |
| **fg015** | Debian13/Ubuntu26.04のQEMU guest native build/dpkg導入・動作とCI/release files | MG007 | [WS108](ws108/ws.md) | 2026-10-02 user指定targets/guest/CI/release、q549/P1〜P5達成。remote未実行 |

### fg010 の達成基準: デモの台本（2026-09-30 ユーザー「WS099のゴールも、明確な達成基準がないような気がします。それはFGに入れて、WSでは、このソフトがこういう基準を満たす、という明確なゴールを設定したいです。ソフトごとにそれをWSで作りましょう」）

下の全ての場面が、Dell Latitude 5330 の実機（内蔵 LCD、`display=edp`、デモの image）で、崩れ・止まり・読めない文字・操作できない所なく通ること。
各場面を支えるソフトの WS が、それぞれの達成基準を持つ（WS の欄）。案は 2026-09-30 main、ユーザーの確認待ち。

| # | 場面 | 見せること | 支える WS |
| --- | --- | --- | --- |
| S1 | 起動 | 電源から Kei の起動画面、greeter | WS084（i915 の引き継ぎ）、WS099 |
| S2 | login | kei で login、デスクトップ（壁紙・system bar・デスクトップの icon） | WS099、WS094 |
| S3 | App Home | 左上から App Home、app の一覧、検索 | WS099 |
| S4 | Files | folder の移動、表示の切り替え、scroll（mouse・touch） | WS093・WS071 の後継（必要なら新しい WS）、WS081 |
| S5 | 画像 | Files から png・jpg → Image Viewer、拡大・pan・前後 | WS091（完了） |
| S6 | text | Files から txt → Text Editor、編集・保存（Save As） | WS092（完了）、WS090 |
| S7 | Settings | 壁紙の差し替え、窓の透明度、検索 | WS089 |
| S8 | Notes | 右上の角の swipe で Notes、pen で書く、下の端から上への swipe（か Esc）で窓に（2026-09-30 ユーザー、ws099-p015） | WS079、WS099 |
| S9 | PDF Viewer | PDF の頁送り・拡大 | WS079 |
| S10 | Terminal | `ls`・矢印の履歴・Tab の補完・`less` | WS086・WS087（完了） |
| S11 | 窓の操作 | 10 個ほどの窓で移動・resize・Wiseview・最大化 | WS099、WS075 |
| S12 | 音量 | system bar の音量の icon で音量を変え、確かめの音が鳴る | WS100 |
| S13 | GPU の compute | Noct の見本を GPU と CPU で走らせ、時間を比べる | WS101 |
| S14 | スクリーンキーボード | 右下の角の swipe で flick（日本語）、左下の角の swipe で QWERTY と手書きの面、Text Editor に打つ | WS102 |
| S15 | 終わり | Log Out → greeter、Shut Down | WS099 |

ブラウザ（WS074）はユーザー指示で再開して p099 まで cleared。IME（WS095）は人間が作業中。ブラウザを台本に足すかは引き続き判断待ち。

### fg010 に必要な判断（2026-09-30 夜の自走で出たもの。2026-09-30 朝に全て決定）

| # | 判断 | 既定の案（判断まで、これで進める） | 要る時期 | 出典 |
| --- | --- | --- | --- | --- |
| 1 | WS101 D1: libglesv2 が「OpenGL ES 3.1」を名乗るか | **決定（2026-09-30 朝 ユーザー「名乗ってOKです」）**: compute を持つ device では名乗り、未実装の 3.1 の関数は error の stub（WS068 の「実装した版を名乗る」方針の例外） | — | [WS101 design](ws101/design.md) |
| 2 | WS101 D2: Noct の build の変更（toolchain）: accel を ON にする | **決定（2026-09-30 朝 ユーザー「許可、構成で選ぶ」）**: 構成の `ZEDBSD_NOCT_ACCEL := y` の amd64 の `/bin/noct` だけ ON。toolchain の変更は main が lock を外して行う | — | 同 §4.1 |
| 3 | WS101 D3: G3 の見本の大きさ N = 4,000,000 と、CPU との倍率の目標 3 倍以上（伸ばせれば 10 倍） | **決定（2026-09-30 朝 ユーザー「このまま」）**。CPU の時間を測ってから見直す | — | 同 |
| 4 | WS101 D5: デモの見本は整数だけでよいか | **決定（2026-09-30 朝 ユーザー「整数だけでよい」）**。Noct の意味は変えない | — | 同 |
| 5 | fg010 の台本（S1〜S14）と WS099 の基準 C1〜C10・WS100 の A1〜A7 の数値 | **決定（2026-09-30 朝）**: ユーザー「案のまま確定」。台本 S1〜S14、WS099 の C1〜C10（C6 は 50 ms を保つ）、WS100 の A1〜A7 | — | 上の表、各 ws.md |
| 7 | WS099 の C7: 空の一覧の案内（Files の「Files you open appear here.」、比 1.8〜2.0）を contrast の基準に入れるか。副次の文字の色を `0x6b7585` → `0x56606f` に暗くした（main が許可、Settings と Files、2026-09-30 夜） | **決定（2026-09-30 朝）**: ユーザー「このままでよい」。副次の文字の色 `0x56606f` を保ち、空の一覧の案内は基準の外 | — | ws099-p005 |
| 8 | WS100: Settings の Sound の頁で音量を変えられるようにするか（p005、基準 A1〜A7 の外） | **決定（2026-09-30 朝）**: ユーザー「入れる」。WS100 p005 で Sound の頁に slider と mute を置き、system bar と同じ設定を共有する | — | [WS100 design](ws100/design.md) |
| 6 | WS099 の C6（窓 10 個で pointer の移動から表示まで中央値 50 ms 以内）: 新しい物差し（`c6.py`、5 run・200 試料）で実機の passthrough の中央値は 121.5 ms・p90 173 ms（compositor 約 9.5 frame/s）。基準を保つか緩めるか | **決定（2026-09-30 朝 ユーザー「50 ms を保つ」）**: 基準は保ち、WS075 p026 の分析と直しで近づける。届かなければ 10/10 に見直す | — | ws075-p024 の途中報告 |

## Workstream registry

| WS | Primary | 内容 | 状態 | 再開点 |
| --- | --- | --- | --- | --- |
| [WS001](ws001/ws.md) | MG002 | POSIX.1-2024 準拠 | incomplete | p033〜p039 cleared（patch、df・du、who、stty、dirname、mktemp・install・base64、xargs）、main へ merge。p040 mesg は実装を merge、uncleared（console の case は BUG-067 待ち）。以後はユーザーの指示のときだけ |
| [WS002](ws002/ws.md) | MG005 | システムサービス | completed | — |
| [WS003](ws003/ws.md) | MG003 | 旧実機 bring-up（終了・再利用禁止） | completed（ユーザー判断で終了） | 未完了は WS027・WS028・F-004 へ |
| [WS004](ws004/ws.md) | MG003 | ハードウェア拡張 | incomplete | NVMe 実機・転送・driver 共通化 |
| [WS005](ws005/ws.md) | MG005 | ネットワーク・WLAN | incomplete | 有線 LAN の常駐管理と起動時の待機（p013〜p017） |
| [WS006](ws006/ws.md) | MG006 | 入力と evdev | completed | — |
| [WS007](ws007/ws.md) | MG006 | グラフィックス・デスクトップ（旧） | incomplete | p004 の再現条件、amd64 の残件 |
| [WS008](ws008/ws.md) | MG006 | Noct と BeUI | completed | — |
| [WS009](ws009/ws.md) | MG001 | 文書 | incomplete | DOC-54（GPU の文書） |
| [WS010](ws010/ws.md) | MG001 | Noct の script と build tool | completed | — |
| [WS011](ws011/ws.md) | MG005 | ネットワーク設定 console | completed | — |
| [WS012](ws012/ws.md) | MG005 | サービス管理 console | completed | — |
| [WS013](ws013/ws.md) | MG007 | CPAR（container 分割） | incomplete（Future Work F-002 に保留） | 昇格まで再開しない |
| [WS014](ws014/ws.md) | MG006 | GPU framework・virtio-gpu・Wayland の土台 | incomplete | p004（最終 API と規約の確認） |
| [WS015](ws015/ws.md) | MG007 | μITRON リアルタイム領域 | planning（Future Work F-003 に保留） | 昇格まで再開しない |
| [WS016](ws016/ws.md) | MG004 | 実行時の swap 制御 | completed | — |
| [WS017](ws017/ws.md) | MG006 | LFB 描画の高速化 | planned | mmap・Xzed の高速描画 |
| [WS018](ws018/ws.md) | MG008 | kernel の source 所有と interface の統合 | completed | — |
| [WS019](ws019/ws.md) | MG003 | インストールとディスク管理 | completed | — |
| [WS020](ws020/ws.md) | MG003 | Intel Mac の UEFI 起動 | completed | — |
| [WS021](ws021/ws.md) | MG001 | x86 LLVM toolchain と sysroot | completed | — |
| [WS022](ws022/ws.md) | MG002 | ELF の TLS | completed | — |
| [WS023](ws023/ws.md) | MG001 | x86 HAL の規約準拠 | completed | — |
| [WS024](ws024/ws.md) | MG004 | 64-bit UFS の一本化 | completed | — |
| [WS025](ws025/ws.md) | MG004 | I/O・cache・物理メモリの再設計 | completed | — |
| [WS026](ws026/ws.md) | MG001 | 試験資産の整理 | planning | Phase 未定義 |
| [WS027](ws027/ws.md) | MG008 | PowerPC 移植 | planned | p001〜p007 |
| [WS028](ws028/ws.md) | MG003 | インストーラの実機動作（4 機種） | planning | NVMe の未動作の切り分け |
| [WS029](ws029/ws.md) | MG006 | i915 native GPU driver | incomplete | cold VFIO attach の間欠的な停止ほか |
| [WS030](ws030/ws.md) | MG006 | 標準 Vulkan 1.0 と直接表示 | completed | — |
| [WS031](ws031/ws.md) | MG006 | i915 native Vulkan 実行器 | incomplete | p015〜p048 planning |
| [WS032](ws032/ws.md) | MG002 | 外部 package のクロスビルド（clang・OpenSSL・OpenSSH） | completed | — |
| [WS033](ws033/ws.md) | MG005 | networking サービスと有線インタフェースの管理 | incomplete | 抜き差しの実機確認 |
| [WS034](ws034/ws.md) | MG002 | アプリケーション拡充と kernel・libc の是正 | incomplete | package の導入 |
| [WS035](ws035/ws.md) | MG006 | デスクトップ環境とアプリケーション | completed | 2026-09-30 ユーザーの判断で閉じた（目標が 2026-09-23 のまま古く、ゴールが不明確）。p001〜p138: compositor・sessiond・greeter・lock・Keiland の app・audiod・起動の短縮ほか。Chromium は取り消し（ユーザー「独自にBrowserを書いているから」）。デモまでの仕上げは WS099。`plan/ws035/tests/` は共有の道具として残す（plan/tools への移動は後の整理） |
| [WS036](ws036/ws.md) | MG008 | amd64 の成果を他 platform へ（aarch64 を含む） | completed | 2026-09-27 完了（p021 全 platform の回帰と規約、p026〜p029、p027 は案 A: boot の parameter の parser を緩めた）。実機は未実施。toolchain の cache（zedbsd8）は 2026-09-27 に rev-0 へ upload 済み |
| [WS037](ws037/ws.md) | MG006 | NVIDIA GPU（予約） | planning | 番号のみ |
| [WS038](ws038/ws.md) | MG006 | Intel Arc dGPU（予約） | planning | 番号のみ |
| [WS039](ws039/ws.md) | MG006 | AMD RDNA GPU（予約） | planning | 番号のみ |
| [WS040](ws040/ws.md) | MG008 | 時間の単位を tick 周期から導く | completed | — |
| [WS041](ws041/ws.md) | MG006 | 起きた thread の即時実行 | completed | — |
| [WS042](ws042/ws.md) | MG002 | `/bin/sh` の POSIX 互換性 | completed | — |
| [WS043](ws043/ws.md) | MG002 | base の utility を POSIX に（sed・grep・awk ほか） | completed | — |
| [WS045](ws045/ws.md) | MG002 | base の text utility の GNU 拡張（sed・awk・grep ほか） | incomplete | p001〜p009 cleared（サブエージェント、2026-09-27 に main へ merge）。GNU の case 515/515、POSIX 492/492、7 package の configure の比較が同じ。3 点は 2026-09-27 夜に決定（dirname は GNU 風、mktemp・install・base64 は追加、xargs は WS001 でよい → WS001）。amd64 以外の image は未実施 |
| [WS046](ws046/ws.md) | MG002 | GNU 互換の make（autotools の出力を実行できる範囲。並列・jobserver は WS064） | incomplete | p002〜p004・p006 cleared。p007 uncleared（BUG-033 の主因を直した）。p009・p012 cleared（BUG-033: configure 204〜252 → 91 秒、link 0.36 秒、file の fault 15 µs/page）。p013 cleared（libc の mount の一覧の API。coreutils の cross build が通った）。次は p014（p011 の当て直し）・p005 |
| [WS047](ws047/ws.md) | MG001 | build.sh と Noct による build system（TUI・kernel・base・packages を別の system に。Makefile は当面残す） | planning | p001 調査と設計 |
| [WS048](ws048/ws.md) | MG008 | Raspberry Pi 4 の USB（PCIe・VL805 の xHCI・USB キーボード） | incomplete | p001〜p003 cleared（FDT、brcmstb の PCIe、firmware の mailbox と VL805 の firmware。host 試験と QEMU の起動、実機は未実施）。p004 cleared（承認済みの hal.h の差分 `hal_pmem_map_uncached` を適用、実機は未実施）。p005 は config の有効化が残り uncleared。2026-09-27 サブエージェント、main へ merge |
| [WS044](ws044/ws.md) | MG008 | rpi4 を開発に使える形に（console の font、FAT32 の boot、lldb） | incomplete | p001 font・p002 FAT32 の boot partition（QEMU）・p005 cleared。p003（lldb）ほかは WS036 の agent。実機は未実施 |
| [WS049](ws049/ws.md) | MG003 | kernel 内の ACPI AML interpreter | incomplete | p001〜p006・p010〜p015 cleared（p006 kernel への統合: 承認済みの `acpi.rsdp` の差分を適用、amd64 の既定で ACPI の driver が起動、guest の `/dev/acpi` が host の dump と一致。2026-09-27 merge）。次は p007。ACPI はデスクトップが片付くかリミットが余るとき（2026-09-27 方針） |
| [WS050](ws050/ws.md) | MG003 | USB-C の UCSI driver | planning | WS049 が前提 |
| [WS051](ws051/ws.md) | MG006 | USB-C の DisplayPort Alternate Mode | planning | WS050 と i915 の display が前提 |
| [WS052](ws052/ws.md) | MG003 | 電源管理（S0i3、modern standby、`/dev/system` で制御。S3・S4 は対応しない） | planning | WS049 が前提 |
| [WS053](ws053/ws.md) | MG001 | clang/LLVM の LTO を vmunix に安全に適用する（優先度高め） | completed | 4 platform の vmunix は既定で full LTO（HAL を含む）。実機はユーザー |
| [WS054](ws054/ws.md) | MG004 | UFS の directory を複数の block に育てる（BUG-038） | completed | 直接の 12 block まで。実機はユーザー |
| [WS055](ws055/ws.md) | MG001 | zedBSD の clang が link に `--undefined-version` を既定で渡す（F-009） | completed | 2026-09-27 完了: zedBSD の clang の linker に `--undefined-version` を既定で（LLVM の patch を zedbsd8 へ）。main の toolchain を zedbsd8 に切り替えた |
| [WS056](ws056/ws.md) | MG002 | POSIX の試験と utility の不具合を直す（BUG-034・035・037、実行中に見つけた BUG-042〜044） | completed | 2026-09-27 完了（p001・p002 cleared。BUG-046 は console の `POSIX-R2.ELF` 10 回連続 status 0 で閉じた、BUG-068・069 は WS073）。試験は plan/tools/posix/ |
| [WS057](ws057/ws.md) | MG004 | 仮想メモリの reserve と commit の分離と commit の swap の裏打ち（over commit 禁止）の確認と修正（design policy 10） | completed | 分離と拒否は実装済み、BUG-048 を修正。裏打ちは物理 + swap のまま（ユーザーの決定） |
| [WS058](ws058/ws.md) | MG004 | cache の大きさを現代の機械向けに見直す（主記憶 4 GB・swap 16 GB 前提、design policy 10） | completed | p001・p002 cleared。buffer 物理/8、page cache 物理/2、object cache 256、file 2048、inode 2048、overlay 4096、I/O pool 64 MiB。8192 級は F-013（動的確保と hash）の後 |
| [WS059](ws059/ws.md) | MG004 | disk の無い mount にも `st_dev` を与える（BUG-047） | completed | p001 cleared。`mount_device_number()`。`df` が全 mount を出す |
| [WS060](ws060/ws.md) | MG004 | UFS の journal の commit を batch にして名前の操作を速くする（BUG-040）。journal を既定にする前提（WS063） | completed | 2026-09-27 完了（規約は WS063-p002 で）。p001 は p002・p003 に置き換えて canceled |
| [WS061](ws061/ws.md) | MG002 | expat の configure と compile を Linux と同等の水準にする（fg011） | incomplete | 受け入れの計測は達成（q449 の後）: configure 8.2〜8.9 秒（host 10.7）、make（直列）11.3 秒（host `-j1` 15.5）、`cc t.c -o t` 76〜84 ms（host 83〜85）。残り: 規約の Phase ws061-p011（最後） |
| [WS062](ws062/ws.md) | MG004 | amd64 の disk image を ESP の vmunix・UFS の root partition・swap partition に（2026-09-25 ユーザー指示） | completed | 2026-09-27 完了（p004: 規約の全文。zedimage-host の出力が同じ） |
| [WS063](ws063/ws.md) | MG004 | UFS の journal を既定にする（journal の無い image は mount の時に作る、`nojournal`）（2026-09-26 ユーザー指示） | completed | 2026-09-27 完了（p002: 規約の全文と回帰、crash の試験 v3・v2・root）。v2 の tail の journal は v2 のまま（2026-09-27 ユーザーが案 A で確定）。制限: transaction ごとの解放 block の追跡は 8192 まで |
| [WS064](ws064/ws.md) | MG002 | base の make の並列（`-j`）と、並列の make の時間を host と同等以上に（2026-09-26 ユーザー指示） | completed | 2026-09-27 完了（p003: 規約の全文、make・sh・kernel の lock・vmspace の fork・libc の posix_spawn。guest の make-diff 100/100、fork・vfork・posix_spawn の試験、expat の configure が同じ。時間は未測定） |
| [WS065](ws065/ws.md) | MG002 | `/bin/sh` に POSIX が未規定とする bash 拡張を足す（2026-09-26 ユーザー指示） | completed | 2026-09-27 完了（p004: 規約の全文、host の sh-diff と guest の expat の configure が同じ） |
| [WS067](ws067/ws.md) | MG002 | `/dev/fd` を呼んだ process の descriptor に合わせる（BUG-054、2026-09-26 ユーザー「最優先」） | completed | BUG-054 resolved（QEMU）。p001・p002 cleared |
| [WS066](ws066/ws.md) | MG002 | 動的 link の program の起動を速くする（`ld.so` の最適化）（2026-09-26 ユーザー「あとでやるリスト」） | planning | p001（費用の内訳と設計）。優先度は低い |
| [WS068](ws068/ws.md) | MG006 | EGL と OpenGL ES（と desktop GL 3.0〜4.6）を Vulkan と display 拡張の上に実装する（Wayland とディスプレイ直接の両方）（2026-09-26・27 ユーザー指示） | incomplete | 自前の GLSL compiler（p003 = p015〜p019、GLSL 1.40〜3.30・ES 3.00 と uniform block の p012 = p020・p021）cleared（2026-09-27、サブエージェント、main へ merge）。次は p013（desktop GL 3.0 の context）・p005（GLES 3.0 の API）・p014（GL 3.3〜4.6）。p002・p008・p010・p006 cleared |
| [WS069](ws069/ws.md) | MG006 | zdesktop で X11 の app を動かす（単体の `zdesktop-x11server`、rootless、GLX）（2026-09-26・27 ユーザー指示） | completed | 2026-09-27 完了（q492）。zdesktop-x11server（rootless、窓は Vulkan、GLX）、BUG-057 の修正、Xzed はレトロ用に戻した。残りは F-021・F-024・F-030 |
| [WS070](ws070/ws.md) | MG006 | zdesktop の System Menu: client がメニューの意味を渡し、zdesktop が浮いたタイトルバーとシステムバーに描く（`xdg_toplevel_menu_v1`、libzdesktop で包む）（2026-09-27 ユーザー指示） | completed | completed（2026-09-27）: System Menu と Titlebar（MENU・CONTROLS・TABS）。残りは Future Work（F-042・F-043・F-045）、i915 実機は WS075 |
| [WS071](ws071/ws.md) | MG006 | zedBSD File Manager: Finder 風で zedBSD らしいファイルマネージャ（ホームのダッシュボード、サイドバー、タグ、Quick Look、System Menu）（2026-09-27 ユーザー指示、仕様案は ws071/spec.md） | completed | completed（2026-09-27）: zdesktop-files の最初の版（すりガラスの付箋の pane、タブ、titlebar の CONTROLS、context menu、PNG の thumbnail、DnD、configure_bounds）。残りは Future Work（F-032〜F-041・F-044）、i915 実機は WS075、窓の外への DnD は WS035 |
| [WS072](ws072/ws.md) | MG004 | write cached の UFS の format の lease（BUG-060）と、NVMe の timeout の後の回復で root の mount が ETIMEDOUT になる（BUG-059）（2026-09-27、サブエージェント） | completed | 2026-09-27 完了（p001 BUG-060: write cached の format の lease、p002 BUG-059: NVMe の timeout の後の再発行） |
| [WS073](ws073/ws.md) | MG002 | Bug Board の bug の解消（2026-09-27 ユーザー「バグリストに載っているものを解決するサブエージェントを1つ追加しましょう。」）。WS072・WS056（BUG-046）・WS001（BUG-050）の担当と性能の bug（BUG-027・033）を除く | incomplete | 2026-09-29: BUG-100・102（ping）・104（less）・051（signal の frame が amd64 の red zone を壊していた、`src/kern/signal.c`、QEMU で 1092 session 失敗 0）を解決、BUG-105（USB マウスの HID）は実機待ち、BUG-106 は調査中（BUG-051 の現れか）。次: BUG-039・031、BUG-107。BUG-093 は toolchain のため main の許可待ち。2026-09-29 夜: p040（BUG-030）uncleared: `plan/ws073/tests/usb-stress.sh` で起動の途中の BOT CSW の時間切れを再現（TCG 2 回に 1 回）、xHCI の完了の event の取りこぼしを疑う（未確認）。Resume は phase040 の「次にすること」 |
| [WS074](ws074/ws.md) | MG006 | zedBSD の Web ブラウザ `userland/base/zdesktop-browser`（HTML5 の layout engine → 最適化にこだわらない JavaScript engine の接続 → CSS の準拠と Chrome との比較で目標値を段階的に上げる。JS と Wasm の実行 engine を共通化。画像は libpng-compat・新しい libjpeg-compat、TLS は当面 OpenSSL）（2026-09-27 ユーザー指示） | incomplete | 2026-09-30: p097 cleared（q506）。10公開siteを2 viewportで固定比較。WPT reftest 44/100、Acid2 90.56%、Acid3 9/100。GitHubのES module実行はp098候補 |
| [WS075](ws075/ws.md) | MG006 | i915 の高度化: 今日のデスクトップ（zdesktop の glass・backdrop のぼかし・タブ）とグラフィックス（GLES 2/3、GL 3.0〜3.2）を Latitude 5330 の i915 のネイティブ実行器で動かす（compiler の inlining・F-022・F-023 の不足、性能と安定）（2026-09-27 ユーザー「OpenGL 3.2が問題なければ、それ以降のOpenGLはいったん保留して、i915の高度化に進んでください。」） | incomplete | 2026-09-30: L1（C6 91.3 ms）と L2（p029: blur は窓ごと、既定は無効・Settings だけ有効、C6 67.3 ms）を満たした。L3 は p030 で計測（文字の draw 約 400 で約 10 ms）。ユーザーの指示で描画の高速化を止め、p031（文字の draw をまとめる）は build まで済んだ patch（`phase031/exp/text-batch.patch`）で保留。再開はユーザーが描画の高速化の再開を言うとき |
| [WS076](ws076/ws.md) | MG002 | libc の libm を自前で正しく書き直す（src/libc、誤差 1 ulp 以内、fmod 等は正確）（2026-09-28 ユーザー「libmは独自に書いてください。libcのツリーに入れてください。」） | completed | 2026-09-28 完了（`src/libc/math/`、群 B は全件で正しく丸め、BUG-078 解決）。F-046・F-047 へ移管 |
| [WS077](ws077/ws.md) | MG001 | PC-98 の PCI を有効にする（BUG-024、2026-09-28 ユーザー「Bug024は、PCIを有効にします。」） | planning | **優先度を下げた（2026-09-28 ユーザー「Bug024は優先度を下げます。」）**。p001（調査と設計）。HAL の差分は承認が要る。PC-98 の試験が要るので着手の前に確認 |
| [WS078](ws078/ws.md) | MG006 | Kei Operating System への名前の移行（2026-09-28 ユーザーの決定: OS の名前 Kei、カーネルの内部名 zedbsd、Keiland、`/bin/wayland`・`/bin/xserver`・`/bin/browser`、`KERN_` の接頭辞、ロゴは Kei の 3 文字） | incomplete | 2026-09-28: p002・p003・p006・BUG-080・retro への移動は済み、p004 はほぼ済み。残り: 注釈と log の名前、p005 |
| [WS079](ws079/ws.md) | MG006 | 手書きノート（Notes、筆圧 4096 段階の USB のペンタブレット、PDF に保存し編集の metadata を持つ）と PDF Viewer（scroll と page の swipe）、上の右端から左下へのスワイプで Notes を起動・最前面・全画面（2026-09-28 ユーザー） | incomplete | 2026-09-30: 段 L1（全 Phase）と L2 の QEMU の分（ws079-p016: 台本 S8・S9 を注入の touch と pen で自動で通す、PDF の頁送りの最長 142 ms ≤ 200 ms）を満たした。残り: 実機と Windows の QEMU での確かめ（ユーザー、`plan/ws079/demo-s8-s9-manual.md`）、L3 の実機のペン |
| [WS080](ws080/ws.md) | MG002 | `ld.coff`: Win64 PE32+ の動的ローダ（PE/COFF の mapping・relocation・DLL・import/export・Microsoft x64 ABI・最小の TEB/PEB・GS base）。NT の loader は再現せず `AddressOfEntryPoint` へ直接。互換の DLL は上に積む（2026-09-28 ユーザーの仕様 [spec.md](ws080/spec.md)） | planning | p001（設計）から。GS base は swapgs（案 A）に決定、差分は p001 で承認を得る。path は `/usr/libexec/ld.coff`・`/usr/lib/coff64/`（商標のため Win64 の名前を OS に出さない）。source は `userland/base/ld-coff/`・`userland/desktop/w64/`。橋の DLL は置かず互換の DLL が zedBSD の UAPI を直接呼び Wayland と直接通信。判断待ち: 優先度 |
| [WS081](ws081/ws.md) | MG006 | touch の操作の質: 慣性のある scroll と、低い fps の安い touch panel の数式による補間・予測。HID の driver・compositor・ブラウザ（と Keiland の app）にまたがる計画をこの 1 か所で（2026-09-28 ユーザー） | incomplete | 2026-09-30: L2 の準備 p016 cleared（touch の報告の記録の道具 `plan/ws081/tests/touchlog.c`、Linux の QEMU で 60 Hz・90 Hz の数と欠けを正しく数えた、ユーザーの 3 行の手順 `windows-touch.md`）。次は Windows の QEMU 用の image（p018、P4）→ ユーザーの計測 → L3 の p017 |
| [WS082](ws082/ws.md) | MG002 | Linux の `/dev/kvm` の移植の検討（eventfd 等の非 POSIX の fd の代わりに unix socket の message で MMIO・IRQ の通知。ioctl を直接の移植・別の仕組みでの代替・実装不能に分類）（2026-09-28 ユーザー） | incomplete | p001 cleared（[study.md](ws082/study.md)）。§10 の 11 項目のユーザーの判断待ち |
| [WS083](ws083/ws.md) | MG006 | Vulkan Video の拡張（`VK_KHR_video_queue`・`video_decode_queue`・`video_decode_h264`）と i915 の VCS・MFX の対応、最初の目標は H.264 の decode（2026-09-28 ユーザー。OSC のデモには必須ではない） | planning | p001（設計）から。デモの後 |
| [WS084](ws084/ws.md) | MG006 | i915 の firmware の画面の引き継ぎ（素の実機の UEFI の起動で GOP が点けた pipe を N1 で止めて driver のものにし、デスクトップを出す）（2026-09-29 ユーザー、main が実装） | incomplete | p001・p002 cleared（2026-09-29、素の 5330 で takeover → LCD の Keiland、24.5 present/s）。残り: parity との乖離 1〜3 の整理 |
| [WS085](ws085/ws.md) | MG006 | Windows版WINQ-EMUのVenusでデスクトップを表示する（2026-09-29 ユーザー） | incomplete | p001 実行中。mapped blob scanoutでデスクトップを表示。SDL→仮想USB HIDタッチを実装しQMP 2指注入でメニューを確認。Files起動停止の共有画像通信を修正し開閉・再起動とTerminal同時起動を確認。物理タッチと表示所有者切替は未試験 |
| [WS086](ws086/ws.md) | MG002 | ls の出力を GNU ls と同じにする（端末なら既定で列、端末の幅）（2026-09-29 ユーザー） | completed | 2026-09-29 完了（端末で GNU と同じ列、GNU ls 9.7 と host 3362 件・guest 180 件で差 0、名前は常に UTF-8）。残りは F-055 |
| [WS087](ws087/ws.md) | MG002 | /bin/sh の対話の行編集: 矢印キーの履歴（BUG-103）と Tab の補完（2026-09-29 ユーザー） | completed | 2026-09-29 完了（QEMU・host）: 履歴の file（~/.sh_history）、矢印の履歴の上限、PS/2 の E0 の key の capability（矢印が届かなかった原因）、prompt の ~、Tab の補完（GNU Readline の名前で libedit に）。実機の確認と BUG-103 の resolved は実機の後 |
| [WS088](ws088/ws.md) | MG006 | Windows で動く Kei-nightly.zip を CI で配布する（元の zip を clang の cache と同じ Release `rev-0` に置いて再利用し、CI が hdd-image.img を入れる）（2026-09-29 ユーザー） | incomplete | 2026-09-29: p001 は draft（整理した元の zip 56 MB、LICENSES・THIRD-PARTY、DLL は MSYS2 と一致）。fork の commit と zip の中身の確認がユーザー待ち。次: p003 の準備 |
| [WS089](ws089/ws.md) | MG006 | 設定のアプリ（Settings: 左に項目の pane、右に設定、浮いたすりガラスの pane）（2026-09-29 ユーザー、デモの優先事項） | incomplete | 2026-09-29: p001〜p009 cleared（QEMU。23 頁、検索、Home の状態、desktop の設定の file、Appearance・Wallpaper（生成の壁紙 5 枚）・Display・Storage・Mouse・Keyboard・Sound、規約と回帰とデモの通し）。受け入れ 1〜6 は QEMU で満たす、実機は未実施。ユーザーの指示でブラッシュアップは後回し（ws.md の「後回しの候補」）。完了の処理（試験の plan/tools/settings への移動）は再開の時に。その後 WS090 の p007（Settings の libkeiui への移行）が始められる |
| [WS090](ws090/ws.md) | MG006 | widget・control の共有 library（少なくとも慣性の smooth scroll、独自の部品）（2026-09-29 ユーザー） | incomplete | 2026-09-30: p001〜p006・p013・p008（PDF Viewer・Image Viewer）・p011（Terminal・Notes の窓）・p014（file chooser を親の title bar にぶら下がる sheet、不透明）cleared。KUI_VERSION 11・KEILAND_VERSION 20。統合の試験（demo-s8-s9.sh、QEMU）PASS。残り: p015（案、scroll を kui_scroll へ）・p009・p010（Files）・p007（Settings）、Terminal の p088 の切り分け。ラップアップ（2026-09-30 夕） |
| [WS091](ws091/ws.md) | MG006 | 画像 viewer（2026-09-29 ユーザー） | completed | 2026-09-29 完了（QEMU）: `/bin/imageview`、PNG・JPEG（EXIF の向き）・GIF（動く）、fit・拡大・pan・pinch・慣性、前後の画像、全画面。Files からの起動は WS093、実機の確認は残り |
| [WS092](ws092/ws.md) | MG006 | text editor（simple）（2026-09-29 ユーザー） | completed | 2026-09-29 完了（QEMU）: `/bin/textedit`、共有の file chooser（libkeiland、KEILAND_VERSION 12）、touch・PRIMARY・clipboard・Files からの起動。実機は未実施。touch の drag での選択は無し |
| [WS093](ws093/ws.md) | MG006 | Files から app の起動（画像・text の double click、file の種類と app の対応）（2026-09-29 ユーザー） | completed | 2026-09-29 完了（QEMU）: Files の double click・Enter・double tap で png・jpeg・gif → Image Viewer、text 系 → Text Editor、html → Browser、pdf → PDF Viewer。Always Open With と Use System Default（`~/.config/keiland/open-with`）。実機は未実施 |
| [WS094](ws094/ws.md) | MG006 | desktop の file の icon（`~/Desktop`）（2026-09-29 ユーザー） | incomplete | 2026-09-30: L1・L2 済み。L3: p008（計測）cleared、p009 uncleared（QEMU: ready 1954 ms・選択の frame 90 ms、残りは zdesktop の import（WS099 p016）と Venus の 10 ms（F-064）、合否は実機で）、p013 cleared（jpg・gif の thumbnail、共有の decoder `userland/desktop/picture/`、EXIF の向き）。溢れた icon は「今のまま」（ユーザー） p010（長い名前を 2 行・中を省く、画面の大きさの変更で保存の場所を保つ）は実装と新しい試験 PASS、C9・boot・既存の guest の手順が未実施のため uncleared（ラップアップ、2026-09-30 夕）。 |
| [WS095](ws095/ws.md) | MG006 | IME（Wayland の input-method-v2・text-input-v3、単一の IME・複数言語、まず日本語、REmacs の辞書）（2026-09-29 ユーザー） | incomplete | p001〜p004 cleared 2026-09-29（設計、日本語の engine、辞書の package、zdesktop の仲介と IME の program: QEMU で Alt+Space → kanji → 漢字 → 確定）。p005（候補の窓と indicator）は書きかけで uncleared（`plan/ws095/p005-wip.patch`）。ユーザーの指示でブラッシュアップ（p005 の残り・p012 の辞書の拡張・p006〜p011）は後回し。既定の image には未登録 |
| [WS096](ws096/ws.md) | MG002 | Qt6（core・gui・widgets）の互換の書き下ろし（API の interface だけ、zlib）（2026-09-29 ユーザー、デモの後） | planning | デモの後 |
| [WS097](ws097/ws.md) | MG002 | GTK4 の互換の書き下ろし（API の interface だけ、zlib）（2026-09-29 ユーザー、デモの後） | planning | デモの後 |
| [WS098](ws098/ws.md) | MG006 | IME の変換のニューラル化: 辞書で候補を作り、小型のモデル（15 MB 未満）で同音異義語の選択（語の番号の並び）とひらがな列の形態素解析（語の境界と品詞、BiLSTM か小型の Attention）を評価する（2026-09-29 夜 ユーザー、IME の最後の仕上げ） | planning | WS095 の基本の辞書の後。学習の corpus と license はユーザーの判断 |
| [WS099](ws099/ws.md) | MG006 | Keiland の compositor（zdesktop）のデモの基準: 窓の操作・App Home・Wiseview・全画面と最大化の解除・greeter から Log Out と Shut Down・すりガラスの上の文字の contrast・回帰の試験の全通過（2026-09-30 ユーザー、WS035 の後継。基準は ws.md） | incomplete | 2026-09-30: L1 がそろった。L2: p011（BUG-121、5330 の角の drag 20 回で消失 6 → 0）、p015（全画面を常に合成、下からの swipe で窓に戻す）、p016（import の待ちを無くす）、p002（C5: 5330 の実機で App Home・Wiseview の開閉の最初の frame 34 回とも ≤ 100 ms、最大 55 ms）cleared。L2 の残りは C1 の実機の目視（ユーザー）。L3 は C6（WS075）・C10 の実機の 1 時間 |
| [WS100](ws100/ws.md) | MG006 | system bar の音量: 右上の通知領域の音量の icon、音量の slider と mute、変えたときの確かめの音（2026-09-30 ユーザー。動画の再生はデモの後） | incomplete | 2026-09-30: L1 がそろった（A1〜A6、Settings の Sound の頁）。L2 は 5330 の実機（A7、ユーザー）。L3 の p008 cleared: 確かめの音の遅れは QEMU の guest の中で中央値 31〜37 ms（≤ 50 ms、合否は実機で）。kernel の fragment を小さくする直しは実機で 50 ms を超えたとき |
| [WS101](ws101/ws.md) | MG006 | GPU の compute: i915 の Vulkan の compute（dispatch・shared memory・barrier・atomic）、libglesv2 の GLES 3.1 の compute、Noct の自動並列化（accel_opengles）が 5330 の GPU で動く（2026-09-30 ユーザー、10/17 のデモまで、最優先ではない） | incomplete | 2026-09-30: L1 で S13 が 5330 で通った。L2: p016（時間の分解）cleared、p017（buffer の使い回しと copy の削減、QEMU の GPU の call 835 → 212 ms、CPU の 10.6 倍遅い）uncleared。ユーザーの判断「今のまま」で S13 は今の見本、最適化はここで止める（5330 の p017 の値は P1 が追記） |
| [WS102](ws102/ws.md) | MG006 | スクリーンキーボード: 右下の角の swipe で右側に flick の panel（英字・記号・日本語）、左下の角の swipe で下側に QWERTY と手書き（認識は stub）。compositor に直接（2026-09-30 ユーザー） | incomplete | 2026-09-30: L1 を満たした。L2: p006・p007・p008・p009・p015・p016（右の列の道具の面）・p017・p018・p020・p021・p023・p024（履歴の tab。受け入れの手順だけ PASS、全手順の回帰・C9・boot test は未実施）、L3 の p019（色付きの絵文字）cleared（QEMU）。ユーザーの指示で優先を下げてラップアップ（2026-09-30 夕）。保留: p022（絵文字の tab）・BUG-125・p010・p011（速さ）・p012（IME、人間） |
| [WS103](ws103/ws.md) | MG006 | compositor を libvulkan だけにする（GPU の UAPI の直の ioctl を無くす）（2026-09-30 ユーザー「規則にして今移す」、規則は Guardrail） | completed | 2026-10-01 完了（p001〜p007、q508〜q514）: V1〜V4 を満たす（QEMU の Venus と 5330 の passthrough、単独の実機の起動は未実施）。Linux・FreeBSD の backend は F-065。試験は plan/tools/gpu-boundary |
| [WS104](ws104/ws.md) | MG006 | Keiland の OS の境界の整理: desktop の公開の header を `userland/desktop/keiland/` へ、libkeiland と compositor の OS の部分を `zedbsd/` の module へ、install の path を macro に。zedBSD の振る舞いは変えない（2026-10-01 ユーザー「Linux移植を進めます」、WS105 の準備） | completed | q515〜q522 / A1〜A6 verified。全文規約と全必須回帰 PASS、Linux は WS105 へ |
| [WS105](ws105/ws.md) | MG006 | Keiland を Linux で動かす（`/opt/keiland`）: `make keiland-linux`、libvulkan-compat（独自の WSI から system の libvulkan へ chain）、compositor の Linux の module（KMS・evdev・linux-dmabuf・logind）、主な app、gdm、wpa_supplicant・ALSA（2026-10-01 ユーザー、F-065 の Linux の分） | completed | L1〜L9/最終source conformance verified、q538 finished。Linux host/ownDebian13guest・zedBSD回帰、BUG-125/127は未修正trackingのユーザー許可。GitHub publication pending、次の実装なし |
| [WS106](ws106/ws.md) | MG001 | base/desktop の test/probe/demo 30件を userland/tests/ へ移し、package/config/install と既存の動作を維持 | incomplete | q540 partial cleared、p002 uncleared（ime-probe回答待ち）、p003未実行。 |
| [WS107](ws107/ws.md) | MG006 | engine の source を libbrowser に所属させ、Wayland無し・標準Vulkan/抽象入力の component と browser shell を整備 | completed | B1〜B5 verified / q544、API v2/public Vulkan client/最終boot。GitHub deferred |
| [WS108](ws108/ws.md) | MG007 | CI で Debian13/Ubuntu26.04 の Linux Keiland .deb を別々に作成/検証/artifact保存 | completed | P1〜P5 / q549、2OS native deb＋QEMU runtime、CI/release定義。remote未実施 |
| [WS109](ws109/ws.md) | MG006 | Linux版の共通描画を利用した native FreeBSD15 Keiland、audio/network/WiFi backend | incomplete | p001 cleared/q551; actual hardware pending |

完了した WS の Phase の記録は 2026-09-24 に plan から削除した（git の履歴に残る）。

## WS の優先順位

### WS109 の実行優先（2026-10-02）

ユーザーのWS108後の実行指示をfg016と最優先へ同時反映。WS108 completedを確認、WS109の既存F1〜F5を対象とする。他WSの相対順位とWS106保留を維持。未知の前提/人間の判断を解消してからdependent Queueを選定。


### WS108 の実行優先（2026-10-02）

ユーザーがWS107 Queue終了後にWS108実行を指定。fg015と実行最優先へ同時反映、q549でP1〜P5達成。実行優先を完了の履歴として保持し、他WSの相対順とWS106保留を維持。WS108の後のQueueは自動開始しない。


### WS107 の実行優先（2026-10-02）

ユーザー「WS107を実行してください。」からfg014とWS107を今回の最優先に反映。WS106は所有回答待ちでincomplete、既存fg010と他WSの相対順位は保持。WS107 completed。後続は2026-10-02のユーザー指示によりWS108を実行する。


### WS106 の実行優先（2026-10-01）

ユーザー「WS106を実行してください。」から fg013 と WS106 を今回の優先対象として反映。
既存デモ WS の相対順は保持する。WS106完了後はその順位へ戻り、別 WS を自動実行しない。


依存による実行順とは別のもの。Queue の権限は変えない。2026-09-28 に整理（それ以前の順は git の履歴にある）。

**優先順位の変更（2026-09-30 夜 ユーザー）**:「ws035は完了なので、書き直しが必要です。ブラウザはある程度動くようになっており、最優先ではなく、デモcriticalな中では中程度です。
最優先はws101に変更します。ws099,ws079,ws090,ws089,ws094,ws100,ws078,ws102をその次に優先します。」→ 下の番号の一覧を書き直した。WS101 を最優先にし、
WS074 はデモ critical の中位。WS035（完了）の名指しは後継の WS099 に。
直後の訂正（同日夜 ユーザー）:「あれ、指示を間違えましたみたいです。Keilandのioctlを除去してlibvulkanのみ使い、どうしても残るところはマクロブロックにする、というWSを最優先にします。」
→ 最優先は WS101 ではなく **WS103**。WS101 は元の「最適化の優先を下げる」「今のまま」に戻す（デモ critical の残りの最後）。

**デモまでの期間（2026-09-29 ユーザー）**: 10-10 ごろまで新規実装を進め、10-10 ごろ〜10-17 は bug の修正と実機での調整だけにする。

**優先の調整（2026-09-29 夜 ユーザー）**:「IMEはとりあえず変換できるようになったら、ブラッシュアップは後回しにして、Keilandを優先しましょう。Bug-030も優先です。Settingsはある程度動いたらブラッシュアップは後回しにします。」→ WS035（Keiland）と BUG-030（WS073）を先に。WS095 は p004（変換と確定）で一区切りにし、候補の窓の残り・辞書の拡張（p012）ほかは後回し。WS089 は p006 の後のブラッシュアップを後回し。

**WS101 の最適化の優先を下げる（2026-09-30 ユーザー）**:「WS101ですが、GPUでは疎通できたので、最適化の優先度を下げます。優先リストにはあるものの、その中では優先度が低いことにしてスケジューリングしてください。」→ WS101 は p015（S13 を demo の image で通す）までを済ませ、L2（p016 の内訳・p017 の 3 倍）以降はデモ critical の中で一番低い優先にする。

**描画の高速化の優先を下げ、機能の実装に（2026-09-30 午後 ユーザー）**:「では、いったん描画の高速化はラップアップして、優先度が高いWSやPhaseの中では優先順位を下げ、機能性の実装にフォーカスしましょう。」→ WS075 の L3（p031 は中断、C6 は L2 の 67.3 ms で止める）、WS102 の L3 の速さ（p010・p011）、WS081 の L3、WS079・WS099 の速さの残りは、デモの critical の中で後ろに回す。空いた枠は、ユーザーの選択で WS090（libkeiui への移行: p008 PDF Viewer・Image Viewer → p011 Terminal・Notes → p009・p010 Files）。WS102 の機能（p007・p009・p017 → p016・p018・p019）は続ける。

**週間の使用量の上限に備えたラップアップ（2026-09-30 夕 ユーザー）**:「A1は残りが小なので、やりきってラップアップ。A2、A3はデモでの優先順位を下げ、きりのいいところでコミットしてラップアップ。A4は残り作業量を教えてください。」→ P4 は ws090-p014 をやりきって終了。P3（ws102-p024、以後の p022・BUG-125 は保留）と P6（ws094-p010）はきりのよい所で止めて終了。P7（ws090-p011）は残りの作業量をユーザーに示して判断を待つ。WS103 は始める時期をユーザーが指示する。

**エージェントの呼称（2026-09-30 ユーザー）**: メインのエージェントは **Q1**（クイーン 1）、サブエージェントは **P1**・**P2**…（ポーン）。2026-09-30 の割り当て: P1 = WS075（i915、High。p027〜p030、BUG-123）、P2 = WS101（High、p015 で終了）、P3 = WS102（スクリーンキーボード、Mid）、P4 = WS079 L2 → WS081 → WS094 → WS099 p016 → WS101 → WS100 → WS102 p009・p019（Mid）、P5 = WS099 の BUG-121（Mid、P2 の後継、p011 で終了）、P6 = WS099 の p015 → p002 → WS102 の p015・p017・p018・p023 → WS094 の p010（Mid、P5 の後継）、P7 = WS090 の libkeiui への移行（Mid、worktree `ws090-kui`、2026-09-30 午後）。P1 は 2026-09-30 午後に描画の高速化とともに終了。新しいサブエージェントは次の番号から。

**広く浅く進める（2026-09-30 朝 ユーザー）**:「デモcriticalなWSについては、幅広く設計を進めて、広く浅く実装をすすめて、ブラッシュアップも段階的に具体的な数値目標を設定しておくことでphaseをたくさんわけておき、広く浅く全体を改善していくことにしたいです。Claudeのトークンになるべく多く課金して実装を進めたいですが、途中で資金が尽きてもデモ全体への影響を小さくしたいからです。」→ デモ critical の WS は、(1) 設計を全体に先に広げる、(2) 各 WS を「まず動く」の段まで浅く実装してそろえる、(3) 磨き込みは段（L1・L2・L3…）ごとに具体的な数値目標を持つ小さな Phase に分け、全 WS の同じ段をそろえてから次の段へ進む。1 つの WS を深く掘り続けない。どこで止まっても、デモの全場面がその時点の段で動いている状態を保つ。

**デモの touch（2026-09-30 朝 ユーザー）**:「タッチはWindows上のQEMUでやります。外付けタッチLCDは間に合えば別途計画を立てます。」→ touch の場面（WS081・WS102・WS090 の文字の編集の touch・WS094 の touch）は、Windows の host の QEMU（WS085 の Venus、WINQ-EMU）の上で見せる。5330 の実機は mouse と keyboard。外付けの touch LCD は間に合えば別の計画。

**一時的に人間が作業（2026-09-30 ユーザー）**:「IMEとブラウザの作業は一時的に、人間が作業するので、作業しないでください。でも一時的です。」→ 当初は WS095（IME）と WS074（ブラウザ）をエージェントに割り当てない判断だった。WS074 は同日のユーザー指示「Run ws074」で解除・再開。WS095 とその source の保護はユーザーが戻すと言うまで継続。

**運用（ユーザー、2026-09-27〜30）**: 作業用のサブエージェントを N=0〜9（2026-09-29 は N=9、1 時間に 5 時間の枠の約 25%）。5 時間の枠を 1 周期とし、枠の終わりに N を減らし、
N=0 になったら実装をまとめて計画（master・ws.md・Future Work・Bug Board）を整理する。試験は amd64 だけ、Phase の終わりに。effort は判断の表の「サブエージェントの effort」。

2026-10-01: 最優先のWS104→WS105/fg012を完了。根拠はWS105 L1〜L9とq538全文規約・回帰。元のユーザー指示と完了履歴を保存し、未完了WSの相対順は変えない。以下は候補の優先順で、実行許可ではない。
1. **デモ critical の上位**（この順、2026-09-30 夜 ユーザー）: WS099（Keiland の compositor、WS035 の後継）、WS079（Notes・PDF Viewer）、WS090（libkeiui）、
   WS089（Settings）、WS094（デスクトップの icon）、WS100（音量）、WS078（Kei への改名）、WS102（スクリーンキーボード）。
2. **デモ critical の中位**: WS074（ブラウザ。ある程度動く。p100 Acid3 が次の候補）。
3. **デモ critical の残り**（ユーザーの順位の指定は無く、Q1 が中位の後に置いた）: WS084（i915 の画面の引き継ぎ）、WS075（i915。描画の高速化は止めたまま）、
   WS081（touch の質。L3 は後ろ）、WS085（Windows の QEMU の Venus。デモの touch の土台）、WS068（GL 3.2 まで。3.3 以降は保留）、
   WS101（GPU の compute。S13 は通った。最適化は「今のまま」で止め、デモ critical の中で一番低い）。
4. **WS080（ld.coff）**: デモ critical の後に loader（p001〜p008）を早めに仕上げる。互換の DLL（kernel32 以降）は下位のモデルの subagent に継続して実装させる。
5. **bug**: WS073（BUG-030・BUG-039・BUG-031・BUG-107、BUG-093 は toolchain の許可待ち）。BUG-027・033 は低い優先度（計測して閉じる）。
6. **ACPI（WS049〜WS052）と Arm64（WS044・WS048）**: デスクトップが片付くか limit が余るとき。
7. **WS001** はユーザーが指示したときだけ。WS077（PC-98 の PCI）・WS066（ld.so の最適化）は低い優先度。
8. **時期がユーザー次第**: WS095（人間が作業中）、WS098（WS095 の後）。

上に無い未完了の WS（WS004・005・007・009・014・017・026〜029・031・033・034・045〜047・061・082・083・088・096・097、予約の WS037〜039、保留の WS013・015）は順位を定めていない。

### 新しいレビュー対象（2026-10-01）

[WS106](ws106/ws.md)・[WS107](ws107/ws.md)・[WS108](ws108/ws.md)・[WS109](ws109/ws.md)を未順位の計画候補として追加。
既存 fg010 と WS の相対順位を保持。レビューの列挙順を実行優先順位/Queue 許可に読み替えない。
source所有/Wayland禁止の新規則は WS074/WS107 に適用。WS074 p100→p101 を保持し、移行と同じ source/runner を並行編集しない。
[出典・決定・未決](reviews/2026-10-01-review.md)。新 target と package manifest は p001 の設計で具体化する。

## Upcoming Work Outlook

見込みであって、約束や実行許可ではない（2026-09-30 に整理）。

| 候補 | 理由 | 準備 |
| --- | --- | --- |
| 実機の image `build/demo-lcd8` の確認（LCD の takeover、10 app の軽さ、Files → Image Viewer・Text Editor、Settings、デスクトップの icon、Terminal の角、Notes の全画面を解く、USB マウス・sh の履歴・プロンプトの `~`） | デモ | ユーザーが試験 |
| WS075 p024: C6 の物差し、flag の再利用、host の試験の既存の失敗（p023 は 1 run -28% で cleared） | 窓が多いときの軽さ | 実行中 |
| fg010 の台本の確定（ユーザー）→ WS099（compositor の基準）・WS100（音量）の p001 | デモ | 台本と基準の案はユーザーの確認待ち |
| WS073: BUG-030 の受け入れの残り（KVM 2×20・boot test）、BUG-116（EP0 の event の取りこぼし、BUG-036 と同じ系統か） | 安定性 | phase041 |
| WS094 p004 の残り（保存した場所への配置の guest の確認・回帰・boot test）→ p005〜p007 | デスクトップの icon | phase004 の Resume point |
| WS090 p004（窓の土台と Text Editor の libkeiui への移行、文字の編集の touch） | 共通の部品 | p003 cleared |
| WS074 p100（Acid3 100/100） | p099 cleared 後の固定順 | Queue 未選択、承認待ち |
| WS095（IME）p005〜: **一時的に人間が作業中** | — | ユーザーが戻すと言うまで |
| WS098（IME のニューラル化）: 学習の corpus と license の判断から | IME の最後の仕上げ | WS095 の辞書の後 |
| WS080: p001（ld.coff の設計） | デモ critical の後 | spec.md |
| WS106 p002 → p003 | 残るime-probe移動 → 全文規約・最終build/boot | p001 cleared、29件＋13files移動済み。ime-probe非競合回答が再開条件、次Queue未選定 |
| WS107 | libbrowser/source所有・品質 | completed、q544/B1〜B5 verified |
| WS108 | 2 distro native .deb/QEMU/CI release | completed / q549、remote未実行 |
| WS109 p002 L1 | native FreeBSD15 library/build foundation | q551 p001 cleared、native ABI/environment verified。実backend統合と実機関門は後段 |

## Tools

回帰と観察の道具は `plan/tools/` に置く。完了した WS の試験は、ここへ移したもの以外を削除した。Phase に固有の試験は各 WS の `tests/` にある。

| tool | 用途 | 使い方 |
| --- | --- | --- |
| `plan/tools/toolchain-lock.sh` | 共有の toolchain の tree（`build/llvm`・`llvm-source`・`llvm-build`・`NoctLang`）の directory を読み取り専用にして、許可の無い変更を防ぐ（BUG-096） | `lock`・`unlock`（main が許可した toolchain の変更の間だけ）・`status` |
| [boot-test.sh](tools/boot-test.sh)（`boot-test.py`） | 起動の確認。OVMF の USB（amd64）か BIOS の IDE（i386）で起動し、画面を QMP で撮って login prompt を読む | `plan/tools/boot-test.sh [IMAGE]`。`OUTPUT`（既定 `build/boot-test`）、`BOOT_TIMEOUT`、`BOOT_MODE=uefi-usb` か `bios-ide` |
| [Keiland の OS 境界 checker](tools/keiland-os-boundary/check.sh)（WS104） | 共通 source の OS include / ioctl、GPU layout の所有、install literal、libc に残る desktop header を C1〜C5、Linux/zedBSD moduleと実build membershipをL1〜L5で確認。evdev の 1 行だけを例外とする | `sh plan/tools/keiland-os-boundary/check.sh`。PASS は exit 0、違反は各項目の file:line と exit 1 |
| [Linux の試験 guest と操作の道具](tools/keiland-linux/README.md)（WS105） | Debian 13 の image / overlay・loopback SSH・QMP screenshot / 入力・install・PNG の画素。host の画面を使わない | `build-guest.sh` / `guest.sh` / `install-guest.sh` / `png-probe.py`、build/ELF/header/source の checks、Vulkan chain/interpose と `wsi-check.sh`（90 frame ×4）。README の timeout 付き command |
| [Keiland の zedBSD の検証手順](tools/keiland-linux/zedbsd-commands.md)（WS104 から移した） | build / warning・sysroot・boot・C1/C2/C9・GPU・glass / pen・Settings / 音量の既存回帰。image build は直列、BUILD と OUTPUT を個別指定 | 各節 §0〜§9 |
| Dell Latitude 5330 の実機の操作とデモの image（[tools/hw5330](tools/hw5330/README.md)、2026-10-01） | 実機の構成（5330 自身が host の passthrough、ssh `solaris10-man`）、`/tmp/i915-hw.lock`、画面・入力・結果の読み戻し、USB の単独の起動（ユーザー）、`build-demo-image.sh` と boot の行の落とし穴、実機の試験の script の一覧と PASS の印、よくある失敗。デモの優先 WS の作業の手引きは各 `plan/wsNNN/guide.md` | README.md |
| GPU の境界の試験（[tools/gpu-boundary](tools/gpu-boundary/)、WS103 から移した） | compositor が GPU を Vulkan だけで扱うことの確かめ: `v1-check.sh`（GPU の UAPI の include と ioctl が `gpu-zedbsd.c` の外に無い、GPU の UAPI の header を `#error` にして compositor が compile できる、`/dev/gpu`・`--gpu` が無い）、host の `run-dedicated-host.sh`（libvulkan の dedicated の import の照合）・`run-gpu-zedbsd-host.sh`（compositor の wire の値の確かめ）、guest の `forge-guest.sh`（偽の buffer を断る、`/bin/gpu-forge-test`）・`fence-guest.sh`（Wayland の present ごとの新しい fence、`--log-frames` の `ZWL ACQUIRE_FENCE`）。guest の image は `build-forge-image.sh`（`config-amd64-forge.mk`: 基準の image に gpu-forge-test・wltest・acquire-fence-test） | 各 script の先頭の使い方 |
| [pc98-boot.py](tools/pc98-boot.py) | pc98 の起動の確認（`boot-test.sh` に PC-98 の mode が無いため）。PC-98 fork の QEMU で起動し、text VRAM で login prompt を読み、root で login して `uname -a`。画面を text と PNG で残す。WS053 から移した | `pc98-boot.py ~/qemu-pc98/build/qemu-system-i386 IMAGE OUTPUT`（`clock/pc98-sleep.py` の `Guest` を使う） |
| [guest/guest.sh](tools/guest/guest.sh) | SSH による guest の操作（USB CDC-ECM、KVM）。コマンドの実行・file の送受・lldb・kgdb・画面 | `start IMAGE`・`wait`・`run CMD`・`put`・`get`・`lldb`・`kgdb`・`screenshot`・`stop`。image は `extra-files` の出力を eval して作る。`GUEST_RUNTIME=<dir>` で別の guest を並べて動かせる（既定 `build/guest`） |
| [guest/serial.py](tools/guest/serial.py) | シリアルの console と対話する（sshd が上がる前。`CONFIG_PCAT_SERIAL_MIRROR=y`） | `serial.py --socket S run 'CMD'`（終了状態を返す）、`login` |
| [qmp.py](tools/qmp.py) | QMP の command を送る | `qmp.py SOCKET quit` など |
| [latency/](tools/latency/) | interactive の応答の測定（起床の遅れ、端末の echo）。WS041 から移した | `run-echo-qemu.sh`、`run-wakebench-qemu.sh`、`pc98-wakebench.py`、`config-*-bench.mk` |
| [clock/](tools/clock/) | guest の時計の進み（`sleep 5` の実時間）。WS040 から移した | `clock-check.py`、`pc98-sleep.py` |
| [ufs/](tools/ufs/) | UFS の directory の試験と volume の検査。`dir-grow.sh` は mount した volume で directory を 12 block まで育て（作成・削除・rename・rmdir・上限）、`verify` で確かめる（`LONG`・`SHORT`・`MOVE`・`GONE` で数）。`check-volume.py` は guest が書いた volume を host で fsck 相当に検査する。`crash-test.sh` は journal の volume の成長の途中で QEMU を止めて replay を確かめる。WS054 から移した | `sh dir-grow.sh DIR make\|verify`（guest）、`check-volume.py IMAGE`、`crash-test.sh IMAGE SECONDS...`（host）。作業の volume は `zedimage-host ufs SIZE EMPTYDIR IMAGE --inodes=16384 [--profile=journal-snapshot]` で作り、NVMe（`-device nvme`）でつなぐ |
| UFS の journal の試験（[tools/ufs](tools/ufs/)、WS063 から移した） | `crash-test.sh`（既定は v3・NVMe の作業 volume、`PROFILE=journal-snapshot` で v2）、`journal-func.sh`＋`journal-guest.sh`（guest での journal の機能: 隠しの `.ufs-journal`、最初の mount での作成、`nojournal`・`writethru`）、`root-crash.sh`（root の強制終了と replay）、`zedimage-compare.sh`（2 つの zedimage-host の UFS の出力の byte 比較） | 各 script の先頭の使い方。`GUEST_RUNTIME`・`VOLUME` を上書きできる |
| SSH の guest image（[guest/](tools/guest/)、WS063 から） | clang の無い SSH の guest image: `config-amd64-ssh.mk`、`build-ssh-image.sh`（package が build/amd64/dynamic に link するので build/amd64 に作る） | `plan/tools/guest/build-ssh-image.sh` |
| 組み合わせの guest image と process の試験（WS064 から） | `guest/hybrid-image.sh BUILD OUT [BASE]`（full の guest image にこの tree の vmunix・BOOTX64.EFI・libc.so・make・sh を入れる）、`guest/make-cases.sh IMAGE`（guest の make-diff）、`process/vfork-test.c`＋`guest-vfork.sh IMAGE`（fork の COW、vfork、posix_spawn、並行の fork） | 各 script の先頭 |
| NVMe と lease の試験（WS072 から） | `nvme/timeout-retry.sh`（QMP の block_set_io_throttle で 2 台目の NVMe を絞り、timeout の後の再発行を確かめる）、`ufs/format-lease-probe.c`（format の lease の下の fsync） | 各 file の先頭 |
| toolchain の試験（WS055 から） | `toolchain/link-undefined-version.sh CLANG`（version script の未定義の symbol の link）、`toolchain/zlib-shared-configure.sh CLANG SYSROOT`（zlib の configure が共有 library を作れること） | host で実行 |
| rpi4 と amd64 の serial の guest（WS036 から） | `guest/rpi4-serial.sh`（raspi4b の guest に serial で login して command を実行、`APPEND` で /chosen/bootargs）、`guest/amd64-serial.sh`（amd64 の UEFI・NVMe・KVM、image に `CONFIG_PCAT_SERIAL_MIRROR=y`）、`rpi4/bootargs-rpi4.sh`（rpi4 の boot の parameter の試験）、`rpi4/noct-rpi4.sh`（rpi4 の Noct の JIT と API） | 各 script の先頭 |
| File Manager の試験（[tools/files](tools/files/)、WS071 から移した） | lean な Venus の guest image（`build-files-image.sh`・`config-amd64-files.mk`）と guest（`files-guest.sh`、runtime `build/ws071-run`、`build/ws035-sq-venus` の renderer が要る）。`files-regress.sh [OUTDIR] [PHASE...]`（zdesktop-files の guest 試験 14 本）、`files-p011.sh`（App Home）、`files-p018.sh`（configure_bounds と置き場所）、`files-lag.sh`。host の files-render（`host-build.sh`・`host-run.sh`・`host-p009/p010/p013/p014.sh`）、`host-png.sh`（libz-compat・libpng-compat を Python と比べる）。`make-home.sh`・`qmp-input.py`。`host-build.sh` は libkeiland の gesture.c・scroll.c・motion.c も build する（ws093-p003）、`host-model.sh`（files-model を一時 folder で）、`host-default.sh`（Always Open With の利用者の一覧、WS093 から）、guest の `files-open.sh OUTDIR mouse|always|info|touch`（file の種類ごとの起動、WS093 から） | 各 script の先頭の使い方 |
| System Menu と Titlebar の試験（[tools/titlebar](tools/titlebar/)、WS070 から移した） | lean な guest image（`build-menu-image.sh`・`config-amd64-menu.mk`、probe 入り。WS035・WS071・WS074 の image の元）と guest（`menu-guest.sh`、runtime `build/ws070-run`）。`menu-p002.sh`（protocol の error）、`menu-p003.sh`（terminal の menu）、`menu-occlude.sh`、`menu-regress.sh OUTDIR TEST...`（WS035 の zdesktop の試験）、`titlebar-p008/p009/p010/p011/p013.sh`（model、glyph、CONTROLS、TABS、tab の key と wheel）、`icons-host.c`、`style-compare.sh REV FILE...`、`menu-hw.sh`（i915 実機、`flock /tmp/i915-hw.lock` の下で） | 各 script の先頭。files の guest で走らせるときは `GUEST_RUNTIME=build/ws071-run` |
| i915 の実機の試験の場面（WS075） | `plan/ws075/tests/test-hw.sh`（`flock /tmp/i915-hw.lock` の下で試験の場面（vke1・vke2・vkx・vkc ほか）を走らせ、共有の /tmp から log を写す）、`capture-hw.sh`（ZDESKTOP_APP ごとの build の directory で zdesktop の capture）、`config-test-hw.mk`（zdesktop の実機の config と serial の mirror）、`shader-survey/run.sh`（host で 122 の module を i915 の compiler の不足と照合）、`vk-calls.py`（client の Vulkan の command と実行器の対応）、`bug085-hw.sh IMAGE OUTDIR [SCENARIO]`（ws075-p015: vkloop-hw.sh の作った zdesktop の capture の image を gdbstub 付きで 1 回、capture の frame の止まり・fault を印にして QEMU を debugger のために残す。`bug085/`。watcher は起動の停止・APIC timer の較正の誤り（BUG-094）・session の終わりの halt の有無も印にする。`bug085/procs.py`・`kstack.py`・`ustack.py` は DWARF 無しの gdb で process・thread・kernel と user の stack を読む、`summary.py` は run ごとの 1 行）。注意: i915 の試験の build の kernel は 16 MiB の上限（AMD64_KERNEL_MAX_BYTES、.bss を含む）の近く。2026-09-28 に 28 KiB 超えたので vkx の場面の state（約 240 KiB）を heap へ移した | 各 script の先頭 |
| HDMI の主出力の試験とデモの image（WS075 p011〜p013） | `plan/ws075/tests/hdmi-h1-hw.sh SCENARIO OUTDIR [FLAGS]`（i915 の試験の場面を lock の下で走らせ、5330 の host の USB を 1 秒ごとに記録、新しい device の HID の report descriptor を取る）、`hdmi-h2-hw.sh OUTDIR "BOOT LINES" [秒...]`（`ZEDBSD_BOOT_EXTRA_LINES` の zdesktop の image を実機で起動し、resident の scanout の buffer を QMP の memsave で PNG に）、`hdmi/host-output-test.sh`（`display=`・`display.mode=` と EDID・CVT・mode の選択の host 試験）、`hdmi-h4-hw.sh`（start・ctl・fetch・stop）（ws075-p016: `h4-ctl.py watch SECONDS MS` は pipe B の TRANSCONF・PLANE_CTL・PLANE_SURFLIVE を 20 ms ごとに標本化、`hdmi/h4-blank.py` はその暗・黒の区間、`hdmi/h4-cycle.sh OUTDIR COUNT` は logout・login の繰り返しと撮影。`shot` の live は PLANE_SURFLIVE から）（実機を段ごとに: lock、QEMU、std VGA の splash の連写、resident の buffer の画面、QMP の pointer・key・drag の周期負荷、guest の disk の log。`hdmi/h4-*.{sh,py}`）。デモの image: `plan/ws075/demo/build-demo-image.sh [BUILD] [passthrough]`（graphical boot + `display=hdmi` + App Home。i915 の node は sessiond が `hw.gpu.attaching` の間だけ待つ（ws035-p113、`greeter_gpu` の回避は削除）） | 各 script の先頭。`plan/ws075/tests/hdmi/h4-ctl.py latency PIPE COUNT`（pointer の移動から次の flip まで、ws084）・`rate PIPE SECONDS`（入力が続く間の flip の率、ws075-p008） |
| ls の GNU との比較（[tools/ls](tools/ls/)、WS086 から移した） | `compare-gnu.py OUR_LS [--gnu /bin/ls]`（host で GNU ls と byte 単位、端末の幅と pipe、locale、環境変数）、`guest-compare.py`（guest の ls を `ssh -tt` と pipe で GNU と比べる）、`tty-run.py COLUMNS CMD`（指定の幅の擬似端末で byte のまま取る） | `python3 plan/tools/ls/compare-gnu.py build/.../ls` |
| sh の対話の試験（[tools/sh](tools/sh/)、WS087 から移した） | host の pty で sh の行編集を試す: `history-host.py`（履歴の file と矢印）、`complete-host.py`（Tab の補完）、`prompt-host.py`（prompt の ~）、`pty-keys.py`（ssh -tt で guest の sh に key を送る）。既存の `vi-host.py`・`sh-interactive.py` と同じ場所 | `python3 plan/tools/sh/complete-host.py` |
| 画像 viewer の試験（[tools/imageview](tools/imageview/)、WS091 から移した） | `run-host.sh`（host: 復号の画素を PIL と比べる、folder の順、view の計算）、`imageview-guest.sh OUTDIR STEP...`（Venus の guest、`make-images.py` の画像）、`touch-guest.sh`（注入の touch: pinch・flick・double tap・swipe・長押し）、`style-extra.py`（style-check が見ない規則の候補の発見的な走査） | `sh plan/tools/imageview/run-host.sh` |
| 共有の file chooser の試験（[tools/keiui](tools/keiui/)、WS092 から移し ws090-p006 で libkeiui へ） | `host-chooser.sh`: libkeiui の `kui_file_chooser_*` の model と描画・`kui_ui` を通した key・click・tap の host 試験（85 件）、絵は `build/keiui-shots/` | `sh plan/tools/keiui/host-chooser.sh` |
| Text Editor の試験（[tools/textedit](tools/textedit/)、WS092 から移した） | `host-core.sh`: 文書・undo・file・表示の行・検索・編集の host 試験（34 件）。`qmp-keys.py`: QMP で文字列・key の組・pointer を guest に送る（US の配列） | `sh plan/tools/textedit/host-core.sh` |
| i915 の実機の計測（WS075） | `plan/ws075/tests/hdmi/measure-apps.sh`（lock の下で 1 回の計測の run、WS099 の C6 の試料を含む）、`c6.py OUTDIR...`（5 run 以上をまとめた C6 の判定: pointer を動かしてから cursor が行き先に出る flip まで、中央値・p90・run の幅）、`engine-gdb.sh`（session ごとの engine の時間を gdb で読む）、`h4-ctl.py`（latency・rate・freq・c6）、`stress-117.sh`（10 app の上で Model viewer の開閉を繰り返し、描画の停止と descriptor の消失を数える）。compiler の guard の host 試験 `plan/ws075/tests/guard/run.sh`（Mesa の brw_asm・brw_disasm と byte で比べる） | 各 script の先頭の使い方 |
| GPU の compute の compiler の host 試験（WS101） | `plan/ws101/tests/host/run.sh`: compute の module の compile、scoreboard、descriptor の応答の bit、EOT、Mesa 25.0.7 の brw_disasm・brw_asm との byte の照合、拒否すべき shader、IR の interpreter での結果の照合（add・ids・atomic・length・dynamic・noct）、実行器の compute の object の試験（executor-test）、p004 の dispatch の batch の試験と genxml の照合（Mesa の genxml を使う。既定は `/home/awe/p014-c/mesa/src/intel/genxml`） | `sh plan/ws101/tests/host/run.sh` |
| GPU の compute の実機の試験（WS101） | `plan/ws101/tests/hw/run-hw.sh`: 試験の組 `I915_TEST_SET`（既定 `all` は vkcs 以外、`compute` は vkcs だけ。試験の kernel の上限 16 MiB のため）で image を lock の外で build し（lock の待ちを含む上限 1 時間）、`flock /tmp/i915-hw.lock` の下で実行器の回帰（vkx・vke1・vke2・vkc）と compute の場面 `vkcs`（ONE・ADD・ID・ODD・PUSH・ATOMIC-SSBO・MIXED・MANYOPS・SPILL）を 5330 の passthrough で順に走らせる 。`plan/ws101/tests/hw/gles-hw.sh`（と `gles/` の構成）: GLES 3.1 の compute（`glescompute`）と egltest の feedback・queries を 5330 の passthrough で、lock の外で build し lock の下で走らせ、guest の disk の log を読む | 先頭の使い方 |
| GLSL ES 3.10 の compute の試験（WS101） | `plan/ws101/tests/glsl/run.sh`: libglesv2 の GLSL の compute の compile・link・spirv-val・i915 の host の compile・brw の往復、断るべき 17 shader、host の Vulkan（lavapipe）での実行と C の計算の照合（Noct の shader の形を含む） | `sh plan/ws101/tests/glsl/run.sh` |
| GLES 3.1 の compute の試験（WS101） | `plan/ws101/tests/gles/run.sh`（host の reflect）、`build-image.sh`（image を `build/ws101-p009-img` に）、`venus.sh`（自分の runtime `build/ws101-p009-run` で guest を起こし、`/bin/glescompute` の version・limits・add・noct・shared・indirect・chain・release・repeat・errors） | 各 script の先頭の使い方 |
| Noct の GPU の見本の試験（WS101） | `plan/ws101/tests/noct/g3-venus.sh`（Venus で `mix.nct` を CPU と GPU で走らせて一致と dispatch の数を見る）、`plan/ws101/tests/hw/g3-hw.sh`（同じことを 5330 の passthrough で、`NOCT` で accel の noct を指定）。libglesv2 の `KEI_GLES_COMPUTE_TRACE` で dispatch ごとの行を出す | 各 script の先頭の使い方 |
| compositor のデモの基準の試験（WS099） | `plan/ws099/tests/criteria.sh`: 基準 C1〜C10 を QEMU の Venus で一括して確かめる（閾値は先頭の変数）。基準の image は `build-criteria-image.sh`・`config-amd64-criteria.mk` | `sh plan/ws099/tests/criteria.sh` |
| libwayland の host 試験（WS035 p075） | `plan/ws035/tests/p075/run-host.sh`（host の libwayland-server と試験の protocol で、生成された protocol の event と server の作る object、client が壊した server 側の object（zombie）への event と fd、id の再利用（p089）） | host で実行 |
| xdg-shell の popup と toplevel の試験（WS035 p076） | `plan/ws035/tests/zdesktop-p076.sh`（Venus の guest、`/bin/popup-probe`（`config-amd64-menu.mk`）で menu・submenu・flip・reposition・dismiss、toplevel の move・resize・min/max size、ping の無応答の表示を QMP で操作し画面を撮る） | 先頭の使い方。PNG は `build/ws035-p076/` |
| sub-surface と seat の試験（WS035 p077・p078） | `plan/ws035/tests/zdesktop-p077.sh`（`/bin/subsurface-probe`: 位置・sync・desync・place_above/below・破棄）、`plan/ws035/tests/zdesktop-p078.sh`（`/bin/seat-probe`: XKB keymap・repeat_info・lock の modifier・wl_output v4）、`plan/ws035/tests/p078/run-host.sh`（host の libxkbcommon で zdesktop の keymap を compile し modifier と keysym を照合） | 先頭の使い方。Venus の guest |
| POSIX の console の試験（[tools/posix](tools/posix/)、WS056 から移した） | `console-posix-r2.sh IMAGE ELF [N]`（serial mirror の kernel `config-amd64-serial.mk` の guest の console で `POSIX-R2.ELF` を N 回、`AS_SH=1` で /bin/sh としても）、`guest-sigev.sh`＋`sigev-thread-mask.c`（SIGEV_THREAD と置き換えの mask の EINTR）、`guest-spawn-probe.sh`＋`spawn-probe.c`、`console-probe.sh`、`guest-pax-test.sh`・`make-pax-archives.sh`（pax・gnu・ustar の展開の比較） | 各 script の先頭の使い方 |
| [kbench/](tools/kbench/) | kernel の microbenchmark（system call、pipe の往復、fork、exec、cached の read、anonymous と file の fault）。kernel の build（LTO・最適化）の比較に使う。WS053 から移した | `kbench/build.sh BUILD OUTPUT`（amd64 の guest 用）で作って guest で `kbench [file]`。予熱の 1 回の後に数回走らせ、中央値で比べる |
| [driver-fragments/prepare.py](tools/driver-fragments/prepare.py) | 統合した driver の source から host 試験用の断片を切り出す（出力は `build/driver-fragments`）。WS025 から移した | WS004 の AX211・xHCI と WS001 の UFS の host 試験が呼ぶ |
| [packages/](tools/packages/) | 外部 package の試験: ライセンス監査、未解決 symbol、取得機構とクロスビルドの host 試験。WS032 から移した | `audit-licenses.sh`、`check-unresolved-symbols.py`、`run-external-host-test.sh`、`run-cross-host-test.sh` |
| [menuconfig-target-host-test.py](tools/menuconfig-target-host-test.py) | menuconfig の target の選択の host 試験。WS020 から移した | `make menuconfig-host-test` |
| [boot-parameter-image-tool.c](tools/boot-parameter-image-tool.c) | image の boot parameter の読み書きと、pc98 の text VRAM の解読（`decode-pc98-vram`）。WS003 から移した | WS005・WS013 の試験が compile して使う |
| [sync.py](tools/sync.py)（[README](tools/README.md)） | GitHub との同期（GitHub mode） | `plan/tools/README.md` |
| sh の試験（[tools/sh](tools/sh/)） | `/bin/sh` を dash と比べる（oils の spec と自前の case）。guest では 40 件ずつ。対話（serial console）と行編集（host の pty） | `build-host-sh.sh`、`sh-diff.py --shell build/ws042/host-sh`（`fetch-oils.sh` で oils を取得）。guest は `--export build/ws042/guest-export` の後 `guest-batches.sh`（中で `guest-diff.sh`）。対話は `sh-interactive.py SOCKET`、行編集は `vi-host.py build/ws042/host-sh`。WS065 から: `build-guest-sh.sh`（この tree の sh を guest の image の libc.so で build）、`guest-batches.sh` の `GUEST_SH=FILE`（guest の copy の /bin/sh を置き換える）、`guest-expat.sh SH`（guest で expat の configure・make・runtests を走らせ configure の生成物の checksum を出す） |
| utility の差分試験（[tools/utils](tools/utils/)） | base の utility を GNU（POSIX mode）と比べる（`cases/` の 484 件、guest へは `--export` と `plan/tools/sh/guest-diff.sh`）。実際の configure（expat・coreutils）を GNU の道具と我々の道具で走らせて生成物を比べる。libc の浮動小数の書式を glibc と比べる | `build-host-utils.sh`、`util-diff.py`、`configure-diff.sh`、`float-format.c`。書き直しの前後の ls の比較は `ls-compare.sh OLD NEW` |
| X11 の回帰（[tools/x11](tools/x11/)） | zdesktop-x11server の上の X11 の app（Venus、`plan/ws035/tests/zdesktop-guest.sh start` の guest）: x11-p003（zterm の rootless の窓、入力、docked）、x11-p004（glxtest の GLX、docked の大きさの変化）、x11-p005（zgears 300 frame、回る、fps）。WS069 から移した | `sh plan/tools/x11/x11-p00N.sh [OUTDIR]`（`GUEST_RUNTIME` の既定は build/ws035-sq-run）。画面を目で確かめる |
| libm の試験（[tools/libm](tools/libm/)、WS076） | libc の libm（`src/libc/math/`）を MPFR（gmpy2）の参照値と比べ、関数ごとの最大・平均の ulp 誤差、正確であるべき結果の不一致、C11 Annex F の特殊な値・errno・例外を出す。host（host の clang、libm を link しない）と guest（amd64、image の libc.so、serial で実行） | `plan/tools/libm/host-test.sh [--count N] [NAME...]`、`plan/tools/libm/guest-test.sh [--count N] [NAME...]`（`BUILD` 既定 `build/ws076-amd64`）。参照の生成は `gen-reference.py OUT.bin`。ブラウザの JS（ws074 の試験と `js/libm.js`）を guest で Chromium と比べる `browser-js.sh`（`js-reference.py --reference` で期待値） |
| 規約の検査（[style-check.py](tools/style-check.py)） | `plan/coding-style.md` のうち機械的に確かめられる規則（条件の中の呼び出し、閉じ括弧の後の空行、段落の comment、入れ子の宣言、条件演算子、goto、前方宣言、comment の形、名前、複数行の本体の括弧） | `python3 plan/tools/style-check.py FILE... [--summary] [--rule NAME]` |
| Browser component（[tools/browser-component](tools/browser-component/README.md)、WS107） | Wayland無し/public headerのみの動的第2client、2view/抽象入力/callback、allocation rollback・async history、標準Vulkan/lavapipeのdraw/record/readback/caller fence/resize/target解放、plain＋ASan/UBSan | `sh plan/tools/browser-component/run.sh [plain|asan]` |
| Keiland native deb（[release driver](../tools/release/keiland-linux-deb/README.md)、WS108） | pinned Debian13/Ubuntu26.04 QEMU native build、fresh guest導入/GUI/input/public Vulkan/upgrade/remove、manifest/buildinfo/checksum | `make keiland-linux-debian` / `make keiland-linux-ubuntu2604` |

QEMU の不具合は log を読まずに、QEMU のデバッグ機能で解析する:

- **gdbstub**: `-S -gdb tcp::<port>` で止めて起動し、host の `gdb` で `target remote :<port>`。`vmunix` は strip されていない。
- **map**: link で作る `$(BUILD)/vmunix.map` で address から関数を引く（`-g` は付けない）。
- **monitor/QMP**: `info registers`・`info mem`・`info tlb`・`x/`・`xp/`・`pmemsave`。
- **trace**: `-d int,cpu_reset,guest_errors -D <file>`（例外と reset だけ）。

pc98 は QEMU の PC-98 fork（`~/qemu-pc98/build/qemu-system-i386`、`-M pc9821,pegc=off,coregraph=on`）で起動し、
`pmemsave 0xa0000 0x2000` で取り出した text VRAM を `boot-parameter-image-tool decode-pc98-vram` で読む。
回帰試験では GPU を使わず、標準 VGA の framebuffer で login prompt だけを確かめる。

guest の memory（2026-09-24 ユーザー決定「ゲストのメモリはamd64とarm64では8GBでテストしましょう」）: amd64 は 8 GiB（`plan/tools/guest/guest.py` の既定と
`boot-test.sh` の `uefi-usb`）。arm64 の QEMU raspi4b は board の model が 2 GiB しか受け付けない（`Invalid RAM size, should be 2 GiB`）ので 2 GiB（2026-09-24 ユーザー決定「raspi4bは2GBでOKです。」）。i386 は変えない。

- WS105 継続 fixture: `plan/tools/keiland-linux/dbus-wire.c` / `dbus-wire.py`（独立 wire / fd 境界、ordinary + ASan/UBSan）、`seat-fd.c`（guest の DRM master / caller fd 所有権）。[手順](tools/keiland-linux/README.md#logind-の-fd-と-d-bus-wire-の独立-fixture)。

## プロジェクト固有の情報

エージェントの守る規則は [AGENTS.md](../AGENTS.md) の「プロジェクトの規則」節にある。ここには計画に要る事実と決定を置く。

### 対象 platform（2026-09-24 ユーザー決定）

| platform | 位置付け | tick 周期 |
| --- | --- | --- |
| amd64 | **主対象**。デスクトップ・GPU・アプリケーション。fg010 のデモ | 1000 Hz |
| aarch64（rpi4 ほか） | **主対象** | 1000 Hz |
| i386（pcat・pc98） | デモ用のおまけ。基本のコマンドと Xzed が動けばよく、性能は考えない | 100 Hz |
| sparcv9（sun4u）、m68k（x68k） | サポート外。コードは残す | 100 Hz |

tick 周期は `include/hal/arch/<arch>.h` の `HAL_TIMER_FREQUENCY`。時間の計算は `kern_ms_to_ticks()`・`kern_ticks_to_ms()`・
`KERN_MS_TO_TICKS()` で行い、tick の数を数字で書かない（WS040）。

### 有効なユーザーの判断（全体に関わるもの）

2026-09-29 に整理した。WS に固有の判断はその ws.md に移し、記録先に書かれたもの・後の判断で置き換わったもの・完了した WS のものは削除した（git の履歴にある）。

| 項目 | 決定 | 記録先 |
| --- | --- | --- |
| Kei Operating System（2026-09-28） | ユーザー:「プロジェクトの名前は Kei Operating System とします。カーネルの内部名がzedbsdです。デスクトップの名前はKeilandで、Kei + Waylandなのですが、カーネルからデスクトップまでOSとして垂直統合しているので、デスクトップ環境とかデスクトップみたいにあえて呼ばず、内部名がKeilandです。OSの見えるところからzedBSD, zed, zの名前を徐々に外していきます。zdesktopは/bin/wayland, zdesktop-x11serverは/bin/xserver, zdesktop-browserは /bin/browser にします。以前から、シンボル名にzedbsdを含めないように実装してきましたが、カーネル、ドライバ、UAPIなどで誤って新規実装で混入してしまっているようです。これは一斉に改めましょう。ZEDBSD_ではなくKERN_が望ましいプレフィックスです。ロゴなどでKだけだとKDEの商標を侵害してしまう可能性があるので、かならず Kei と3文字にします。Keiは日本語の軽いという意味です。」→ [WS078](ws078/ws.md) |
| 画像の library の置き場（2026-09-28） | ユーザー:「JPEGライブラリは、userland/base/libjpeg-compatにして、共有にしましょう。GIFもそうするのがいいです。include/libc/jpeg/みたいな位置にヘッダがあるのがいいです。」→ libjpeg-compat は base の共有 library（desktop の分類でなく base）。GIF も `userland/base/libgif-compat`（WS074 design D5 の案を採る）。同日の訂正:「include/libc/compat/jpeglib.hの方がいいです。訂正します。」→ header は今の `include/libc/compat/` のまま（GIF も同じ所）。さらに:「PNGもbase/libpng-compatにして、include/libc/compat/png/に入れましょう。」→ libpng-compat も base の共有 library、header は `include/libc/compat/png/`。zlib（main の問い、ユーザーの回答「base・全 platform・header を compat/zlib/ へ」）→ libz-compat も base の共有 library・全 platform、header は `include/libc/compat/zlib/zlib.h`（PNG も全 platform に）。WS074 が実施 |
| retro・GOP・Kei の印（2026-09-28） | ユーザー:「Keiマークはちょうどいいです。GOPフレームバッファは1920x1080を要求して上下左右の不足部分を黒い帯にすればいいかなと思います。X11のプログラムはuserland/X11/にあると思いますので、それはuserland/retro/に入れましょう。zedinstはretro/に入れておいて、Waylandであとで作り直しますが、それはOSCでのデモでは必須ではないので、優先度を下げます。」→ `userland/X11` → `userland/retro`、`userland/base/zedinst` → `userland/retro/zedinst`（main が実施、image の build・boot test・menu の試験 PASS）。UEFI の loader は GOP の 1920x1080 を求め、足りない所は黒い帯。Wayland の installer の作り直しは低い優先度 |
| デモの利用者・KVM の検討の時期（2026-09-28） | ユーザーの回答: デモは**名前のある利用者**でログインする（指定が無いので利用者名 `kei`、表示名「Kei」。root で空の password のログインはデモの image から外す）。WS082（KVM の検討）は**空いた枠で始める** |
| 既定の image と login（2026-09-29） | ユーザー: root の password を root、kei の password を kei、見つかった外部の display → 内蔵の LCD の順、make の既定の hdd-image.img でデスクトップを試せる、既定で自動の graphical login → base の passwd・group・shadow に kei（uid 1000、network）と SHA-512 の password、i915 は display= が無い・auto・hdmi で HDMI を先に探し、edp・panel で内蔵（boot の parser も受ける）、menuconfig の既定を amd64 に、Venus・i915 を amd64 で既定 y、sessiond が `/etc/keiland/autologin`（既定 kei、root は拒む）の利用者を boot で 1 回 login（Log Out の後は greeter）。serial・pc98 の試験の login は password を送る。確認: 既定の config で image の build と boot test PASS、Venus の guest で greeter を経ずに desktop（`build/ws035-shots/default-image-20260929-autologin.png`）。既定の image は font を持たず文字が出ない、guest の harness の SSH の鍵も無い。実機は未実施 |
| デモの image の root（2026-09-28） | root を lock し su も無いと実機で管理の作業ができない件でユーザーの回答:「root に password を設定する」→ `plan/ws035/demo/demo-accounts.sh` が root に password を付ける（`DEMO_ROOT_PASSWORD`、無ければ 12 文字の乱数。`BUILD/demo-accounts/root-password`（0600、git に入れない）に書く。image には SHA-512 crypt だけ）。空の password の root は引き続き無し。kei は password 無しのまま |
| 表示の build の既定 | ユーザー:「GPUの問題は解決したとみなして、以後はロゴを出してメッセージを隠すビルドにしましょう。ふたたびGPUドライバの修正をするとき、ロゴを無効にしましょう。」→ demo・実機の image は既定（logo と `kmsg=quiet`、`plan/ws075/demo/build-demo-image.sh BUILD`）。GPU の driver を直す Phase だけ `ZEDBSD_GRAPHICAL_BOOT=n "ZEDBSD_BOOT_EXTRA_LINES=display=edp login=graphical"`（logo を消し kernel の message を画面に残す） | Guardrail、WS084 |
| LCD のみの構成 | ユーザー:「HDMIはいったんやめて、LCDのみの構成にします。」→ demo の既定 `display=edp`。HDMI の LCD は WS075 に戻す | WS084、WS075 |
| 文字の符号（2026-09-29） | ユーザー:「我々のOSはutf-8のみをサポートしており、Escapeは不要と思います。LANG=CをUtf-8と解釈するのが乱暴というなら、C.UTF-8を設定するのでもいいです。」→ ls は locale に関わらず名前を UTF-8 として扱い、表示できる UTF-8 の文字を escape しない（escape は制御文字と不正な byte だけ）。幅は UTF-8 の表示幅 | WS086 |
| デモに必須の追加（2026-09-29 夜） | ユーザーの回答: 新しい要望のうちデモ（10/17）に必須は「画像 viewer と text editor」（WS091・WS092、Files からの起動 WS093 を含む）。WS090（widget の library）・WS094（desktop の icon）・WS095（IME）はデモに必須ではない。窓の縁の resize は四隅に加えて辺も入れた（ws035-p128、設計の「枠と角＝resize」どおり） | WS091〜WS093、WS035 |
| デモまでの進め方（2026-09-29 夜） | ユーザー:「実は、すでにデモに耐えられるだけの完成度にはなっています。…いちおう、当日までOSCでのデモという目標は掲げたままにします。まだ当日まで時間があるので、新規実装をどんどん行って、デモの1週間前くらいから、バグ修正とデモ実機での調整のみの期間に入ろうかなと思っています。」→ fg010 は保つ。**2026-10-10 ごろまでは新規実装**（デモに必須でない WS090・WS094・WS095・WS080 等も進めてよい）、**2026-10-10 ごろ〜10-17 は bug の修正と実機（5330）での調整だけ**（新しい機能は入れない） | WS の優先順位 |
| 文字の編集の touch（2026-09-29 夜） | ユーザー:「スクロールは2本指にするのと、共通部品にしましょう。」（Text Editor の touch の選択について）→ 文字を編集する view（Text Editor・text field）では 1 本指の drag を選択、scroll を 2 本指にし、WS090 の共通の部品（libkeiui）で作って Text Editor へ入れる。Files・Image Viewer などの 1 本指の pan は変えない（main の解釈） | WS090、WS092 |
| compositor の速さとすりガラス（2026-09-30） | ws075-p023 の実機の計測（10 app）: すりガラスを切っても compositor の 1 run は縮まず（8.8 → 9.2・8.9 ms）、分岐の中の ALU を飛ぶと 4.9 ms の見込み。ユーザー:「分岐の中の計算を飛ばす、にしますので記録しておいてください。」→ すりガラスは残し、WS075 p023 で panel.frag の分岐の中の ALU を飛ぶ実装をする。WS035 p135（暗い壁紙の上の glass の文字）はすりガラスを残す前提で再開できる | WS075、WS035 |
| ファイルピッカー（2026-09-29 夜） | ユーザー:「テキストエディタのファイルピッカーは、KeiのUIライブラリに入れるのがいいと思いました。」→ Open・Save As の chooser を app ごとに持たず、共有の library（今は libkeiland、WS090 の最初の部品）に置く。作るのは WS092 のエージェント、WS091（画像 viewer）・WS089（Settings）・Notes・PDF Viewer が順に使う | WS090、WS092、WS091 |
| サブエージェントの運用（2026-09-29 夕） | ユーザー:「サブエージェントを使って作業します。N=6で、6エージェントを起動します。メインエージェントであるあなたは、サブエージェントに依頼して、結果を受け取ってマージする、プランナーです。サブエージェントは、5時間の利用制限に到達したときに強制終了されてしまうので、そのときに作業内容が失われます。そこで、こまめにメインエージェントに依頼して、マージを行います。また、強制終了した場合もサルベージ可能なように、作業ディレクトリを構成します。5時間制限の残り時間と使用率から、N=0からN=6の間で調整していきます。サブエージェントにはラップアップを依頼することで、キリのいいところで終了が可能です。」→ worktree は固定の path と branch（`.claude/worktrees/wsNNN-<名前>`・`wt/wsNNN`、前の枠の 2 つは元の path）、build が通るたびに WIP commit、1 回の依頼は 1 Phase、終わるたびに main が merge して同じエージェントに続きを依頼、強制終了は branch と未 commit の差分から回収 | queue.md |
| サブエージェントの effort（2026-09-29 夜） | ユーザー:「いくつかのエージェントは、Opus 5.5のMidで動かすように、エージェント設定を変更したいです。明示的にHighのままにしたいのは、i915、Keilandデスクトップ (WS035）、カーネル、バグフィックス、あたりです。そのほかは、特にブラウザは、Midにしたいです。これは試行回数が大きいですからね。」→ `.claude/agents/phase-runner-mid.md`（effort: medium）を足した。High（`phase-runner`）: i915（WS075）・kernel・bug の修正（Keiland の bug を除く）。Mid（`phase-runner-mid`）: Keiland のデスクトップ（WS035、その bug を含む。2026-09-29 ユーザー「KeilandのサブエージェントもMidにします。」）・touch（WS081、同日「WS081もMidにします。」）・ブラウザ（WS074）・アプリ（WS089・WS091・WS092）・IME（WS095）ほか。走っているエージェントは次の Phase の区切りで Mid の新しいエージェントに引き継ぐ | queue.md |
| toolchain の保護と subagent の範囲（2026-09-28） | ユーザー:「再発防止のため、ツールチェインはメインエージェントの許可がないと変更できないようにしましょう。また、サブエージェントの修正可能範囲を明示しましょう。」→ AGENTS.md の「禁止と承認」に 2 つの規則、`plan/tools/toolchain-lock.sh`（共有の toolchain の tree の directory を読み取り専用に、main だけが一時的に unlock）。経緯: 15:55〜15:57 に古い Makefile の worktree の build が共有の `build/llvm-source` に clang・libcxx の package の patch 5 つ（41 file）を当てた。main が patch -R で戻し、manifest の全 file の SHA-256 と file の一覧の一致を確認（BUG-096） |
| 試験の範囲（2026-09-27 14 時半） | ユーザー:「試験はamd64のみにしましょう。phase内ではビルドが通れば先に進み、phaseの最後にテストしましょう。」→ 試験は amd64 だけ（pcat・pc98・rpi4 は走らせない）。Phase の途中は build が通れば進み、試験（guest の試験・回帰・boot test）は Phase の最後に 1 回。main の merge の後の検証も amd64（デスクトップの image と boot test）だけ | 検証、AGENTS.md の回帰の範囲の例外 |
| 古いグラフィックの試験の driver（2026-09-28） | ユーザー:「venus-backend-testのような初期のテストドライバは、もう使わなくてOKです。グラフィック関連の古いテストドライバは捨てて、回帰テストは不要です。デスクトップ環境が起動しているからです。どうしても特定の機能をテストしたいときは、そのときにテストを書いてください。i915はまだexecutorを実装する必要があるので、テストドライバは残していいです。」→ 初期のグラフィックの試験の driver（venus-backend-test 等）は削除してよく、回帰の対象から外す。デスクトップの起動が回帰の代わり。特定の機能は必要なときに試験を書く。**i915 の executor の試験の driver（vkx・vke1・vke2・vkc 等）は残す** | 検証、WS030・WS031・WS075 |

### 主な依存関係

- WS046（make）→ guest での expat の build（WS042 の残り）。
- ws035-p051（承認）→ p052〜p055・p057（合成）→ fg010。
- WS014・WS031（GPU の土台）→ WS035 の合成とアプリ。
- WS049（AML）→ WS050（UCSI）→ WS051（DP Alt Mode、i915 の display も要る）。WS049 → WS052（S0i3）。
- WS036 p026（AArch64 の LLVM target）→ aarch64 の userland と package。

### 参照資料

- [設計方針・決定の参照資料](master-design-policy.md): 独立実装・ライセンス境界、module の設計、toolchain、個別の設計判断。
- [コーディング規約](coding-style.md)、[Guardrail](guardrail.md)、[Awesome Plan の設定](config.md)。

WS105 p005 の追加道具: [display-probe.c](tools/keiland-linux/display-probe.c)（guest 専用、seat fd / direct の3色・oldSwapchain・CRTC復元）。[使い方](tools/keiland-linux/README.md)。

WS105 KMSの再開検証: [flip-delay.c](tools/keiland-linux/flip-delay.c)（test-only、単発poll timeoutの後の実eventを検証）。

WS105 p010 の追加道具: [network-probe.c](tools/keiland-linux/network-probe.c)・[audio-probe.c](tools/keiland-linux/audio-probe.c)（production library / userkei の実WiFi・ALSA）、[wifi-setup.sh](tools/keiland-linux/wifi-setup.sh)（disposable guestのhwsim AP、192.0.2.2の試験専用IP）。
