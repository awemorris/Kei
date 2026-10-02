# q587終端後の保存資産・再開条件

現在q587-i01/p007 **uncleared**。userの全agent終了指示で正常停止。新Queue未投入、以下は再開資料であり実行権限ではない。[結果・未達](q587-result.md)。

## 保存済みinput

- worktree `/home/awe/zedBSD-worktrees/b1`、branch `codex/b1-ws114`、製品source checkpoint19452fe8。mainへMR01/MR02統合ACK済み。最終MR03はdocs/evidenceのみ。
- Linux停止済みoverlay `/home/awe/zedBSD-worktrees/b1/build/b1-q587/guest/overlay.qcow2`。backing `/home/awe/zedBSD-worktrees/b1/build/b1-q581/overlay.qcow2` とそのq580 backing chainを保持する。移動・削除・上書きしない。
- kernel/initrd/SSH keyはread-only prior P9資産 `/home/awe/zedBSD-worktrees/p9/build/p9-q580/guest/{vmlinuz,initrd.img,id_ed25519}`。keyをevidence/repositoryへコピーしない。
- Linux final build `/home/awe/zedBSD-worktrees/b1/build/b1-q587/linux`。wayland SHA fd3b0fcf…はguest未導入。停止済みoverlay内は1e298dfd… runtime。必ず両者を区別する。
- private target sysroot `/home/awe/zedBSD-worktrees/b1/build/b1-q587/toolchain-cache/sysroot`、targetbuild `build/b1-q587/zedbsd`、ownconfig `build/b1-q587/config.mk`。shared LLVMは読取input symlink。shared toolchain/sysrootへ生成しない。
- 再利用tests: `plan/ws114/tests/{decoration-wire.py,interactive-wire.py,gtk4-baseline.py,q587-control.py}`。rawreceipt/trace/PNGは `plan/ws114/evidence/q587` にversioned保存。

## 承認後の有限再開手順

1. mainで残scope/基準と現在source差分を照合し、p007再attemptのexact Queueを承認記録とともに投入する。旧q587をactiveへ戻さず、新attempt IDを使う。guest開始前にmainの排他的QEMU grantとSSHport空きを確認する。
2. 保存overlayは読取inputとして新Queue-owned overlayへcloneし、backing chainを維持する。既存guest.sh startはsnapshot/packageを上書きし得るためq587資産へ直接実行しない。q587 startup receiptのmanual QEMU条件（KVM/q35/virtio-vga/keyboard/tablet、loopbackSSH2249、専用QMP）を新ownedruntimeへ移す。
3. 起動helper hostcopy `build/b1-q587/start-session.sh`、guest `/tmp/q587-start-session.sh` を参照。再起動時VT10使用中の場合はguest実績の `openvt -f -c 10` が必要（hostcopyは-fを持たない）。新Queue固有XDG_RUNTIME_DIR/displayへ置換する。旧runtimeは `/run/b1-q587` / `wayland-q587`、sessionenv `/tmp/q587-session.env`。
4. finalsource Linux buildを導入、guest実物SHA/ELFを確認し既存部分証拠に混ぜず新attemptに記録。限定wire・GTK4 renderer/input・native SSDの必要smokeと結果表の未達項目を有限scopeで実施する。clipboardはwindow cascade後のreceiver空entry座標を確認してからexact Unicode equalityを試す。
5. finaltarget warning0 rebuild/専用image install後、規定 `plan/tools/boot-test.sh` のPNGでzedBSD起動を確認する。Linux console/serial logをその代わりにしない。private stamp freeze/config環境とOS/GPUのactualmembership検査を継承し、旧failed receiptは履歴のまま保持する。
6. finalsource reviewとその実物の結果を保存しp007 clearanceを判断、normal停止証拠を確認しmainへ提出する。未達は具体resume条件を残しuncleared。p002他行採否/p005引継ぎ/p006全WSconformanceは独立した未達として保持する。

実行を延ばす新portal/未採用G行/physicalGPU campaignはこの残scopeに含めない。新たな発見でmaterial scopeが必要になればmainで具体amendmentを調整する。
