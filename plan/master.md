<!-- awesome-plan project=zedbsd record=master -->

# zedBSD Master

[Queue](queue.md) · [Guardrail](guardrail.md) · [Future Work](future-work.md) · [Bug Board](known-bugs.md) · [Past Log](history/index.md) · [設定](config.md) · [Agent の運用](agents/protocol.md) · [Agent の台帳](agents/registry.md) · [GitHub Project](https://github.com/users/awemorris/projects/2)

<!--
  現在の状況の領域。各 block は「master:<名前>:start」から「master:<名前>:end」までで、sed/awk で丸ごと置き換えてよい。
  例: sed -i '/master:agents:start/,/master:agents:end/{//!d}' plan/master.md の後に新しい行を差し込む。
-->
<!-- awesome-plan-current:start -->
## 現在の状況

<!-- master:updated:start -->
更新: 2026-10-03 夜 Q1（全担当のソフトな停止と統合、リポジトリの作り直しの前の持ち越し）
<!-- master:updated:end -->

<!-- master:agents:start -->
- 体制: 単一 session の Q1 ＋固定名サブエージェント（実装 P1〜P8、試験 T1・T2）。2026-10-03 夜 user「これによりすべての作業をソフトに停止します。」で P1・P2・T1・T2 はラップアップして終了、全ての成果は main に統合済み（P4 の WS118 の source と記録も Q1 が取り込んだ）。動いている担当は無い。
- 再開の時の割り当ての候補（user が決める）: P1 = WS131 の p008 の試験と p009 以降、P2 = WS134 の p003 の直しと p004、T1・T2 = 台帳の未実行の予約（`plan/agents/T1/requests.md`・`T2/requests.md`）。WS135（設定の一本化、BUG-162）の担当と時期は未定。
<!-- master:agents:end -->

## リポジトリの作り直し（2026-10-03 夜、持ち越し）

2026-10-03 user「リポジトリを作り直します。build/以下は削除されます。.git/も削除されます。持ち越したい情報があれば、plan/以下のドキュメントに記載が必要です。」

- **git の履歴**: plan の中の commit の SHA（`6ecf801cc`・`f94b1b633` など、2026-10-03 以前の全て）は古いリポジトリの物で、作り直した後は引けない。記録の意味（どの変更か）は文で残っている。
- **build/ の証拠**: plan が指す `build/…` と `/home/awe/zedBSD-worktrees/*/build/…` の PNG・log・image（S1 の image `build/s1-pre/hdd-image.img` を含む）は消える。結果と判定は各 phase.md・Bug の ticket・`plan/agents/T1|T2/requests.md` に文で残っている。消えた証拠を指す行を「証拠の file が残っている」と読まない。
- **作り直した後の手順**（Q1）:
  1. commit の hook: `cp plan/tools/git-hooks/commit-msg .git/hooks/ && chmod +x .git/hooks/commit-msg`（メッセージが `WIP` ちょうどでない commit を拒否。AGENTS.md「git」）。`.claude/settings.json` の `attribution`（Co-Authored-By などの付記を無効）は tree にあるので残る。
  2. remote は `git@github.com:awemorris/zedBSD.git`、author は `Awe Morris`。push はユーザーが指示した時だけ（AGENTS.md の確かめの手順）。
  3. サブエージェントの worktree: 古い `/home/awe/zedBSD-worktrees/*`（agent/p1〜p4・t1・t2、codex/a1〜a3・p8〜p10）と `/home/awe/zedBSD-rpi4/.claude/worktrees/*` は古い `.git` に結び付くので使えない。担当を起こす時に main が `git worktree add /home/awe/zedBSD-worktrees/<名前> -b agent/<名前>` で作り直す（[protocol](agents/protocol.md) の 1）。古い worktree の directory の削除はユーザーが行う。
  4. toolchain: `make toolchain` などで `build/llvm`・`llvm-source`・`llvm-build`・`NoctLang` を作り直した後に `plan/tools/toolchain-lock.sh lock`。
  5. Linux の試験の guest（WS105・WS131、QEMU+KVM）: `plan/tools/keiland-linux/build-guest.sh`（base と gdm）で `build/keiland-linux/guest`・`guest-gdm` を作り直す。guest の SSH の鍵 `plan/tmp/guest/` は tree にある。
  6. 統合の前の残り: 無い（全ての agent の branch は main に入っている。`worktree-agent-aefedcaf…` の 2 commit は BUG-066 の決定で merge しない物）。P1 の `build/p1-q640/p007-wip.patch` は p007 が commit 済みなので不要。
- **tree に残るが git に入らない物**（削除の対象外の前提）: `.wifi`（WiFi の認証情報、ユーザーが作成）、`.claude/settings.local.json`、`config.mk`、`plan/*/temp/`。
- **host の状態**（repo の外）: sysctl の一時の設定（`vm.dirty_background_bytes=512M`・`vm.dirty_bytes=2G`・`vm.swappiness=10`、2026-10-03 user「sysctl の調整はやってみてください」）は再起動で戻る。残すかは user の判断待ち。fstrim は 2026-10-03 に実行済み（35 分、204.6 GiB）。

<!-- master:focus:start -->
- **fg019 ベータ1 のリリース（目標 2026-10-17、凍結なし、できた所までをベータ1 に。版 zedbsd-0.1.0-beta1）**。内容は下の「Current Focused Goals」。
- fg018 Linux 標準 GTK4（WS114 は p007・p008 まで達成、GTK4 の zedBSD 移植 WS115 は後回し）。
<!-- master:focus:end -->

<!-- master:blocked:start -->
- 5330 の AX211 の passthrough は停止（host の hang 2 回）。iGPU は i915 の driver の改善の Phase だけで使い、iGPU と AX211 の同時は禁止。
- 実機の要る bug（BUG-157・158・119・159・143・120・156 と QEMU で resolved にした bug の実機の確認）は安定版 S2 の実機試験で（[WS133](ws133/ws.md)）。
- GitHub への記録の公開は保留（.sync が無い）。push はユーザーの指示の時だけ。
<!-- master:blocked:end -->

<!-- master:pending-decisions:start -->
- WS135（設定の一本化、BUG-162）の担当と開始の時期。WS131 の p010・p011 の設定の部分は WS135 が引き取る。
- host の sysctl の設定を `/etc/sysctl.d` に残すか。
- docs/ の本文に残る Plan の ID の記述を消すか。
- WS132（PnP の通知・自動 mount・eject）をベータ1 に入れるか。
- 5330 の host の設定（AX211 を起動時から vfio-pci、iwlwifi・btusb の blacklist）。
<!-- master:pending-decisions:end -->
<!-- awesome-plan-current:end -->

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
| **MG002** UNIX アプリケーションの実行基盤 | O1 | process・memory・libc・loader/TLS の対応範囲を互換性台帳と代表アプリで確認できる | TLS（WS022）と外部 package の導入（WS032）は完了。base の utility の POSIX 化（WS043）は完了。POSIX 台帳（WS001）、アプリ導入（WS034）、sh（WS042）が進行中 | WS001, WS022, WS032, WS034, WS042, WS043, WS045, WS046, WS061, WS115, WS116 |
| **MG003** 対象機へ導入して単独起動 | O2, O4 | 合意した機種・媒体でインストール後の単独起動と login を確認できる。実機と QEMU の証拠を分ける | インストーラ（WS019）と Intel Mac（WS020）は完了。4 機種の実機受け入れ（WS028）が残る | WS003, WS004, WS019, WS020, WS028 |
| **MG004** データの保持とメモリ/ストレージの実用 | O1, O2 | 永続化、低メモリ時の進行、媒体世代、既定構成の性能を確認できる | swap（WS016）、UFS（WS024）、I/O・cache（WS025）は完了。実機の性能の一部は未測定。UFS の directory は 12 block まで育つ（WS054、完了） | WS016, WS024, WS025, WS054, WS057, WS058, WS059, WS060 |
| **MG005** 一貫したネットワーク/サービス管理 | O1, O2, O3 | networkd・netconf・service の責務・設定・操作が一貫し、永続化と失敗後の復旧を確認できる | サービス（WS002）、net console（WS011）、service console（WS012）は完了。有線 LAN の常駐管理（WS005・WS033）が残る | WS002, WS005, WS011, WS012, WS033 |
| **MG006** グラフィカルな操作環境 | O2 | 入力・描画・ウィンドウ・端末・GUI ツールの一連の操作を確認できる | 入力（WS006）、Noct/BeUI（WS008）、標準 Vulkan（WS030）、即時起床（WS041）は完了。**Wayland デスクトップ（WS035）が fg010 の中心**。WS104 の OS 境界は A1〜A6 と全体回帰で完了、LinuxのWS105はL1〜L9・全文規約/両OS最終回帰でcompleted（fg012達成、既知resizeはユーザー許可のtracking）。WS109 nativeFreeBSD15.1/実i915/主要app/backend/全文規約がq572でverified、q574実機build/installとp008ユーザー実操作でF6受け入れ合格。WS113複数displayとWS114 Linux標準GTK4互換性は計画のみ。デモ実機を含むMG006全体は未完了 | WS006, WS007, WS008, WS014, WS017, WS029, WS030, WS031, WS035, WS037〜WS039, WS041, WS068, WS104, WS105, WS107, WS109, WS113, WS114 |
| **MG007** 用途別の独自ディストリビューション | O1, O2 | 第三者が用途別に構成し、独自ブランドで build・配布できる | 担う作業は一部だけ（WS013・WS015 は Future Work に保留）。WS105独立/opt build・installとWS108の2OS native deb/QEMU検証・CI/release定義をverified、WS109 nativeFreeBSD独立build/install/privateprefixもverified、5OS package/CI配布はWS112で計画のみ、MG007全体は未充足 | WS013, WS015, WS108, WS112 |
| **MG008** 最小 HAL の移植契約と異種機での実証 | O4 | HAL 契約・移植手順と異種/レトロ機での実証を公開する | source の所有の整理（WS018）と時間の単位（WS040）は完了。他 platform への反映（WS036、aarch64 を含む）と PowerPC（WS027）、rpi4 の開発環境（WS044）が残る | WS018, WS027, WS036, WS040, WS044 |
| **MG009** AI 活用 OSS 開発の知見の公開 | O5 | 設計権限・レビュー・変更追跡・失敗からの回復の事例と根拠を公開する | 担う作業が未定義 | なし |

## Current Focused Goals

| Goal | 当面の成果 | Milestone | 担当の WS | 出典 |
| --- | --- | --- | --- | --- |
| **fg019** | **ベータ1 のリリース（目標 2026-10-17）**。凍結なし、できた所までをベータ1 にし、安定化は後のベータの版で。版と tag は `zedbsd-0.1.0-beta1`、CI が Prerelease を作りユーザーが動作確認して Latest に手で昇格、配布物は USB の image と Windows の QEMU/Venus の zip | MG003・MG006・MG007・MG002 | 下の「fg019 の内容」 | 2026-10-02 user「次のFeature Goalはベータ1のリリースにします」ほか |
| **fg018** | Linux 標準 GTK4 の実測と互換性の改善 | MG006 | [WS114](ws114/ws.md) | 2026-10-02 user。p007・p008 まで達成 |

### fg019 の内容（2026-10-02〜03 のユーザーとの議論で決定）

| 区分 | WS | 状態・メモ |
| --- | --- | --- |
| デスクトップの基盤 | [WS099](ws099/ws.md)（BUG-125 は blocking）、[WS094](ws094/ws.md)、[WS114](ws114/ws.md) p008 | BUG-125 の原因と修正は済み、BUG-147 の試験の頑健化の後に閉じる |
| ネットワーク（WiFi を含む） | [WS005](ws005/ws.md)、[WS033](ws033/ws.md) | WiFi は network group に制御を許可、自動再接続は起動時（system の store）と login（利用者の store） |
| IME 日本語 | [WS095](ws095/ws.md) | 辞書・inline の変換中の文字・候補の窓・右上の status は済み |
| desktop の構造 | [WS131](ws131/ws.md)（libkeiland-backend と libkeiland、libkeiui の吸収）、[WS132](ws132/ws.md)（PnP の通知） | WS131 は移行計画のレビュー待ち |
| 標準 app（WS131 の後に組み直す） | [WS127](ws127/ws.md) Files（最重点）、[WS089](ws089/ws.md) Settings（重点）、[WS128](ws128/ws.md) 他の app、[WS120](ws120/ws.md) 音楽（m4a） | 日本語 UI はベータ2 以降（F-068） |
| 複数 display | [WS113](ws113/ws.md) | D-ATOMIC は (a) で決定、WS131 の後 |
| 対象 platform | Latitude 5330、[WS118](ws118/ws.md) Latitude 5320 | 5320 は RTL8156 の USB の LAN で遠隔の log |
| インストーラ | [WS119](ws119/ws.md) | Wayland、disk 全体のみ、UEFI のみ。WS118 の後 |
| packages | [WS124](ws124/ws.md) Emacs（端末版）、[WS125](ws125/ws.md) vim、[WS126](ws126/ws.md) Python 3（core） | release の image に入れる |
| GTK4 | [WS115](ws115/ws.md) | 素の GTK4 の移植は後回し（p010 の ld.so の上限から再開） |
| 動画（drop 可、別セッションでユーザーと） | [WS083](ws083/ws.md)・[WS122](ws122/ws.md)・[WS121](ws121/ws.md) | このセッションは割り当てない |
| リリース作業 | [WS129](ws129/ws.md) | — |
| 最後 | [WS112](ws112/ws.md) Linux の package | 優先度最下位 |
| 対象外 | [WS074](ws074/ws.md) ブラウザ | Codex が担当 |

### 達成した Focused Goal

| Goal | 成果 | 達成 |
| --- | --- | --- |
| fg010 | 2026-10-17 の OSC のデモの Kei Operating System（Latitude 5330） | 2026-10-02 user 判断（実装で到達、nightly の release で公開） |
| fg012 | Keiland の OS の境界の整理と Linux の Keiland（[WS104](ws104/ws.md)・[WS105](ws105/ws.md)） | 2026-10-01 |
| fg013 | test・見本を userland/tests へ（[WS106](ws106/ws.md)） | 2026-10-01（WS106 は残りあり） |
| fg014 | libbrowser の component 化（[WS107](ws107/ws.md)） | 2026-10-02 |
| fg015 | Debian・Ubuntu の native の deb と CI（[WS108](ws108/ws.md)） | 2026-10-02 |
| fg016 | native FreeBSD15 の Keiland（[WS109](ws109/ws.md)） | 2026-10-02 |

以前の詳しい記録（fg010 のデモの台本・判断、各 WS の実行優先の経緯）は [master の旧版](history/master-2026-10-03-before-rewrite.md) と git の履歴にある。

## WS の優先順位

依存による実行の順とは別のもの。Queue の権限は変えない。最新の指示は上の「現在の状況」の master:next。

1. **debug**（P1・P2）: WiFi（BUG-145・BUG-138、ws005-p020）→ BUG-147（ws099-p024）→ 優先の bug（BUG-052 → BUG-120 → BUG-143。BUG-135 は WS131 p005・p006 の後）。
2. **[WS131](ws131/ws.md)**（P3）: libkeiland-backend の分離と libkeiui の吸収。移行計画のユーザーのレビューの後に実装。
3. **標準 app**（WS131 の後に組み直す）: WS127 Files → WS089 Settings → WS128 他の app → WS120 音楽、WS113 複数 display。
4. **platform と導入**: WS005・WS033 の残り → WS118 5320 → WS119 インストーラ（BUG-041 はこの後）→ WS132 PnP。
5. **GTK/Qt**: WS115（p010 から）→ WS097 独自 GTK4 → WS117 Linux Qt6 → WS116 Qt6 移植 → WS096。
6. **packages**: WS125（image に多数の file を入れる共通の仕組み）→ WS124 → WS126。
7. **リリース**: WS129、最後に WS112。
8. 動画（WS083・WS122・WS121）は別セッション、WS074 は Codex。WS130（IPv6）は計画だけ、実装はベータ2 以降。
9. 上に無い未完了の WS（WS001・004・007・009・014・017・026〜029・031・034・036〜039・044〜052・061・066・068・075・077〜088・090・096〜098・100〜102・106・110 ほか）は順位を定めていない。WS095 以外のデモの頃の WS（WS079・WS090・WS100・WS102 など）は標準 app の組み直しで扱う。

## Workstream registry

状態は各 ws.md が正本、ここは投影。完了した WS の Phase の記録は 2026-09-24 に plan から削除した（git の履歴にある）。

| WS | Primary | 内容 | 状態 | 再開点 |
| --- | --- | --- | --- | --- |
| [WS001](ws001/ws.md) | MG002 | POSIX.1-2024 準拠 | incomplete | p033〜p039 cleared（patch、df・du、who、stty、dirname、mktemp・install・base64、xargs）、main へ merge。p040 mesg は実装を merge、uncleared（console の case は BUG-067 待ち）。以後はユーザーの指示のときだけ |
| [WS002](ws002/ws.md) | MG005 | システムサービス | completed | — |
| [WS003](ws003/ws.md) | MG003 | 旧実機 bring-up（終了・再利用禁止） | completed（ユーザー判断で終了） | 未完了は WS027・WS028・F-004 へ |
| [WS004](ws004/ws.md) | MG003 | ハードウェア拡張 | incomplete | NVMe 実機・転送・driver 共通化 |
| [WS005](ws005/ws.md) | MG005 | ネットワーク・WLAN | incomplete | p019（network group の WiFi の制御・system bar の鍵の入力・有線優先）と p024（起動時・login・logout の自動再接続）は実装済み。p020（RTL8822BU の USB passthrough）で DHCP の EIO の原因（ブロードキャストが有線の gateway へ）を直し、2.4GHz で lease を確認、残りを P1 が確認中（q627） |
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
| [WS073](ws073/ws.md) | MG002 | Bug Board のbug解消（2026-09-27の対象境界を保持）。2026-10-02のP8はWS073に限らず、mainが各bugの既存handling WS/Phaseを照合して配属 | incomplete | p045（BUG-135、UFS の namespace_lock と journal の commit の待ち）を修正（停止 10→2 回）。残りは WS131 p005・p006 の後 |
| [WS074](ws074/ws.md) | MG006 | zedBSD の Web ブラウザ `userland/base/zdesktop-browser`（HTML5 の layout engine → 最適化にこだわらない JavaScript engine の接続 → CSS の準拠と Chrome との比較で目標値を段階的に上げる。JS と Wasm の実行 engine を共通化。画像は libpng-compat・新しい libjpeg-compat、TLS は当面 OpenSSL）（2026-09-27 ユーザー指示） | incomplete | p099 cleared（q507、Acid2 100%）。p172/q579はbranch統合済みだが最終review残でuncleared。A1でp172再開後、p100→p174→p175→p173→p176。p101 CSS2も保持 |
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
| [WS089](ws089/ws.md) | MG006 | 設定のアプリ（Settings: 左に項目の pane、右に設定、浮いたすりガラスの pane）（2026-09-29 ユーザー、デモの優先事項） | incomplete | p010（回帰と棚卸し、候補 C1〜C17）・p012（検索の key・touch の scroll・Wi-Fi の待ちの slot・文言）は済み。p019（titlebar の検索欄の Down・Up、compositor 側）。標準 app の開発は WS131 の後に組み直す |
| [WS090](ws090/ws.md) | MG006 | widget・control の共有 library（少なくとも慣性の smooth scroll、独自の部品）（2026-09-29 ユーザー） | incomplete | 2026-09-30: p001〜p006・p013・p008（PDF Viewer・Image Viewer）・p011（Terminal・Notes の窓）・p014（file chooser を親の title bar にぶら下がる sheet、不透明）cleared。KUI_VERSION 11・KEILAND_VERSION 20。統合の試験（demo-s8-s9.sh、QEMU）PASS。残り: p015（案、scroll を kui_scroll へ）・p009・p010（Files）・p007（Settings）、Terminal の p088 の切り分け。ラップアップ（2026-09-30 夕） |
| [WS091](ws091/ws.md) | MG006 | 画像 viewer（2026-09-29 ユーザー） | completed | 2026-09-29 完了（QEMU）: `/bin/imageview`、PNG・JPEG（EXIF の向き）・GIF（動く）、fit・拡大・pan・pinch・慣性、前後の画像、全画面。Files からの起動は WS093、実機の確認は残り |
| [WS092](ws092/ws.md) | MG006 | text editor（simple）（2026-09-29 ユーザー） | completed | 2026-09-29 完了（QEMU）: `/bin/textedit`、共有の file chooser（libkeiland、KEILAND_VERSION 12）、touch・PRIMARY・clipboard・Files からの起動。実機は未実施。touch の drag での選択は無し |
| [WS093](ws093/ws.md) | MG006 | Files から app の起動（画像・text の double click、file の種類と app の対応）（2026-09-29 ユーザー） | completed | 2026-09-29 完了（QEMU）: Files の double click・Enter・double tap で png・jpeg・gif → Image Viewer、text 系 → Text Editor、html → Browser、pdf → PDF Viewer。Always Open With と Use System Default（`~/.config/keiland/open-with`）。実機は未実施 |
| [WS094](ws094/ws.md) | MG006 | desktop の file の icon（`~/Desktop`）（2026-09-29 ユーザー） | incomplete | p014（規約の指摘）cleared。残り p012（5330 の実機）・p007（全回帰） |
| [WS095](ws095/ws.md) | MG006 | IME（Wayland の input-method-v2・text-input-v3、単一の IME・複数言語、まず日本語、REmacs の辞書）（2026-09-29 ユーザー） | incomplete | p012（辞書 1,478 見出し）・p013（変換中の文字を本文と同じ大きさで inline）・p005（候補の窓・右上の IME の status・key repeat）cleared（QEMU）。実機の目視はユーザー |
| [WS096](ws096/ws.md) | MG002 | Qt6（core・gui・widgets）の互換の書き下ろし（API の interface だけ、zlib）（2026-09-29 ユーザー、デモの後） | planning | デモの後 |
| [WS097](ws097/ws.md) | MG002 | GTK4 の互換の書き下ろし（API の interface だけ、zlib）（2026-09-29 ユーザー、デモの後） | planning | デモの後 |
| [WS098](ws098/ws.md) | MG006 | IME の変換のニューラル化: 辞書で候補を作り、小型のモデル（15 MB 未満）で同音異義語の選択（語の番号の並び）とひらがな列の形態素解析（語の境界と品詞、BiLSTM か小型の Attention）を評価する（2026-09-29 夜 ユーザー、IME の最後の仕上げ） | planning | WS095 の基本の辞書の後。学習の corpus と license はユーザーの判断 |
| [WS099](ws099/ws.md) | MG006 | Keiland の compositor（zdesktop）のデモの基準: 窓の操作・App Home・Wiseview・全画面と最大化の解除・greeter から Log Out と Shut Down・すりガラスの上の文字の contrast・回帰の試験の全通過（2026-09-30 ユーザー、WS035 の後継。基準は ws.md） | incomplete | p023（BUG-136/137、直す前からの C 基準の失敗）cleared。p020（BUG-125）と p021 は BUG-147 の試験の頑健化（p024、P2 が作業中）の後に判断 |
| [WS100](ws100/ws.md) | MG006 | system bar の音量: 右上の通知領域の音量の icon、音量の slider と mute、変えたときの確かめの音（2026-09-30 ユーザー。動画の再生はデモの後） | incomplete | 2026-09-30: L1 がそろった（A1〜A6、Settings の Sound の頁）。L2 は 5330 の実機（A7、ユーザー）。L3 の p008 cleared: 確かめの音の遅れは QEMU の guest の中で中央値 31〜37 ms（≤ 50 ms、合否は実機で）。kernel の fragment を小さくする直しは実機で 50 ms を超えたとき |
| [WS101](ws101/ws.md) | MG006 | GPU の compute: i915 の Vulkan の compute（dispatch・shared memory・barrier・atomic）、libglesv2 の GLES 3.1 の compute、Noct の自動並列化（accel_opengles）が 5330 の GPU で動く（2026-09-30 ユーザー、10/17 のデモまで、最優先ではない） | incomplete | 2026-09-30: L1 で S13 が 5330 で通った。L2: p016（時間の分解）cleared、p017（buffer の使い回しと copy の削減、QEMU の GPU の call 835 → 212 ms、CPU の 10.6 倍遅い）uncleared。ユーザーの判断「今のまま」で S13 は今の見本、最適化はここで止める（5330 の p017 の値は P1 が追記） |
| [WS102](ws102/ws.md) | MG006 | スクリーンキーボード: 右下の角の swipe で右側に flick の panel（英字・記号・日本語）、左下の角の swipe で下側に QWERTY と手書き（認識は stub）。compositor に直接（2026-09-30 ユーザー） | incomplete | 2026-09-30: L1 を満たした。L2: p006・p007・p008・p009・p015・p016（右の列の道具の面）・p017・p018・p020・p021・p023・p024（履歴の tab。受け入れの手順だけ PASS、全手順の回帰・C9・boot test は未実施）、L3 の p019（色付きの絵文字）cleared（QEMU）。ユーザーの指示で優先を下げてラップアップ（2026-09-30 夕）。保留: p022（絵文字の tab）・BUG-125・p010・p011（速さ）・p012（IME、人間） |
| [WS103](ws103/ws.md) | MG006 | compositor を libvulkan だけにする（GPU の UAPI の直の ioctl を無くす）（2026-09-30 ユーザー「規則にして今移す」、規則は Guardrail） | completed | 2026-10-01 完了（p001〜p007、q508〜q514）: V1〜V4 を満たす（QEMU の Venus と 5330 の passthrough、単独の実機の起動は未実施）。Linux・FreeBSD の backend は F-065。試験は plan/tools/gpu-boundary |
| [WS104](ws104/ws.md) | MG006 | Keiland の OS の境界の整理: desktop の公開の header を `userland/desktop/keiland/` へ、libkeiland と compositor の OS の部分を `zedbsd/` の module へ、install の path を macro に。zedBSD の振る舞いは変えない（2026-10-01 ユーザー「Linux移植を進めます」、WS105 の準備） | completed | q515〜q522 / A1〜A6 verified。全文規約と全必須回帰 PASS、Linux は WS105 へ |
| [WS105](ws105/ws.md) | MG006 | Keiland を Linux で動かす（`/opt/keiland`）: `make keiland-linux`、libvulkan-compat（独自の WSI から system の libvulkan へ chain）、compositor の Linux の module（KMS・evdev・linux-dmabuf・logind）、主な app、gdm、wpa_supplicant・ALSA（2026-10-01 ユーザー、F-065 の Linux の分） | completed | L1〜L9/最終source conformance verified、q538 finished。Linux host/ownDebian13guest・zedBSD回帰、BUG-125/127は未修正trackingのユーザー許可。GitHub publication pending、次の実装なし |
| [WS106](ws106/ws.md) | MG001 | base/desktop の test/probe/demo 30件を userland/tests/ へ移し、package/config/install と既存の動作を維持 | incomplete | q540 partial cleared、p002 uncleared（ime-probe回答待ち）、p003未実行。 |
| [WS107](ws107/ws.md) | MG006 | engine の source を libbrowser に所属させ、Wayland無し・標準Vulkan/抽象入力の component と browser shell を整備 | completed | B1〜B5 verified / q544、API v2/public Vulkan client/最終boot。GitHub deferred |
| [WS108](ws108/ws.md) | MG007 | CI で Debian13/Ubuntu26.04 の Linux Keiland .deb を別々に作成/検証/artifact保存 | completed | P1〜P5 / q549、2OS native deb＋QEMU runtime、CI/release定義。remote未実施 |
| [WS109](ws109/ws.md) | MG006 | Linux版の共通描画を利用した native FreeBSD15 Keiland、audio/network/WiFi backend | completed | q574 native実機build/install+全文規約、p008 user「完璧に動作しました」でF6受け入れ合格 |
| [WS110](ws110/ws.md) | MG006 | 通常compositor起動を既定にし--testingで試験用有限modeを明示 | planning | ユーザー指定で検討のみ、alias/opt契約案とlaunch候補を保存、実装未承認 |
| [WS111](ws111/ws.md) | MG006 | Linux/FreeBSD共通console keiland-desktop、GDMはdirect維持 | completed | q575/q576: 共通console launcher/両native install/全source確認、GDMdirect不変。--login検討のみ |
| [WS112](ws112/ws.md) | MG007 | Linux5種類のbinary packageを指定make/CIで作成しreleaseへ添付 | incomplete | p001/q585契約調査uncleared、D1 Fedora/Arch boot回答待ち。RPi arm64、CI runtime不要、FreeBSD source-only。q591は候補のみ |
| [WS113](ws113/ws.md) | MG006 | zedBSD i915 hotplug/Vulkan Displayから複数画面・Settings/libkeiland・窓の全体移動 | incomplete | p001/q586設計調査uncleared、D-ATOMIC未決、A3成果回収/終了。全拡張/全mirror、pointer越境で窓一括移動。実装未投入 |
| [WS114](ws114/ws.md) | MG006 | Linux標準GTK4互換性を調査し機能表レビュー後にXDG-shell/portal等を選択改善 | incomplete | p007（Linux 本物の GTK4 の CSD）・p008（KDE の server decoration、宣言の無い窓は SSD）cleared |
| [WS115](ws115/ws.md) | MG002 | upstream GTK4をzedBSD `packages/desktop/gtk4`へ移植し知見を記録 | incomplete | p001・p004〜p009・p002（GTK 4.18.6 と依存一式の build）cleared。p010（zedBSD で起動）は ld.so の上限で止まり、ユーザーの判断で後回し（再開点は phase010） |
| [WS117](ws117/ws.md) | MG006 | Linux の本物の Qt6 を調査し、素の Qt6 アプリが動くよう compositor を改良（WS115/116 の前） | planning | WS115・WS097 の後 |
| [WS118](ws118/ws.md) | MG003 | Latitude 5320 で Kei を動かす（LCD の制御の不具合、sshd の遠隔 log 用 image、ユーザーと実機） | planning | p001（遠隔 log の image A は QEMU で SSH・collect まで）。実機は RTL8156 の USB の LAN でユーザーと |
| [WS119](ws119/ws.md) | MG003 | インストーラの作り直し | planning | p001 planned（要件の案） |
| [WS120](ws120/ws.md) | MG006 | 音楽アプリ（fg019） | planning | p001（設計、m4a だけ・AAC を独自実装）。標準 app の開発は WS131 の後 |
| [WS121](ws121/ws.md) | MG006 | Web ブラウザでのアクセラレーションつきのビデオ再生（fg019） | planning | p001 |
| [WS122](ws122/ws.md) | MG006 | 動画プレーヤアプリ（fg019） | planning | p001 |
| [WS123](ws123/ws.md) | MG006 | VA-API のライブラリ | canceled（2026-10-02 user、アプリが Vulkan Video を直接使う） | — |
| [WS124](ws124/ws.md) | MG002 | GNU Emacs の package（fg019） | planning | p001・p002 planned |
| [WS125](ws125/ws.md) | MG002 | vim の package（fg019） | planning | p001・p002 planned（p002 は package の tree を image に入れる共通の仕組み） |
| [WS126](ws126/ws.md) | MG002 | Python 3 の package（fg019） | planning | p001 planned |
| [WS127](ws127/ws.md) | MG006 | Files のベータ1 のブラッシュアップ（最重点）（fg019） | incomplete | p001（棚卸し）cleared、p002（BUG-140・Move To・New Window・重ね表示の scroll bar・spring-loaded・PDF の thumbnail）は eject を除き済み。eject は WS132。標準 app の開発は WS131 の後 |
| [WS128](ws128/ws.md) | MG006 | 標準アプリ全般のベータ1 のブラッシュアップ（fg019） | incomplete | p001（棚卸し、候補 C1〜C15）・p002（Notes の Open・Save As）・p003（Text Editor の Replace・Open Recent）cleared。標準 app の開発は WS131 の後 |
| [WS129](ws129/ws.md) | MG007 | ベータ1 のリリース作業（版・release notes・既知の問題・CI の release・最終回帰）（fg019） | incomplete | p009（デモの image を CI 土台に）・p010（全 desktop app と base の program を config へ）cleared。版 zedbsd-0.1.0-beta1、Prerelease を user が手で昇格 |
| [WS130](ws130/ws.md) | MG005 | IPv6 の network stack（ベータ1 は計画だけ、実装はベータ2 以降。DHCPv6 は `dhcpc -6`） | planning | p001 設計 |
| [WS131](ws131/ws.md) | MG006 | libkeiland を GUI toolkit 兼 desktop 機能の抽象化層にする（app の窓の作成を含む GUI の構築を共通化） | incomplete | p003〜p005 cleared、p006（Linux は T1-042 PASS、zedBSD の T2-007 は未実行）・p007（demo-s8-s9 未実行）・p008（試験未依頼）は uncleared。次は p008 の試験と p009。設定の部分は WS135 へ |
| [WS132](ws132/ws.md) | MG006 | /dev/system の電源管理と PnP の通知（subscriber が事象を指定）、自動 mount、Files の eject | planning | p001 設計（/dev/system に電源管理と PnP の通知、subscriber が事象を指定） |
| [WS133](ws133/ws.md) | MG003 | 安定版 S1 の実機試験（安定版の image を実機で起動し SSH で複数の試験を詰め込む。最初の項目は ws005-p020・p024 から移した WiFi） | planning | 安定版 S1 の内容と試験の一覧はユーザーと決める |
| [WS134](ws134/ws.md) | MG006 | システムモニターのアプリ（Analytic Spatial UI、中央の状態コア、層構造、2026-10-03 ユーザー） | incomplete | p001・p002 cleared、p003 uncleared（sim の fps 4.7）、p004 uncleared（`interact.c` まで） |
| [WS135](ws135/ws.md) | MG006 | 設定の読み書きを libkeiland に一本化（libkeiland が直接か compositor の拡張で解決、監視と通知の API、desktop.conf は compositor の内部で session の開始・終了だけ読み書き。BUG-162、2026-10-03 ユーザー） | planning | p001 設計 |
| [WS116](ws116/ws.md) | MG002 | upstream Qt6の範囲をGTK4移植後に検討し `packages/desktop/qt6`へ移植 | planning | WS115の知見後。旧WS034 p030移管、Queue none |

完了した WS の Phase の記録は 2026-09-24 に plan から削除した（git の履歴に残る）。

## Upcoming Work Outlook

見込みであって、約束や実行許可ではない。担当の線と順:

| 線 | 順 |
| --- | --- |
| P1（network・debug） | ws005-p020（WiFi、q627）→ BUG-052 → BUG-120 → BUG-143 → 低優先度の確認（BUG-036・033・027・103）→ WS118（5320、ユーザーと）→ WS119 |
| P2 | ws099-p024（BUG-147、q625）の後に畳む |
| P3（WS131） | p002 移行計画 → ユーザーのレビュー → p003〜p016 |
| 別セッション（ユーザー） | WS083 → WS122 → WS121 |

---

# 付録

## Tools

回帰と観察の道具は `plan/tools/` に置く。完了した WS の試験は、ここへ移したもの以外を削除した。Phase に固有の試験は各 WS の `tests/` にある。

| tool | 用途 | 使い方 |
| --- | --- | --- |
| `plan/tools/git-hooks/commit-msg` | git の commit-msg の hook（メッセージが `WIP` ちょうどでない commit を拒否、Co-Authored-By などの混入の防止、2026-10-03） | `cp plan/tools/git-hooks/commit-msg .git/hooks/ && chmod +x .git/hooks/commit-msg`（clone・作り直しの後に毎回） |
| `plan/tools/toolchain-lock.sh` | 共有の toolchain の tree（`build/llvm`・`llvm-source`・`llvm-build`・`NoctLang`）の directory を読み取り専用にして、許可の無い変更を防ぐ（BUG-096） | `lock`・`unlock`（main が許可した toolchain の変更の間だけ）・`status` |
| [boot-test.sh](tools/boot-test.sh)（`boot-test.py`） | 起動の確認。OVMF の USB（amd64）か BIOS の IDE（i386）で起動し、画面を QMP で撮って login prompt を読む | `plan/tools/boot-test.sh [IMAGE]`。`OUTPUT`（既定 `build/boot-test`）、`BOOT_TIMEOUT`、`BOOT_MODE=uefi-usb` か `bios-ide` |
| [Keiland の OS 境界 checker](tools/keiland-os-boundary/check.sh)（WS104） | 共通 source の OS include / ioctl、GPU layout の所有、install literal、libc に残る desktop header を C1〜C5、Linux/zedBSD moduleと実build membershipをL1〜L5で確認。evdev の 1 行だけを例外とする | `sh plan/tools/keiland-os-boundary/check.sh`。PASS は exit 0、違反は各項目の file:line と exit 1 |
| [Keiland launcher確認](tools/keiland-launcher/README.md) | 共通shellのruntime/env/argv/customprefix/exec signal、GPU/VTを取得しない | Python3 check.py＋launcher.in。Linux/FreeBSDで実施 |
| [FreeBSD native の検証](tools/keiland-freebsd/README.md)（WS109） | actual native header/ELF/borrowedfd、properVulkanwindow、IntelGPU/VTlease/input、主要app/PTY/fileopen。専用guest限定、mockを実GPU結果としない | READMEのnativecompile/fixture手順。SSH/QMPの操作は既存承認範囲だけ |
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
| guest の Wayland client の番号（[zwl-clients.sh](tools/guest/zwl-clients.sh)、2026-10-03 ws099-p025・BUG-146） | Venus の guest 試験で app の client の番号を compositor の log（`ZWL CLIENT`）から求める共有の helper。`zwl_app_clients`（ime=1 でない client を接続順に zc1〜zc4）、`zwl_app_client N`。keiland-ime が先に client 1 を取っても試験が外れない。`. plan/tools/guest/zwl-clients.sh` で読み込む |
| QEMU の加速と build の並列の数（[qemu-accel.sh](tools/guest/qemu-accel.sh)・[jobs.sh](tools/guest/jobs.sh)、2026-10-03 ws129-p011） | `qemu_accel_args [MODEL]`: `/dev/kvm` が使えれば `-accel kvm -cpu host`、無ければ TCG（`QEMU_NO_KVM=1` で外せる、guest.py も同じ）。user「すべてのテストで統一して、kvmを使いましょう。」。`ZEDBSD_JOBS`（既定 16、image の build の make の並列の数）。user「フルビルドは-j16にして、複数かぶってもいいようにできませんか？」 |
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
| [kbench/](tools/kbench/) | kernel の microbenchmark（system call、pipe の往復、fork、exec、cached の read、anonymous と file の fault）。kernel の build（LTO・最適化）の比較に使う。WS053 から移した | `kbench/build.sh BUILD OUTPUT`（amd64 の guest 用）で作って guest で `kbench [file]`。予熱の 1 回の後に数回走らせ、中央値で比べる。2026-10-03 `ffault.c`（file に裏付けられた page の fault の時間、BUG-027 の計測、`build.sh` の PROGRAM 引数で作る）を追加 |
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

### 試験の方針（2026-10-03 ユーザー決定）

細かい修正ごとの回帰試験はやめる。実装をレビューして確信を持ち、WS の最後にまとまった単位で試験の担当 T1 に依頼する。T1 は依頼をまとめて 1 つの QEMU で流し、長くしすぎない。負荷・耐久試験はユーザーに確かめて夜間。規則は [AGENTS.md](../AGENTS.md) の「検証」と [protocol](agents/protocol.md) の「試験の担当 T1」。

### 実機試験の進め方（2026-10-03 ユーザー決定）

2026-10-03 user「えーと、WiFi試験は、実機でやります。zedBSDの安定版を作って、実機で起動し、SSHで接続して試験を行います。1つの安定版に、複数の実機試験を詰め込みます。WiFiに限らずです。さまざまな試験をそこで行い、結果をいくつかのWSで修正して、また別な安定版を作り、そこで実機試験を詰め込みます。」
- 実機の試験は「安定版」ごとにまとめる: 安定版の image を作る → 実機で起動 → SSH で接続して、その版に詰め込んだ複数の実機試験（WiFi に限らない）を行う → 結果の不具合を各 WS で直す → 次の安定版を作り、また実機試験を詰め込む。
- 安定版ごとの実機試験は一つの WS にまとめる（最初は [WS133](ws133/ws.md)）。各 WS の「実機は未実施」の項目は、次の安定版の実機試験の候補として WS133 型の WS に集める。QEMU の仮想 WiFi（zedBSD 用の hwsim 相当）は作らない。

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
