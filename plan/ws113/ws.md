<!-- awesome-plan project=zedbsd record=ws113 -->

# WS113: Keilandの外部ディスプレイ・複数画面設定

Status: incomplete
Primary Milestone: MG006
Related Milestones: MG001（GPU/API契約と回帰証拠）
Parent: [Master](../master.md)
Queue: q586 / A3（p001契約調査のみ、製品実装は後続）
Resume point: p001契約/能力20行とfixture設計を保存。D-ID/物理移動解釈等のmain reviewとoutcome判定。実装順位/次Queueは未指定。

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

[現状と順番](design.md)、[p001契約](phase001/contracts.md)、[ID/完了保証比較](phase001/identity-completion.md)、[fixture](phase001/fixtures.md)、[全文方針](../standards/ws113-display.md)、[Guardrail](../guardrail.md)、[C全文](../coding-style.md)、[automation](../standards/automation.md#ws113-multi-display-coverage-2026-10-02)。WS075/WS103/WS089はcontext。既存のSettings Display stubを新目標で拡張する。WS099の現在のデモ基準と既存順位は変更しない。
ユーザーが追加判断: pointer境界で切替、「単一display」は拡張時だけ、実機完了はまずzedBSD i915。設定の永続化/異解像度mirror/安定IDの細目はp001で技術設計する。各Phaseは有限1 Queue、今回Queueには入れない。

## Phases

| ID/link | Purpose | Goal | Status | Dependencies |
| --- | --- | --- | --- | --- |
| [ws113-p001](phase001/phase.md) | 契約・能力と実機fixture | hotplug/複数出力/拡張とmirror/Settings/窓所属の仕様を確定 | in-progress / q586 | 既存WS075/WS089/WS103の実出力を確認（context） |
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

## Event history

2026-10-02 / ws113-multidisplay-plan-20261002-created: userの追加WS指示を9Phaseへ分割し、質問への3回答を受け入れ条件/全文方針へ反映。planned/未順位/Queue none。WS089 stubの過去結果を変更せず、後続の実装先を新WSとして記録。GitHub Issue/Project公開はlocal outbox pending。

2026-10-02 / ws113-contract-design-20261002-a3-ws113: p001読取設計を契約/能力/IDとpresent保証/fixtureへ詳細化し、p002–p009の各procedure/検証/resumeとforeign eventを保存した。固定sequence/単一rdからの下層不足、EXT依存/全entry、private ID未採択、標準present_waitの限界を受け入れ材料へ反映。Phase追加/依存変更/acceptance緩和は無し。WS incomplete、p001 in-progress/q586、後続planned/Queue none。GitHub event delivery、Master/Queue/standards/agent lane投影はmain依頼pending。

2026-10-02 / ws113-technical-choice-20261002-a3-ws113: main通常技術裁量で初回全extended/internal anchor、edge snap/非重複/辺連結、退避窓自動奪回無し、active同UID peer変更、初回eDP+HDMI fixtureを採択。p001–p008へ自Phase影響とeventを記録。私有Vulkan identity拡張不採用、standard短portkeyとUUID別gateを詳細化。D-ATOMIC user回答、旧boot override互換性と現fixtureは未確認。WS acceptance/Phase依存/実装権限の削除拡張無し。main remote delivery/projection pending。
