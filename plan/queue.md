<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: q547
Last finished Queue: q546
Status: active
Cycle: q547
Approval: current user（2026-10-02 JST、このchat）「現在のQueueを完了したら、WS108を実行してください。」。Debian13/Ubuntu26.04の指定make targets、両OSのQEMU guest内でnative build/.deb導入/動作確認、既存CI/nightly release files組込み。p001〜p004のfinite1Phase Queueで実行。
Timebox: 最大60分、1Phase
Focus: fg015 / WS108、Q1/N=0。commit WIP / pushなし、remote CI/GitHub publication deferred。
Snapshot: [approved Phase](/home/awe/zedBSD-claude1/plan/history/ws108/q547/approved-phase.md)、SHA256 cae4dbfe6c9c754f430ba5048f136a53bf49d3cbee4847379a6179f06674da21

| Attempt | Phase | Exact scope | Status | Dependencies |
| --- | --- | --- | --- | --- |
| q547-i01 | ws108p003 | 既存CIの2 distro QEMU make matrix、artifact/checksum fail gate、nightly release needs/添付。既存zedBSD image/zipを維持、local同手順/構造検証。 | in-progress | ws108p002 cleared と実output（context） |

Dependency graph: ws108p002 → q547-i01。contextは実装許可ではない。
Started UTC: 2026-10-01T16:53:35.269410+00:00

## Upcoming Work Outlook

WS108の既存範囲を依存順で進める。WS106 ime-probe回答待ちは保ち、browser追加/FreeBSD対応を本Queueへ加えない。
