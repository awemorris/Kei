<!-- awesome-plan project=zedbsd record=ws073-p045 -->
# ws073-p045: BUG-135 — guest で stat() などの file 操作が数秒止まる原因を解析して直す

Status: uncleared（q624-i01、P1 generation4、2026-10-03。原因を特定し直しを実装、QEMU で stat の停止が 10→2（4 回ずつ）、UFS の試験と boot test PASS。Q1 の割り込み（WiFi の試験）で中断、再開点は末尾）
Disposition: normal
Parent: [WS073](../ws.md)
Bug: [BUG-135](../../bugs/BUG-135.md)

## 範囲（Q1、2026-10-03。ユーザーの優先順位の (3) 優先度の高い bug）

BUG-135 は複数の症状の根の可能性がある: compositor の preferences の stat の 0.7〜6.5 秒（BUG-125、回避済み）、sshd の応答の stall（試験の SSH の retry）、app の起動の数秒の停止（BUG-147）、IME の確定の 500 ms 超（BUG-143、辞書の保存の fsync）。

1. QEMU の guest で再現する最小の手順を作る（stat・open・fsync の時間の分布を測る小さな probe）。
2. 止まっている間を gdbstub・QMP で捕まえ、kernel のどこで待っているか（UFS・buffer cache・journal の commit・lock・I/O の完了）を特定する（console/serial log で判定しない）。
3. 原因を直す（kernel/UFS の変更。HAL の API の変更が要るなら差分を plan に置いて止める）。
4. 直した後に probe の分布、BUG-143・BUG-147 の症状、sshd の stall が減るかを確かめる。UFS の既存の試験（`plan/tools/ufs/`）、kernel の build（warning 0）、boot-test。

## q624-i01 の結果（P1 generation4、2026-10-03 07:18〜08:30、base main `d433226dc`。Q1 の割り込みで中断）

### 再現（QEMU、本物の kernel、NVMe の root）

新しい probe [fsprobe.c](../tests/fsprobe.c)（[fsprobe-build.sh](../tests/fsprobe-build.sh) で guest 用に build）。guest の中で 2 つの書き手
（`fsync`: 16 KiB を書いて fsync、`replace`: desktop.conf を `.new` に書いて fsync して rename する preferences と同じ形）と、cache に載った
desktop.conf への `stat` を 10 ms ごとに測る。

- host の disk が空いている時は stat・fsync とも止まらない（fsync は約 20 ms）。
- **host の disk に負荷がある時に再現**（host で `dd ... bs=1M count=1000 conv=fdatasync` を繰り返す。/proc/pressure/io の some が 40〜80%）:
  guest の fsync は 3〜15 秒、replace は 13〜26 秒、**cache に載った file の stat が 1〜5.8 秒止まる**（45 秒で 1〜3 回）。同時に測った
  `nap`（10 ms の nanosleep）は 1 回も遅れない＝guest 全体が止まっているのではなく、kernel の中の待ち。
- SSH の banner の timeout も host の負荷の間に出た。その時の guest の kernel thread は全て idle（sshd の listener は poll の中）で、guest の
  kernel の待ちではなかった（host 側の QEMU の network・I/O の遅れと見られる。この Phase の直しの対象の外）。

### 原因（gdbstub で特定。DWARF と frame pointer 付きの調査用の kernel）

調査用の build: `ZEDBSD_KERNEL_LTO_CFLAGS="-g -fno-omit-frame-pointer"` で kernel だけを作り直した image（`build/p1-dbg`、commit しない）。
[p045-stacks.py](../tests/p045-stacks.py)（指定の process の thread の kernel の stack を、`asm_task_dispatch` の保存した文脈と frame pointer で辿る）と
[p045-locks.py](../tests/p045-locks.py)（UFS の mount の mutex の持ち主と、その stack）で、負荷の間に 0.5 秒ごとに標本を取った:

- stat の thread は `sys_stat_path_call → namei → inode_lookup → ufs_lookup → mutex_lock(namespace_lock)` で待つ。
- `namespace_lock`（volume 全体）の持ち主は、journal の周期の commit（`flusher → j3_hook`）か、名前の変更（`ufs_rename`・`ufs_create`）。
  j3_hook は `namespace_lock` と `ms->lock` を取ったまま `j3_commit_locked` の中で disk に書き、**device の cache の flush（`bio_flush`）を待つ**。
  名前の変更は `namespace_lock` を持ったまま `ms->lock` を待ち、`ms->lock` は fsync（`ufs_sync`）が持ったまま commit と **最後の
  `disk_sync`（flush）** を待っていた。host の flush が秒単位の時、この鎖で stat が秒単位で止まる。

### 直し（`src/drivers/fs/ufs.c`、kernel の外の API は不変）

1. **名前の検索と journal の commit が namespace を共有**: `namespace_lock` を名前の変更の排他の lock のまま残し、共有の側
   （`namespace_guard`・`namespace_readers`・`namespace_drained`）を足した。`ufs_lookup`（と、namespace を持たない呼び手の `load_inode`）と
   `j3_hook` は `namespace_share`（lock を一瞬通って数を足す）で入り、名前の変更（9 か所と orphan scan）は `namespace_enter`（lock を取り、
   共有の数が 0 になるまで待つ）で入る。変更は今までどおり単独で走り、commit とも重ならない。検索は commit の disk の書き込みと flush を待たない。
2. 共有の検索が in-core の inode を二重に作らないよう、`load_lock` で loader を 1 つずつ通す（以前は排他の namespace_lock がその役だった）。
   namespace を単独で持つ呼び手（変更の中の検索）は従来どおり。
3. `ufs_sync`（fsync）の最後の `disk_sync`（書き込みと flush）を `ms->lock` の外へ。commit は lock の中のまま。
4. `j3_hook` は lock を取る前に `buf_sync` で content を先に書く（commit が lock を持つ時間を短くする）。

### 確かめ（QEMU。実機は未実施）

- **分布**（同じ guest の手順・同じ host の負荷で、直す前（`build/p1-main`）と直した後（`build/p1-fix`）を交互に 4 回ずつ、各 45 秒）:
  直す前の stat の停止 3・1・3・3 回（最長 5.8 秒）、直した後 0・2・0・0 回（2 回の回は最長 3.8 秒）。`nap` の遅れは全て 0。
  調査用の直した kernel でも負荷の下の 3 回で 0 回。残る 2 回は、名前の変更が排他で namespace を持ったまま fsync の flush を待つ鎖
  （上の 2 つ目）で、検索がその変更を待つ形と見ている（未証明。下の再開点）。
- UFS の既存の試験（`build/p1-fix`）: `plan/tools/ufs/journal-func.sh` rc=0（FAIL 0、VERIFY-OK、3 volume UFS OK）、`crash-test.sh IMG 4 9` rc=0
  （replay の後 HOLDS=PREFIX、UFS OK）、`root-crash.sh IMG 6` rc=0（durable-content、UFS OK）。
- 新しい [p045-churn.sh](../tests/p045-churn.sh)（guest、60 秒: 4 つの検索と 2 つの変更（作成・rename・mkdir/rmdir・削除・sync）を同じ directory で）:
  hang 無く完了（各 2977 段）、残り 0。
- build: `build-settings-image.sh build/p1-fix` exit 0、warning 0。style-check: ufs.c の違反の数は直す前と同じ（579、新しい違反 0）。
- boot test: `build/p1-fix/hdd-image.img` PASS（`build/ws073-p045/boot-test/login.png`）。

### 未実施・再開点

- BUG-143（IME の確定）・BUG-147（app の起動）・sshd の stall の症状での確認は未実施。sshd の stall は guest の kernel の待ちではなかった（上）。
- 残る停止（名前の変更が namespace を持ったまま `ms->lock` を待つ）を減らすには、fsync の commit の flush を `ms->lock` の外に出す（journal の
  commit を「閉じる」段と「書いて flush する」段に分ける、二重の transaction）必要がある。規模が大きく危険もあるので、次の attempt で
  Q1 と範囲を決めてから。
- 再開の手順: `build/p1-dbg`（調査用の kernel、`ZEDBSD_KERNEL_LTO_CFLAGS="-g -fno-omit-frame-pointer"`）と `build/ws073-p045/experiment.sh`
  （worktree の build の下、commit していない: 書き手 2 つ・stat・nap を guest で、host で dd の負荷）で同じ測りを続ける。
