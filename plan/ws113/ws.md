<!-- awesome-plan project=zedbsd record=ws113 -->

# WS113: Keilandの外部ディスプレイ・複数画面設定

Status: incomplete
Primary Milestone: MG006
Related Milestones: MG001（GPU/API契約と回帰証拠）
Parent: [Master](../master.md)
Focused goal: fg019（ベータ1、2026-10-17。2026-10-02 user「複数 display は標準アプリの次」）
Queue: none（q586 / A3 は p001 の契約調査で uncleared 終端）
Resume point（2026-10-02 ベータ1の計画）: **D-ATOMIC のユーザーの回答 → p001 の新しい attempt（45 分、残りの契約の確定だけ）→ p002 → p003 → p004 → {p005 → p006, p007} → p008（5330 + 外部 display）→ p009**。下の「ベータ1 の計画」。
過去の resume: p001/q586-i01 uncleared（90分上限、D-ATOMIC未決）。契約/能力20行/fixture/次候補保存、D-ID A2等main採択済み。

## Objective / scope

zedBSD i915で外部ディスプレイの接続/切断をVulkan Display拡張からcompositorへ通知し、Settingsからlibkeiland経由でcompositor拡張を操作して複数ディスプレイを設定できるようにする。compositorとi915のGPU表示操作はlibvulkanのVulkan API/Display拡張を通す。

選べる構成は全ディスプレイを別々に使う**拡張表示**と、全ディスプレイへ同じ画面全体を出す**ミラーリング**の二択。拡張表示の配置をSettingsでドラッグ変更できる。
拡張表示の窓は常にどれか1つのディスプレイだけに見える。ドラッグ中のpointerが隣の画面に入った時点で窓全体の所属と描画先を切り替え、窓の一部を他画面へ描かない。ミラーでは同じ論理desktopの窓が全物理画面へ複製される（ユーザー回答）。

対象: i915/displayイベントと複数出力、libvulkan標準Display通知/WSI、compositor出力・窓配置、専用Wayland拡張/公開libkeiland API、Settings Displayページ、zedBSD i915実機検証。Linux/FreeBSDは既存単一画面のbuild/動作維持を確認し、複数画面の受け入れは今回対象外。別GPU、混合mirror/extended、HAL/toolchainの変更は含めない。

## Completion criteria

| ID | WS自身の受け入れ |
| --- | --- |
| D1 | i915 HPD接続/切断→libvulkan標準Vulkan Displayのhotplug通知→compositor再列挙が実i915で動き、接続済みoutputを正しく保持/解放する |
| D2 | 2台以上の接続displayで全拡張/全mirrorを切替でき、各物理出力に意図した内容が同時に表示される。1台/切断/再接続でも画面を失わず状態を再構成する |
| D3 | Settingsがlibkeiland公開APIを通してcompositor拡張へ照会/設定/変更通知を行い、配置をドラッグで変更できる。無効設定の失敗が分かる |
| D4 | 拡張では窓を1つのdisplayにだけ描き、pointerが隣画面へ入った時に窓全体を移す。境界で別画面に窓の一部を見せない。mirrorは全画面複製 |
| D5 | 最終変更全文の規約/回帰/実i915証拠を照合し、Linux/FreeBSD単一表示も維持。実行していない検証と既知制限を記録する |

## Design / standards / dependencies

[現状と順番](design.md)、[p001契約](phase001/contracts.md)、[checkpoint証拠](phase001/evidence.md)、[次選定候補](phase001/next-selection.md)、[ID/完了保証比較](phase001/identity-completion.md)、[fixture](phase001/fixtures.md)、[全文方針](../standards/ws113-display.md)、[Guardrail](../guardrail.md)、[C全文](../coding-style.md)、[automation](../standards/automation.md#ws113-multi-display-coverage-2026-10-02)。WS075/WS103/WS089はcontext。既存のSettings Display stubを新目標で拡張する。WS099の現在のデモ基準と既存順位は変更しない。
ユーザーが追加判断: pointer境界で切替、「単一display」は拡張時だけ、実機完了はまずzedBSD i915。設定の永続化/異解像度mirror/安定IDの細目はp001で技術設計する。各Phaseは有限1 Queue、今回Queueには入れない。

## Phases

| ID/link | Purpose | Goal | Status | Dependencies |
| --- | --- | --- | --- | --- |
| [ws113-p001](phase001/phase.md) | 契約・能力と実機fixture | hotplug/複数出力/拡張とmirror/Settings/窓所属の仕様を確定 | uncleared / q586-i01。次の attempt（0.75h）は D-ATOMIC の回答待ち（planning の扱い） | 既存WS075/WS089/WS103の実出力を確認（context）、D-ATOMIC |
| [ws113-p002](phase002/phase.md) | i915のHPD・複数display出力 | driverからGPU表示イベントを提供し、複数出力を同時に扱う | planned | p001 cleared/driver契約 |
| [ws113-p003](phase003/phase.md) | Vulkan Displayの列挙・通知 | libvulkanから標準Display API/拡張でhotplugと複数出力を公開 | planned | p002 cleared/実driverイベント |
| [ws113-p004](phase004/phase.md) | compositorの出力・表示モード | 全拡張または全mirrorで複数outputを描画 | planned | p003 cleared/Vulkanの実複数出力 |
| [ws113-p005](phase005/phase.md) | compositor拡張とlibkeiland | Settings用の照会/変更/通知APIを公開 | planned | p004 cleared/出力状態と適用API |
| [ws113-p006](phase006/phase.md) | Settings Displayページ | モード選択とドラッグ配置を提供 | planned | p005 cleared/libkeiland公開API |
| [ws113-p007](phase007/phase.md) | 窓の出力所属と画面間移動 | 拡張表示で窓全体を1出力にだけ表示 | planned | p004 cleared/論理座標・出力描画（p006とは独立） |
| [ws113-p008](phase008/phase.md) | 実i915の全経路受け入れ | 接続からSettings・表示・窓移動まで実機で確認 | planned | p002〜p007 cleared/実driver・API・UI・窓出力 |
| [ws113-p009](phase009/phase.md) | 最終全文規約とWS受け入れ | 全変更sourceと実証結果の最終照合 | planned | p008 cleared/最終source・実機証拠 |

Dependency graph: WS075/WS103/WS089 context → p001 → p002 → p003 → p004 → {p005→p006, p007} → p008 → p009。
p008はp002〜p007の実出力を要する。見込みは実装許可ではない。

## ベータ1 の計画（2026-10-02、fg019）

到達目標はこの WS の D1〜D5 のまま（緩めない）。ベータ1 で測る形:

| # | ベータ1 の受け入れ | 測り方 |
| --- | --- | --- |
| M1 | 5330 の eDP + HDMI の外部 display で、HDMI の抜き差し 10 回の後も両方の画面が出て compositor が落ちない（D1・D2） | p008 の実機の手順、各出力の撮影（ユーザーの写真か passthrough の capture） |
| M2 | Settings の Display の頁で全拡張 ⇔ 全mirror を 5 回切り替え、配置の drag で左右を入れ替えられる（D2・D3） | p006 の guest の手順（模擬の 2 出力）と p008 の実機 |
| M3 | 拡張で窓を 10 回画面の間へ drag し、窓の一部が他の画面に出た frame が 0（D4、採択した D-ATOMIC の判定で） | p007 の試験と p008 |
| M4 | Linux・FreeBSD の単一 display の build と起動、zedBSD の boot test、変えた source の全文規約（D5） | p009 |

目安の時間（agent、直列の最短の経路）: p001 0.75h → p002 2h → p003 2h → p004 2h → p007 2h（p005 1.5h → p006 1.5h と並列）→ p008 2h + ユーザー → p009 1.5h。合計 約 15h、最短の経路 約 12h。
標準アプリ（WS127・WS089・WS128）の次の優先なので、開始は 10/4〜10/5 の見込み。10/10 ごろの凍結（未確定）に対して余裕が小さい。

並列と衝突:
- p002（i915 driver、`src/` の GPU display）と p003（`userland/desktop/libvulkan/`）は他の WS の desktop の Phase と file が重ならず並列可。ただし WS075・WS084（i915）・WS083（Vulkan Video、libvulkan）の Phase とは重なりうる（Q1 が確かめる）。
- p004・p007 は compositor の `display.c`・`compose.c`・`shell.c`・`seat.c`・`cursor.c` を大きく変える → WS099 p020・p021、WS094 p014、WS102 p022、WS114 の compositor の Phase と同時に流さない。**WS099 p020・p021 の後に始める**のがよい。
- p005 は libkeiland（KEILAND_VERSION）と新しい protocol → WS089 p013 と直列。p006 は `settings/page-*.c` の Display → WS089 の Phase と直列。
- p008 は 5330・i915 の lock・外部 display を使う。

未決の判断:
1. **D-ATOMIC（ユーザー）**: 拡張で窓を隣の画面へ移すときの物理の保証。(a) logical owner の同時の更新で足りる（実装が最も軽い、計画エージェントの推奨）／(b) 物理の重なり無し・短い不表示を許し source の完了を待つ／(c) 2 つの head の同時の latch。[contracts.md §10](phase001/contracts.md)。
2. GPU display の UAPI に power・first-pixel・vblank の counter が無い（[native-contract](phase001/native-contract.md)）。p002・p003 で足すなら、共有の UAPI の変更の main の許可、`include/hal/hal.h` に及ぶならユーザーの差分ごとの事前承認（AGENTS.md）。
3. ベータ1 に間に合わない場合の扱い（ユーザー）: (i) 凍結の後も WS113 だけ続けて入れる、(ii) ベータ1 は単一 display のまま WS113 を後に、(iii) mirror だけ先に入れる、のどれか。
4. p008 の実機の fixture: 5330 に外部の HDMI の display をつなぐ（2026-09-29 に HDMI の touch LCD を外している）。ユーザーの準備が要る。

## Event history

2026-10-02 / ws113-multidisplay-plan-20261002-created: userの追加WS指示を9Phaseへ分割し、質問への3回答を受け入れ条件/全文方針へ反映。planned/未順位/Queue none。WS089 stubの過去結果を変更せず、後続の実装先を新WSとして記録。GitHub Issue/Project公開はlocal outbox pending。

2026-10-02 / q586-audit1-integrated: A3-001 6e34d1bb9をmain 8021bc210へ統合・ACK。[18行source/能力調査](phase001/source-audit.md)を実sourceとVulkan一次仕様へ照合。固定HPD sequence/単一出力の現状、標準拡張の全command/dependency、ID/handleの保証範囲を記録。p001 in-progress、製品/hardware変更なし、ID/scanout/fixture契約は調査継続。

2026-10-02 / ws113-contract-design-20261002-a3-ws113: p001読取設計を契約/能力/IDとpresent保証/fixtureへ詳細化し、p002–p009の各procedure/検証/resumeとforeign eventを保存した。固定sequence/単一rdからの下層不足、EXT依存/全entry、private ID未採択、標準present_waitの限界を受け入れ材料へ反映。Phase追加/依存変更/acceptance緩和は無し。WS incomplete、p001 in-progress/q586、後続planned/Queue none。GitHub event delivery、Master/Queue/standards/agent lane投影はmain依頼pending。

2026-10-02 / ws113-technical-choice-20261002-a3-ws113: main通常技術裁量で初回全extended/internal anchor、edge snap/非重複/辺連結、退避窓自動奪回無し、active同UID peer変更、初回eDP+HDMI fixtureを採択。p001–p008へ自Phase影響とeventを記録。私有Vulkan identity拡張不採用、standard短portkeyとUUID別gateを詳細化。D-ATOMIC user回答、旧boot override互換性と現fixtureは未確認。WS acceptance/Phase依存/実装権限の削除拡張無し。main remote delivery/projection pending。

2026-10-02 / ws113-local-port-id-20261002-a3-ws113: mainがA2（同machine/local PCI segment:BDF+kind+物理DDI key）をnative name→standard displayNameへ採択。37byte/NUL38例を確認、64byte上限・invalid/unknown/collision拒否・mode/capability再validate・kind/port人向けlabelを契約化。GPU UUIDquery追加と私有Vulkan拡張は必須にしない。旧boot hdmi/edpはpreferred anchorで全connected inventoryを隠さない。p001–p008の影響/foreign eventを保存。D-ATOMICのみuser回答待ち、現physical fixtureは後続readiness未確認。新API/製品実装承認へ拡大無し。main remote delivery/projection pending。

2026-10-02 / ws113-next-selection-20261002-a3-ws113: p001がq592候補/criteria mappingとcheckpoint ledgerを保存。p002追加/依存変更/実装承認は無し。D-ATOMIC回答とp001 outcome判定を待ち、次Queueはmainのexact approval/actual readinessを必要とする。同一agent sessionを保ちcheckpoint後に次指示待機。main delivery/projection pending。

2026-10-02 08:42 UTC / ws113-q586-result-20261002-a3-ws113: p001/q586-i01は90分上限でuncleared。D-ATOMIC未決が契約確定を妨げ、artifact/他技術採択/有限fixtureは保存。WS incomplete、p002–009 planned/未承認を保持。p001 resumeはD-ATOMIC採択とexact残scopeの新attempt選定。source/実機成果でclearした意味にはしない。[origin outcome](phase001/phase.md)/[ledger](phase001/evidence.md)。main outcome projection/history/remote delivery pending。同session次指示待機。

2026-10-02 / ws113-beta1-plan: fg019 の計画エージェントがベータ1 の受け入れ M1〜M4（D1〜D5 の測り方）、時間の見積もり、衝突と並列、未決の判断 4 件を追加。Phase の追加・依存の変更・受け入れの緩和は無し。Queue は未投入。
