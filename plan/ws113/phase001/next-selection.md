# WS113 p001: q592候補とcriteria mapping（選定材料）

Date: 2026-10-02 / A3 / q586-i01の文書成果。
Status: **候補のみ。q592は未投入/未承認、実装を開始していない**。standing Queue/agent approvalをmainが作成・確認してから適用する。p001契約以外の新製品scopeは追加しない。
Refs: [p002](../phase002/phase.md)、[契約](contracts.md)、[native結線](native-contract.md)、[fixture](fixtures.md)、[WS](../ws.md)。

## 1. 準備するexact scope

Candidate: q592-i01 → ws113-p002 / A3、既存Phaseの120分有限1-item Queue。
Purpose: i915のHPD→coherent inventory/sequence、connector別mode/generation/lease、2出力同時scanoutを実装・検証し、p003が標準Display全entryを公開するnative能力を渡す。

Affected roots: `src/drivers/gpu/i915/display/`と関連i915-owned display状態、必要な既存GPU core/UAPI/driver interface差分のみ。common callback/ioctl番号/ABI layoutはcandidate差分をmain/ownerがreviewした後にscope snapshotへ固定する。新HAL API、toolchain/source build、libvulkan/compositor/Settingsの製品source編集は含めない。GPU標準UUID opcode148追加もA2に不要で含めない。

Implementation boundary:

1. HPD workerで接続/modeを確定してknown connector slot/ID/genをpublish。QUERY→完全inventory→ACKのcore契約とIRQ-safe sequence/wakeを保持。
2. stable port ID/keyはPCI公開既存API+kind+DDI。name[64]のversion付きA2 keyはcanonical/length/invalid/collisionを検査。旧boot hdmi/edpはpreferred anchorで全inventoryを隠さない。
3. per-output lease/pipe/PLL/scanout/worker、対応modeだけのvalidate、切断時停止/drain/reference保持。eDP+HDMIの2claim/実同時presentを証明する。
4. [native結線](native-contract.md)のpower/lease無しfirst-pixel、有限timing observationをdriver-owned状態で提供する。counterを実装できない場合はcapability0を明記してp003がfake counterを広告しない。新HAL/APIの承認と実装を同一視しない。
5. D-ATOMICのuser回答が追加の強いcompletionを要する場合だけ、その承認済部分をsource/criteria snapshotへ取り込む。通常owner切替案/strict案を回答前に実装方針として選ばない。

## 2. 検証とp002 clearanceへの対応

| p002 criteria / p003への必要実出力 | bounded checks / evidence | 不足時 |
| --- | --- | --- |
| HPDと列挙内容一致 | H01/H03/H07: initialbaseline、coherent seq前後、plug/unplug新generation、同port ID、旧mode/lease副作用無し | attempt uncleared、failedstep/seq/connector/SHAを記録 |
| openごとのACK独立 | H02: 2openとdup、copyout失敗、QUERY S→S+1→ACK S。既存core契約を壊さない | pending capabilityと原因を残す |
| 2output同時present | H08: actual i915 eDP+HDMI独立lease、pipe/PLL allocation、両physical pattern、片方disconnectでpeer継続 | host modelのみなら実機gate未達、Phase uncleared |
| native EXT input | H09/H10: power off/onとfirst-pixel境界、finite/0timeout、generation/close/cancel、counter支持bit/未対応0 | libvulkan標準全entry広告のprerequisiteとして不足を明記。偽成功でclearしない |
| lifetime / regression | bounded host fixtureでlease close/unregister/pendingwait、format/style/full/manualと必要narrow object build、既存単一selected outputのboot/present/wait | unresolved violation/failureはresidual、aggregate make check無し |

Scope/commands/version/fixturesのexact listは実implementation選定時に固定する。host-contract replay各race1固定seed、real plug/unplugは各3cycle程度をfinite候補とし、失敗時に新証拠無しの反復を続けない。予定checkは実施済みとは書かない。

## 3. 選定前のreadiness

| prerequisite | 現在の証拠 / まだ必要なもの |
| --- | --- |
| p001 cleared | D-ID A2/boot/layout/reconnect/auth/初回fixtureはmain採択。D-ATOMICはuser回答待ち、p001 in-progress。whole-Phase依存を飛ばしてq592開始しない |
| API ownership | 既存native契約/source不足とsemantic結線は保存済み。exact common GPU/UAPI差分、callback ABIとfull rulesをmain/owner reviewし、HALに触るなら差分事前承認 |
| hardware execution | historical eDP+HDMIだけで現可用性は未確認。実行器owner、競合Queue、lock、finite操作、許可evidenceをmanifestに記録してから選定 |
| review/sync | A3 checkpoint→main ACK、p001/WS/foreign Phase eventのpending deliveryとcanonical Queue/Agent lane projectionをmainが照合 |

full p002には単一rdから複数worker/資源へ変える実装とactual i915 gateが含まれるため、120分は停止上限であり完了保証ではない。未達ならこのattemptをunclearedとしてsame Phaseにresidual/次resumeを残す。後続p003を自動開始しない。

もしmainがより小さいpartial itemを選ぶ場合は、**承認前にexact partial scope/criteriaを別snapshotへ記録**する。例えばHPD/inventoryだけを選んでも2出力/全EXT native能力のwhole-p002 clearanceを主張しない。p003依存もそのactual scoped outputだけで満たせる範囲に明示改訂する必要がある。本書はそのpartial実装を承認していない。
