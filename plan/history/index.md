<!-- awesome-plan project=zedbsd record=past-log -->

# Past Log

Last finished Queue: [q538](queue-q538.md)（ws105-p011 cleared / WS105 completed、2026-10-01）

## 最新: q538 / WS105の完了

WS105 L1〜L9と全文規約を最終source c7e8a35aで照合。Linux独立/opt build/install、Vulkan chain/WSI/KMS、root/gdm、主なapp/日本語入力、WiFi/WPA/ALSAをverified。clean gcc/clang warning0、24ELF各/331source、AST32unit0/style0、host同期/独立D-Bus、own fresh guestの実画面・VT/LogOut・fd回収PASS。clean buildのlibrary/program object変数衝突を補完、LINUX.mdと継続道具を整備。

zedBSD build/boot/GPU/forge/fence/glass/Settings/音量PASS。C1/C2/C9は元11PASS/2resizeFAIL、BUG-125へ追加してユーザーの具体的tracking/clear許可で非阻害とした。BUG-127も未修正tracking。修理・全13PASSは主張しない。旧uncleared q526/q529/q534はそのまま。

[WS105](../ws105/ws.md)、[q538の正確な結果](queue-q538.md)、[全文規約](ws105/q538/conformance.md)、[証拠SHA256](ws105/q538/evidence/SHA256SUMS)、[全attempt](ws105/index.md)。fg012達成、MG006/fg010全体は未完了。F-065のLinux分は完了、FreeBSD/design§8はdeferred。Phase/surveyをarchive後に整理、再利用試験はMaster Toolsへ。Q1/N=0、guest停止/overlay破棄、host追加package0・toolchain変更0・host /opt変更0。

GitHub body/comment/Issue close/Projectはoutbox pending、remote close/read-back未実施。commit WIP、pushなし。次Queue未選定。元harness失敗/補正と未実施のhardware/cooperative pause等はconformance/Phaseに記録。source変更後は影響範囲を再検証する。

## Queue history（直近30、古い順）

| Queue | Scope/outcome |
| --- | --- |
| [q509](queue-q509.md) | WS103 の p002（compositor の起動の問い合わせを VK_KHR_display へ、`--direct` の削除）。 |
| [q510](queue-q510.md) | WS103 の p003（libvulkan: VK_KHR_dedicated_allocation と VK_KHR_get_memory_requirements2、image の cap… |
| [q511](queue-q511.md) | WS103 の p004（compositor を dedicated の import に切り替え、`GPU_RESOURCE_IMPORT`・`GPU_RESOURCE_DESTROY` を… |
| [q512](queue-q512.md) | WS103 の p005（libvulkan の WSI が、Wayland の target の present ごとに新しい fence を作って送る）。 |
| [q513](queue-q513.md) | WS103 の p006（compositor の fence を poll だけに、`/dev/gpu0` と `--gpu` の削除、GPU の UAPI を `gpu-zedbsd.c` … |
| [q514](queue-q514.md) | WS103 の p007（規約の全文で WS の全 source の変更を見直す、回帰、5330、V4 の性能の計測）。WS103 の最後の Phase。 |
| [q515](queue-q515.md) | desktop の公開ヘッダーを libc から分離し、WS104 と WS105 の開始条件を整える。 |
| [q516](queue-q516.md) | audio の漏れを libkeiland へ（`keiland_audio_available`） |
| [q517](queue-q517.md) | libkeiland の OS の 3 file を `libkeiland/zedbsd/` へ |
| [q518](queue-q518.md) | compositor の GPU の buffer の境界を引き上げる |
| [q519](queue-q519.md) | compositor の入力の device の層を `wayland/zedbsd/input-zedbsd.c` へ |
| [q520](queue-q520.md) | compositor の session と OS の hook を zedBSD の module に |
| [q521](queue-q521.md) | install の path を `userland/desktop/paths.h` の macro に |
| [q522](queue-q522.md) | 規約の全文の見直し、境界の確かめの script、回帰 |
| [q523](queue-q523.md) | ws105-p001 cleared |
| [q524](queue-q524.md) | ws105-p002 cleared |
| [q525](queue-q525.md) | ws105-p003 cleared |
| [q526](queue-q526.md) | ws105-p004 uncleared |
| [q527](queue-q527.md) | ws105-p004 cleared |
| [q528](queue-q528.md) | ws105-p005 cleared |
| [q529](queue-q529.md) | ws105-p006 uncleared |
| [q530](queue-q530.md) | ws105-p005 cleared |
| [q531](queue-q531.md) | ws105-p006 cleared |
| [q532](queue-q532.md) | ws105-p007 cleared |
| [q533](queue-q533.md) | ws105-p008 cleared |
| [q534](queue-q534.md) | ws105-p009 uncleared |
| [q535](queue-q535.md) | ws105-p005 cleared |
| [q536](queue-q536.md) | ws105-p009 cleared |
| [q537](queue-q537.md) | ws105-p010 cleared |
| [q538](queue-q538.md) | ws105-p011 cleared |

以前の全summary/index/判断/bugリンクは[q537までのPast Log](past-log-through-q537.md)、さらに[q522まで](past-log-through-q522.md)。元承認scope/hash・attempt結果は各Queueに保持する。
