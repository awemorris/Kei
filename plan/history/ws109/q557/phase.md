<!-- awesome-plan project=zedbsd record=ws109p003 -->

# ws109p003: 共有描画と FreeBSD の device/session/input 境界

Status: uncleared
Disposition: normal
Parent: [WS109](/home/awe/zedBSD-claude1/plan/ws109/ws.md)
Queue / Attempt: q557 / q557-i01

## 目的・範囲

描画本体を再利用し、確定した FreeBSD fd/sync・VT/seat/input の OS module を実装。protocol/標準 Vulkan の契約を守る。

## 完了条件

F3。Linux source を丸ごと複製した renderer を作らない。device release/fd lifetime も確認。

## 前提・未決・実行手順

依存: p002。WS の scope と acceptance、設計の未決を確認する。
技術的な細部は委任範囲で決める。対象/受け入れ/外部契約を変える結果は実装前に計画と承認範囲へ反映する。
調査→変更表と手順の確定→有限 Queue の承認→実装→指定検証→結果・WS・Master・Queue の照合。
最初の p001 は調査の案（1 session / 最大60分、満たせない点と再開条件を残す）。後続の timebox/command は設計後に選定する。

## 適用規則・影響する部品・検証

[WS の制約/部品/受け入れ](/home/awe/zedBSD-claude1/plan/ws109/ws.md)、[Guardrail](/home/awe/zedBSD-claude1/plan/guardrail.md)、
[C 規約全文](/home/awe/zedBSD-claude1/plan/coding-style.md)、[自動化](/home/awe/zedBSD-claude1/plan/standards/automation.md)を適用。
新規/変更 C は全文該当節を読み、clang-format-19 と style-check の限界を補う。
最終 conformance は全 WS の source を全文で review。build warning 0、必要な契約検証、diff-check を記録する。
具体的な build/config/tool version と script は p001 の結果で固定する。`make check` は禁止。
zedBSD の起動は boot-test.sh の PNG。Linux の既存検証は WS105 の手順と許可範囲、FreeBSD は検証環境の確定が必要。

## 証拠・結果・再開条件

未実行。command/result/commit/artifact/skipped checks はまだ無い。計画の作成を clearance としない。
再開: prerequisite の実 output と変更の所有、scope snapshot、Queue 承認を確認する。

## イベント

2026-10-01 / review-20261001-planning: 新設した Phase 案。親 WS の目標への寄与と依存を記録。実装の選定は未実施。
GitHub の Phase 作成/comment/Project の projection は公開保留、local outbox に保持する。

## 2026-10-02 / ws109-user-decisions-20261002 / このPhaseへの反映

ユーザー「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」（2026-10-02 JST、このchat）。D1:既存FreeBSD drm-kmod利用可、GPL-free systemstack条件をこの範囲で置換。Keiland sourceの寛容license/外部実装を取り込まない境界は維持。D2:WS109専用FreeBSD QEMU guestのloopback SSH/QMP PNG検証を承認。D3:実機はユーザーが準備、入手前にnative build/backend実装を進める。実GPU/WiFi結果は将来の実機関門に残し、mock/QEMUbuildで代替しない。

shared graphics +FreeBSD OS/seat/input/backend実装を先行可。QEMUにはDRM実表示の証拠無し、realGPU/fences/device releaseのcriteriaは実機到着後にverify。partial実装Queueと全F3のclearanceを分ける。 [origin](/home/awe/zedBSD-claude1/plan/ws109/phase001/phase.md)・[WS summary](/home/awe/zedBSD-claude1/plan/ws109/ws.md)・[scope](/home/awe/zedBSD-claude1/plan/standards/ws109-native.md)。foreign Phase own検証/再開条件を更新、remote comment pending。

## q551 design reconciliation / ws109-q551-native-environment

Prerequisite is verified p002 L1 library/header output, not full F2 integration. Implement shared renderer/OS sync and native seat/evdev modules; seatd-only permissive path avoids installed libseat LGPLbasu dependency. GPU/fence/seat actual acceptance retained for physical machine. Reason: actual native dependencies and backend/link ordering inspected in p001. [Origin](/home/awe/zedBSD-claude1/plan/ws109/phase001/phase.md), [WS](/home/awe/zedBSD-claude1/plan/ws109/ws.md), [native design](/home/awe/zedBSD-claude1/plan/history/ws109/q551/environment.md). Own revised verification/resume condition saved; remote structural comment pending.

## q557 shared dma-buf/evdev/session mechanism / exact partial scope

Verified q553L1 and q556actual native service/UI/PDF output; wholeF2/F4 physical criteria
are not claimed. Maximum60min/onePhase. Reuse Linux-defined standard dma-buf protocol/Vulkan
import/render machinery by moving wayland/linux/gpu-linux.c to dmabuf/gpu-dmabuf.c;
one reservation-export operation goes behind private dmabuf/sync.h with nativeLinux/FreeBSD
sync modules using previously verified q551 fixedDRMUAPI and q553 exporter ownership convention.
No duplicate renderer. Keep identical implicit fence/ENOTTY CPU-wait fallback/CLOEXEC/errors.
This fd reservation query preserves WS105's approved dma-buf synchronization contract;
zedBSD GPUUAPI/directGPU/display ioctls remain excluded, mandatory display uses libvulkan.
Move portable Linux handoff lifecycle to session/handoff-session.c. Move Linux evdev scan,
capability/read/clock/probe mechanism to evdev/input-evdev.c, isolate seat device authority
behind small private evdev/seat.h and Linux adapters to existing direct/logind ownership.
Add the approved tiny FreeBSD native input-header selector in zwl-evdev.h; no broadOSmacros.
Native seat implementation and fullcompositor linkage are subsequent partialp003 output;
this attempt has no successful stub or physical/device authority claim.

Files: three moved mechanismC/privateheaders, wayland/{linux,freebsd}/sync OS modules,
Linux seat adapter, Makefile.linux, tiny zwl-evdev.h selector and WS109 rejected-sync
native/Linux probes/compile recipes/evidence. FullC rules for moved/changed code, no style
relocation exception. Review actual moved source against full rules and runclang-format19/
stylecheck; fix in-scope findings within timebox. Linux actual fullcompositor -j16warning0
and affected contract regression; native commonmechanism object build with real headers,
new syscall-export rejection/borrowedfd/output/errno ownership; physicalpositiveDMAfence
and actualFreeBSD seat/input/display remain F3/p005. Unsupportednativecompile prerequisites
end attempt uncleared with evidence/revised plan. No hosttoolchain/network/buildcleanup,
no push/publication, WIPcommit. FullWSconformance and nativeapps/3OS/physical gates retained.

q557 boundary tooling clarification: existing check.sh must recognize freebsd OS directories and the
shared evdev mechanism's five existing native metadata/clock request families selected by the
approved tiny input-header bridge. Scope includes required path/membership/guard-test reference
updates; no new device operation, public contract, renderer algorithm or hardware acceptance.

## Result / q557-i01 / 2026-10-01T20:40:05.930503+00:00

Queue item cleared /whole Phase uncleared。Linux共有dma-buf/evdev/sessionを再利用、FreeBSDsync/nativeinput header境界を実装。native4objects/exportownershipとLinuxfullcompositor/境界契約PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q557/result.md)。FreeBSDseat/fullcompositor/actualGPU・positiveDMA/全F3は保持。

Event ws109-q557-cleared: local evidence/outcome saved; remote comment (no Phase close) pending.
