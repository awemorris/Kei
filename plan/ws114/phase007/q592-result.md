# q592-i01 / ws114-p007 結果（P3）

承認: 2026-10-02 ユーザー「作業を開始しましょう。」、ユーザーの決定「GTK4は、Linuxでの本物のGTK4によるCSDの動作を完成させましょう。」。Q1 が q592 として P3 に投入。範囲は [q587 の未達 1〜5](q587-result.md#未達--再開条件) だけ（[phase.md の次の attempt](phase.md#次の-attempt2026-10-02-計画担当queue-承認ではない)）。時限 3h、開始 2026-10-02 10:49:21 UTC。
worktree `/home/awe/zedBSD-worktrees/p3`、branch `agent/p3`、base 901037f9f。**製品 source の変更なし**（`userland/desktop/wayland/` の 9 file は q587 で review 済みの 19452fe8 から差分なし）。証拠は [evidence/q592](../evidence/q592/)、操作の記録は [actions.jsonl](../evidence/q592/actions.jsonl)。

## 未達 1: 最終 source の Linux 実物を guest に導入し、限定 smoke

- build: `make -j16 -f userland/desktop/keiland-linux.mk KEILAND_LINUX_BUILD=build/p3-q592/linux all` と `install DESTDIR=build/p3-q592/stage`。exit 0、warning 0、`elf-check: PASS (24 ELF)`。GCC 14.2.0 (Debian 14.2.0-19)。[linux-build.log](../evidence/q592/linux-build.log)
- wayland の SHA256 は `1eed587637933aa240dbbe2afd5b1677215d82b5fe049ca625a491f9dec0536b`（libkeiland・files などが q587 以降に変わったので q587 の fd3b0fcf… とは違う）。terminal 448d908e…、files 025b74d2…、textedit 60cc0e0b…。
- guest: q587 の停止済み overlay を `build/p3-q592/guest/base-q587.qcow2` に複写し（md5 一致、読み取り専用）、その上に overlay `q592.qcow2` を作成。KVM/q35/virtio-vga/keyboard/tablet、SSH は 127.0.0.1:10322 だけ。`install-guest.sh` で入れた後、guest の 4 binary と libkeiland の SHA が host と一致、ELF magic と ldd も確認。[guest-install.log](../evidence/q592/guest-install.log)
- decoration wire（最終 binary）: 6 PASS（object なし、mode/configure/commit 境界、native opt-in と撤回、CSD 優先、duplicate と invalid の protocol error）。[wire-summary.log](../evidence/q592/wire-summary.log) / [wire.log](../evidence/q592/wire.log) / [mode audit](../evidence/q592/wire-mode-audit.log)
- GTK4 4.18.6 GL（実際は GskGLRenderer）: SSD の二重表示なし（GTK は decoration object を作らない＝CSD）。menu → Second action、tooltip、modal（transient）、maximize 1280×762 → restore 700×460、fullscreen 1280×800 → restore 700×460、CSD move の後に button release (state 0) がちょうど 1 回。[gl-state.log](../evidence/q592/gl-state.log) / [gl-move-release.log](../evidence/q592/gl-move-release.log) / PNG（gl-initial・gl-menu・gl-tooltip・gl-modal・gl-max・gl-max-restore・gl-full・gl-full-restore・gl-move）
- Cairo（GskCairoRenderer）: click・key・paste・wheel scroll（4 回で value 101→135）・CSD の close。[cairo-scroll-close.log](../evidence/q592/cairo-scroll-close.log)
- Vulkan（GskVulkanRenderer）: key 入力「vk1」、move の後に release が 1 回、CSD resize 700×460 → 780×510（release の後の configure に resizing state が無い）、close。[vulkan-input.log](../evidence/q592/vulkan-input.log) / [vulkan-resize.log](../evidence/q592/vulkan-resize.log)
- どれも CPU llvmpipe と wl_shm。物理 GPU と dmabuf は未試験。日本語 glyph の豆腐は distro の font 環境による（q587 と同じ）。

## 未達 2: 別 client の空 entry への Unicode paste の完全一致 — 達成

GL の client A の source entry から「q592 Ω copy-ünï」を Ctrl+A/Ctrl+C。PNG で受け手の座標を確かめ、別 process の Cairo client B2 の**空の** target entry を click して Ctrl+V。B2 の `target-text` は 1 回だけ出て、その値は `q592 Ω copy-ünï` と完全に一致（EXACT）。A の trace に paste 時の `wl_data_source.send("text/plain;charset=utf-8")` がある。[clipboard.log](../evidence/q592/clipboard.log) / [verdict](../evidence/q592/clipboard-verdict.log) / [PNG 前](../evidence/q592/clip-b2-target-focused.png) / [PNG 後](../evidence/q592/clip-b2-pasted.png)
操作の誤り: 1 回目の受け手 B は、raise のつもりの click (955,240) が CSD の close button に当たって閉じた（close-request、[clip-b-closed.log](../evidence/q592/clip-b-closed.log)）。B2 を起動し直して上の結果を得た。

## 未達 3: native Textedit と SSD の phantom release — 達成

- Textedit（最終 binary）: edit（" edited q592" の入力）→ Save（file の中身を od で確認、「Saved」）→ Undo/Redo（dirty の印が付いて消える）→ Find「second」の highlight → more menu（File/Edit/View/Help）→ Open dialog → Cancel → SSD の maximize（titlebar が system bar に dock）→ dock 中の Undo/Redo → restore → SSD の close（process 終了）。[textedit-save.log](../evidence/q592/textedit-save.log) / [textedit-close.log](../evidence/q592/textedit-close.log) / PNG（textedit-*）
- restore の位置: 最大化の前は y=180、restore では y=108。`window_undock` の `zwl_glass_fit`（window を画面の中に収める、ws035-p138）による既存の動作で、p007 の回帰ではない。
- phantom release: `interactive-wire.py move`（CSD の wire client）を出したまま、Textedit の SSD move ×2、maximize/dock、dock 中の control、restore を行った。この間に wire の pointer に届いた button event は 0。その後の本物の press で move に入り、`PASS one matching release [(272, 1), (272, 0)]` と `PASS release frame`（host に保存した log でも button event はこの 2 つだけ）。[phantom2-result.log](../evidence/q592/phantom2-result.log) / [phantom2-wire.log](../evidence/q592/phantom2-wire.log)。1 回目は wire client の 90 秒の期限が先に切れた（SSD の操作中の button event は 0、[phantom-attempt1-expired.log](../evidence/q592/phantom-attempt1-expired.log)）。そのため `WS114_WIRE_DEADLINE` を足した（試験の道具だけの変更）。
- lock と DnD の優先は q587 の source review のまま。runtime では試していない。
- 最終 binary の Terminal: 入力、Shell menu、Ctrl+Shift+T の tab、maximize/dock、dock 中の tab の選択、restore、close。Files: search「edit」（4 件）、list 表示、maximize/dock、restore、close。PNG（terminal-*・files-*）、[terminal-close.log](../evidence/q592/terminal-close.log) / [files-close.log](../evidence/q592/files-close.log)

## 停止

compositor に TERM を送り `ZWL EXIT frames=2899 error=0 cleanup_failed=0`（[compositor-stop.log](../evidence/q592/compositor-stop.log)）。guest で `systemctl poweroff`。11:07:01 UTC に QEMU PID 513268 が消え、port 10322 が空いたことを確認（[stop-proof.log](../evidence/q592/stop-proof.log)）。overlay `build/p3-q592/guest/q592.qcow2` と複写した base は残している。q587 の資産（b1）は読んだだけで、変更していない。

## 未達 4: 最終 source の zedBSD target build・image・boot PNG — build は済み、boot PNG はまだ

- config: main の `config.mk` の複写から、共有の work tree が要る外部 package（libcxx remacs curl openssh ca-certificates openssl）を外し、`ZEDBSD_NOCT_ACCEL := n` にした（ws004-p051 と同じ扱い）。`build/p3-q592/config.mk`。
- command: `make -j16 ZEDBSD_CONFIG=build/p3-q592/config.mk BUILD=build/p3-q592/zedbsd ZEDBSD_LLVM_SOURCE=/home/awe/zedBSD-claude1/build/llvm-source disk-image`。共有の LLVM source は読むだけで、取得も展開もしていない。sysroot は worktree の中の `build/amd64/sysroot`。distfiles・firmware・NoctLang の tarball は main から worktree へ複写した。1 回目は sysroot ができる前に `zedbsd-target-toolchain-ready` の判定が先に走って exit 2。`sysroot-amd64` の後の 2 回目で exit 0。我々の source の warning は 0（残る 2 行は gmake の jobserver の通知と外部 Noct の upstream の source）。[zedbsd-build.log](../evidence/q592/zedbsd-build.log)
- target の wayland の SHA256 `69e6c8d3fe19938a34731bc8633cc1523b4a476001a133609f40ea337d32cf9b`（q587 の target build と同じ値。製品 source が同じなので再現した）。image の SHA256 は `b270c73d…`。
- boot-test.sh: 3 回とも FAIL（boot-test.py の QMP screendump の 30 秒 timeout）。手で同じ条件の QEMU を QMP で調べたところ、起動後約 15 秒で QEMU の main loop が 1 回 41 秒止まり、その後は応答して guest は `login:` まで達した（[手での QMP PNG](../evidence/q592/manual-qmp-login-not-boot-test.png)。boot-test の証拠ではない）。[attempts](../evidence/q592/boot-test-attempts.log) / [1 回目の最後の frame](../evidence/q592/boot-attempt1-last-frame.png)。Q1 によれば、host disk の I/O で main loop が止まる問題として P1 の q594 で修正済み（main a768b804c、tmpfs の BOOT_TEST_WORK）。修正版で再試行するまで、この基準は未達。

## 未達 5: 規約・境界・停止

- 製品 C: 9 file は 19452fe8（q587 の全文 review の対象）から差分なし。`style-check.py --summary` は total 0。新しい C は書いていない。[style-recheck.log](../evidence/q592/style-recheck.log)
- 試験の道具: `q592-control.py`・`interactive-wire.py` は py_compile OK、`q592-start-session.sh` は `sh -n` OK、`git diff --check` は 0。
- OS 境界: `MAKEFLAGS="-o disk-image <上の vars>" sh plan/tools/keiland-os-boundary/check.sh` で C1–C5・L1–L5 が PASS。image の mtime は変わっていない。[os-boundary.log](../evidence/q592/os-boundary.log)
- GPU 境界: `ZEDBSD_STANDALONE_CONFIG=build/p3-q592/config.mk sh plan/tools/gpu-boundary/v1-check.sh build/amd64` で 54 source が PASS（[gpu-boundary.log](../evidence/q592/gpu-boundary.log)）。config を渡さなかった最初の 1 回は、find の予備の経路で freebsd の source を拾って FAIL になった。原証拠として [gpu-boundary-fallback-fail.log](../evidence/q592/gpu-boundary-fallback-fail.log) を残す。
- 未実施: 集約の make check（禁止）、物理 GPU・dmabuf、portal、lock/DnD の runtime。WS 全体の最終 conformance は p006。
