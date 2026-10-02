# WS113 p001: 実i915 fixture・有限検証の設計

Date: 2026-10-02 / q586-i01 / A3。
Status: 読取調査のみ。**実機fixtureは未取得/未利用**。SSH、VFIO bind、boot、実hardware lock取得、host変更は行っていない。
Parent: [契約](contracts.md)、[能力表](source-audit.md)、[p008](../phase008/phase.md)。

## 1. sourceと過去証拠が示す範囲

| 参照 | 確認済みの歴史的事実 | 今回へ持ち越せない結論 |
| --- | --- | --- |
| [WS075](../../ws075/ws.md) / guide.md | 5330 / Alder Lake-P 8086:46a8、display v13、PCH_ADP、内部eDP 1920×1080。実i915のnative display/present/waitの既往検証 | 現fixture接続/利用権、今回hotplug通知・複数lease・2台compositor成功 |
| [dual diagnostic](../../ws031/handover/expert-reports/report-e123b-dual.md) | 2026-09-20のeDP pipe A/DPLL0 1920×1080 + HDMI pipe B/DPLL1 1280×720。20秒でA/B frame counterが進み、userの両画面観察あり | production複数outputのCLAIM/PRESENT、全mirror/extended、同時latch、今のLCD可用性 |
| [HDMI context](../../ws075/hdmi-main-output.md) / WS075 p011–p013 | JTG S123 EDID native 1920×1280@60、164.36MHz、DVI route/pipe B。passthrough実i915のexternal scanoutとuser観察の歴史証拠 | 本日同じLCD接続、内部同時表示、plug/unplug経路完成 |
| Master fg010 / WS075現状 | 後のデモはLCDを外し内部panel単独を使う方針 | 2台fixtureが現在使えるという推定 |
| `capture.c` I915_TEST_CAPTURE | 1920×1080@60のvirtual output、RAM capture。physical training/backlight/scanoutをskip | 物理HPD、vblank/first-pixel-out、2head/実latchの受け入れ |

これらはread-only context。歴史ファイルのserial dump/SSH/VFIO再bind手順を今回は実行しない。旧WSの例外承認を新Queueへ流用しない。

## 2. p008選定前のfixture記録

選定前にparentが実行器所有/利用時間と専用Queueを確認し、次のmanifestを完成する。

| 必須field | 記録内容 / 現状 |
| --- | --- |
| source/build | exact git SHA、kernel/userland/libvulkan/compositor/Settings revision、build flags。今はsource読取baseline0e68854acのみ |
| machine/GPU | 5330等のhost identity、PCI device/revision、render/display node対応、実i915 vs virtual native adapter |
| runtime | zedBSDの実GPU実行方式（bare metal / actual i915 passthrough）、boot args、desktop session/seat。QEMUのみのvirtual captureと区別 |
| physical sinks | connector名/port、EDID識別（credential無し）、native mode、cable、internal/external、hotplug操作方法。現在未確認 |
| capability | 全connected列挙、generation/event seq、pipe/PLL/plane allocation、独立claim、vblank/present completion、power/first-pixel-outの実能力 |
| occupancy | i915 single runner、適用する`/tmp/i915-hw.lock`運用、他Queueの停止/所有確認。A3は占有していない |
| evidence | 既存許可されたboot-test.shログとPNG、driver counter/port status、同時2画面user観察、必要なphysical写真等の許可/取得源 |
| bounds | testごとの有限時間、最大replug数、失敗時停止と復旧方法。host reset等は該当Queueの明示scope内だけ |

0接続を作るために内部eDPを物理的に外すことは提案しない。実fixtureで0台が安全に再現できなければ、host contract fixtureの0台証拠と実機0台未検証を別に記録する。WS自身の0台gateに必要なら未達/再開条件として残す。

## 3. p002 / p003の下層検証

下記は後続選定時に必要な実出力。本文はtestsを実行した記録ではない。

| Case | 狙い / 期待結果 | 証拠class |
| --- | --- | --- |
| H01 fresh open | initial seq1→inventory→ACK。plugを起こしていない時に偽hotplugを通知しない | host contract + actual i915 |
| H02 two opens / dup | 別open A/Bで独立POLLPRI/QUERY/ACK。A ACK後Bはready。dupは同じACK。copyout失敗でACK/observedを進めない | GPU core contract fixture、actual topology event |
| H03 ACK race | QUERY S、workerでS+1publish、ACK Sでもready。count一定のconnector交換も完全inventoryを更新 | deterministic host race + actual replug |
| H04 idle / 0 output | swapchain/lease無しでもdevice event fenceが発火。POLLIN render完了とは別。no-deviceとno-outputを分ける | independent Vulkan client + real connector操作 |
| H05 per-fence | 同じdeviceのpending fence2個、別VkDevice/process2個へ同じplug/unplugを通知。一方のwait/destroy/resetで他方を消さない | standalone Vulkan fixture + real i915 |
| H06 fence lifecycle | unsignaled resetが監視を取消さない、signal保持、fresh再登録、pending destroy/device teardown、wait-any/all・有限timeout・外部payload復帰 | host deterministic sync + actual event |
| H07 stale generation | unplug前mode/surface/leaseを再利用して副作用無し。再plug新generation、同port同ID、別port/同nameに取り違え無し | native contract + actual disconnect/reconnect |
| H08 two claims | eDP/HDMI別leaseで同時present、片方切断で片方維持。one ownerで別output plane資源を奪わない | **actual i915両画面** + counters |
| H09 extension completeness | EXT_control4entryとsurface_counter dependency、enable/procaddr gating、無counter bit0、unsupported power/first-pixelの扱い、valid pNext/allocator | ABI/dispatch fixture + actual supported operations |
| H10 present completion | armed flipとlatchを区別、対象presentID/native seqのcompleted mapping、旧generation WAIT拒否 | deterministic mapping + **actual latch/counter** |

plug stormを無限反復しない。有限例として通常plug/unplug各3cycle、worker遅延を作るmodel raceを各1seed固定にする。最終回数/seedはp002/p003選定scopeに記録する。failureを同じ手順で繰り返すだけの再試験は止め、最新状態・原因・resume条件を保存する。

## 4. compositor / Settings / 窓の実機受け入れ

| Case | 期待結果 | 必要な観察 |
| --- | --- | --- |
| D01 boot 0/1/2 | 0台はserver/watchを保持、1台描画、2台は全接続加入。初回default/overrideは採択済D-BOOTに一致 | complete snapshot + actual display、0台の未利用時は明記 |
| D02 extended | 別画面識別patternと窓を各headへ同時表示。配置左右/上下/負originをApply、UIとwl_output一致 | snapshot、per-output captures、両physical画面観察 |
| D03 unequal mirror | 1920×1080 vs fixture外部native（歴史1920×1280）など非同解像度で四隅・全contentが両方見える。aspect保持/黒帯/cursor一致 | exact mode/viewport値 + physical両画面。共通modeへ勝手に落とさない |
| D04 boundary drag | shared edgeの直前/直後（1919→1920例）、左右/上下、端、非shared edge、large delta、往復でroot+popup+shadow全部が単一owner | ownership_epoch、source/target render captures、実scanout完了証拠 |
| D05 window sizes | targetより大きい窓、fullscreen/maximized、client configure未ACK、subsurface/transient、grab offset維持/title可視 | real application tree + pointer/global/local座標 |
| D06 disconnect | idle/drag/queued present/Apply中のexternal unplug。残るoutput維持、window tree退避、stale request拒否、0台park | notifications+結果serial、physical表示、grab状態 |
| D07 reconnect/restart | 同port key/新generation、保存配置をvalidateして戻す、退避窓は採択D-REC通り。compositor/session再起動でkey保持 | persisted file（非secret）、before/after ID/mode/layout |
| D08 failures | stale snapshot、重複/欠落output、overflow、unsupported mode、資源不足、apply/rollback failure、disk permission/full、同UID/別UID/非active権限 | result code + truthful current snapshot、未適用保存無し |
| D09 Settings | mode二択、extended drag draft/Apply/cancel、mirror drag状態、外部hotplug通知、ENOTSUP server、保存失敗文言 | libkeiland-only API監査、UI screenshotとlive output |
| D10 physical single-owner | 採択D-ATOMICのgateを満たす。source erase完了前にtarget windowをpresentしない厳格案なら短いgapを観察、重複frame無し | source completed presentID/latchとtarget submit順、physical evidence。logical capturesだけで物理保証を宣言しない |

複数headのcaptureは撮影時刻/serial/epochを添える。別々の時刻の静止画だけから「同時」や「atomic」を証明しない。kernelがcaptureしたbufferと実scanout/physical観察を区別する。mirrorのrefresh同期は今回の基本案で保証しない。

## 5. evidence境界 / 検証コマンドの扱い

- p001で実施: `rg`/`sed`/`cat`によるsource・plan読取、Khronos一次ページopen、`git diff --check`、docs内link/sourceの照合。実装・build・hardware test無し。
- p002以降: relevant narrow object/userland build、formatter/style helper、API ABI/host-contract fixturesをそのPhaseのsourceとversionに結びつける。shared toolchain/HAL所有外はread-only。aggregate `make check`は実行しない。
- p008: user承認の実機実行器を使い、`boot-test.sh`は現Guardrailのlogin PNG検査scopeを守る。render outputの機能検査やphysical同時表示は別の許可された観察で記録し、古いserial dumpを黙って追加しない。
- p009: WS最終source全文をC/Guardrail/ws113-displayと照合、mandatory check/versionを記録。Linux/FreeBSDの既存単一画面build/動作維持は独立regression。今回の複数表示の実機gateはzedBSD i915。
- 環境不在/既知能力不足ではPhaseをunclearedとして不足とresume条件を残す。model/capture/QEMUのPASSは実i915受け入れの代替にならない。

## 6. 本Phaseの残判断

[contracts.md D-ATOMIC](contracts.md#10-未決と通常提案の選択材料)のuser回答が未決。D-ID A2とD-BOOT/LAYOUT/REC/AUTH/PORT通常案、旧boot preferred anchorは2026-10-02 main技術採択。current physical fixture availabilityはp008選定前のreadiness前提として未確認。p001は能力/技術契約/有限試験を提示できるが、未採択の物理移動policyを採択済みとしてclearしない。後続Phaseの開始は新Queue選定/承認とactual prerequisitesの確認後。
