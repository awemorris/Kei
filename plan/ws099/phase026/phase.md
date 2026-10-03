<!-- awesome-plan project=zedbsd record=ws099-p026 -->
# ws099-p026: BUG-147 の残り — cursor-owner の「guest の session が途中で消える」

Status: cleared（q645、P2、2026-10-03。Q1 判定）
Disposition: normal
Parent: [WS099](../ws.md)
Bug: [BUG-147](../../bugs/BUG-147.md)

## 範囲（Q1 の q645）

BUG-147 の 2 つの形のうち p128（起動の検出）は ws099-p024 で直した。残りの cursor-owner（q609 の C9 ×5 の 1 回目）の原因を
記録から特定し、直す。

## 原因（記録の読み、QEMU は起動していない）

q609 の [run1-cursor-owner.log](../phase023/evidence/acc/run1-cursor-owner.log) の読み:

1. 最初の行が `window at 440,250`。これは cursor-owner.sh の `wx=${1:-440}; wy=${2:-250}` の既定の値で、窓の位置を読む最初の
   `guest` が何も返していない。つまり **試験の始めから guest が無かった**（途中で消えたのではない）。
2. `guest.py run` の失敗は `no guest is running (start one first)`: `GUEST_RUNTIME/session.json` が無い。session.json は
   `guest.py start` が QEMU を Popen した直後に書き、`guest.py stop` だけが消す。QEMU は `start_new_session=True` で起こすので
   `timeout` の process group の kill は届かない。
3. QMP は `FileNotFoundError`（`qmp.sock` が無い）、VNC は `ConnectionRefused`（`vnc.sock` は残り、聞く者がいない）。前の QEMU が
   SIGTERM で正常に終わると QMP の socket は消え VNC の socket は残る（今の `build/ws035-sq-run` も同じ形で、`qemu.log` の末尾は
   `terminating on signal 15 from pid … (python3)`）。

よって、前の試験（p138、PASS）の後の `criteria.sh` の `start_guest` が、前の guest を止めた後の `guest.py start` で session.json を
書く前に失敗した（2.2 GB の image の複写か、その前の段階。`timeout 180` の切れ・host の容量・IO の混み合いなど）。`start_guest` は
start の結果を `>/dev/null 2>&1` で捨て、`sleep 40` の後に確かめずに次の試験を流していた。失敗の理由そのものは出力を捨てていたため
残っていない。

## 直し（試験の側、`plan/ws099/tests/criteria.sh`）

- `start_guest SIZE NAME`: start の出力を `OUTDIR/NAME.start.log` に残す。start の後に `sleep 40`（従来どおり）、その後
  `guest.py wait --timeout 180` で SSH の応答を確かめる。start か wait が失敗したら 1 回だけ起こし直す。start の時間の上限は 180 → 300 秒。
- `run`: 2 回とも失敗したら試験を流さず、`keep_failure` で QEMU の生死・qemu.log を残し、results に
  `FAIL seconds=0 INFRA: the guest did not start (…start.log)` と書く。試験の FAIL と基盤の FAIL が区別できる。

## 確かめ

- host: `start_guest` を stub（start と wait を差し替え）で 4 通り: 正常 → 0、start が 1 回目だけ失敗 → 起こし直して 0、start が毎回失敗 → 1、
  SSH が答えない → 1。各回の start.log の内容も期待どおり。`sh -n` PASS。
- QEMU（T1 に依頼）: criteria の image で `C9_TESTS="p128 plan/ws099/tests/cursor-owner.sh" criteria.sh IMAGE OUT C9` が 2 本 PASS、
  `OUT/*.start.log` に `guest: ready`。結果は未着。

## 結果（Q1、2026-10-03）

cleared。T2-002（2026-10-03、QEMU）: criteria ebe770901 で C9 p128 PASS（47 s）・cursor-owner PASS（23 s）、start.log に `guest: ready`、`guest start 1 failed` 無し。証拠 worktrees/t2/build/t2-002/
