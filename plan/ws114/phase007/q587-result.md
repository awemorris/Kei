# q587-i01 / ws114-p007 終端結果

Outcome: **uncleared** / disposition normal。WS114はincomplete。

開始2026-10-02 07:41:58 UTC、3時間deadline 10:41:58 UTC。userの「すべてのエージェントを終了に向かわせます」「きりのいいところで作業をきりあげてもらいます」に従い、08:50:10 UTCに専用guestの正常停止を確認して終端した。未達基準の免除・Phase取消・全体acceptanceを意味しない。新guest/追加反復を開始していない。

## 承認・変更・提出

元承認: B main322d127a / q587-approved-phase SHA256 `6de8672526c942a4211b12369846a116e1049f6a38e6461cfc7a068e789a339f`。技術amendment01: main2b7ed7d4、SHA256 `30d8b4d7b2730648518f5aeb7a60d1395135c715e086d5f840dd026f9eaf766f`、seat/toplevelと最小teardown追加。元timeboxを保持。

- MR01 `831635c3`（main4b655803統合ACK）: decoration snapshot/committed mode、描画/hit/geometry一体、native opt-in、wire test。
- MR02 `19452fe8`（main6f04f1a8統合ACK）: accepted client interactive操作のorigin/button保持・release exactly once・直接move終了・teardown、focused testsとreview。
- 最終MR03: 本結果/再開/Phase・WS投影と採取済み最終evidenceのみ。製品sourceはMR02以降変更なし。提出SHAはmainへのMR receiptで保持し、commit内へ自己SHAを埋め込まない。

変更製品sourceは `userland/desktop/wayland/{decoration.c,protocol.c,shell.c,titlebar.c,seat.c,toplevel.c,objects.c,zwl.h,extras.h}`。toolchain/sysroot/checker/portal/HALのsource変更なし。

## 実装契約

objectなし/初期unset/CSDはclient_side、explicit SSDとnative keiland_titlebar_v1 opt-inはserver_side、explicit xdg CSDを優先する。configureごとにserial/mode/generation snapshotを保持し、ackが選択したmodeを次surface commitで適用する。ackA→requestB→commit（未ackB）はAを維持。destroy/native withdrawalは次commit CSD、旧snapshotをinvalidにする。描画・hit・shadow・corner・dock制御はcommitted modeを共有し、restore geometryはxdg geometryから取得する。

accepted xdg move/resizeはactual press serial・live origin/client・held initiating buttonを確認し、original surfaceとbuttonを借用保持する。matching releaseだけが操作を終了し元liveclientへexactly once＋pointer frameを届ける。別button releaseでは終了しない。window外でも届き、surface/role teardownでclearする。SSD操作にclient ownershipを作らず、lock/DnD/popup優先を維持。詳細設計/reviewは[checkpoint01](q587-checkpoint-01.md) / [checkpoint02](q587-checkpoint-02.md)。

## 実測結果と限界

| 確認 | 保存結果 / 証拠 | 限界 |
| --- | --- | --- |
| decoration wire | objectなし/初期unset/CSD/SSD/unset/destroy、ack/request/commit、複数outstanding ack、duplicate/invalid、native opt-in/CSD優先PASS。[wire](../evidence/q587/wire-final.log) / [summary](../evidence/q587/wire-final-summary.log) / [actual applied mode audit](../evidence/q587/final-wire-mode-audit.json) | 下記guest実物SHAの確認。最終source binary未導入 |
| client move/resize | GTK release state0各1回後scroll/click/key、700×460→770×500 resize settle。[move](../evidence/q587/release-move.log) / [resize](../evidence/q587/final-resize-correct.log) | 最初のresize座標試行はrequest無し。correct receiptだけを有効操作証拠にする |
| 原surface外release | bounded real SHM client300×180/max360×200、pointer1100,740でmatching release1＋frame PASS。[result](../evidence/q587/bounded-result.log) / [wire](../evidence/q587/bounded-wire.log) / [PNG](../evidence/q587/bounded-outside-hold.png) | 任意アプリ/全button組合せの網羅ではない |
| button/teardown | left開始中right releaseは終了せず、right開始moveはmatching release1＋frameで実終了。moving client kill後cleanup、新client key/scroll回復。[multiple](../evidence/q587/multiple-button-move.log) / [right](../evidence/q587/rightmove-result.log) / [destroy](../evidence/q587/destroy-moving.log) / [recovery](../evidence/q587/destroy-recovery.log) | lock/DnD優先/teardownはsource review。専用SSD phantom-release wire試験未実施 |
| GTK4 GL | 実GskGLRenderer、SSD二重表示無し、text/menu/tooltip/modal、max1280×762/full1280×800、各restore xdg700×460。[state](../evidence/q587/gl-final-state.log) / [trace](../evidence/q587/gtk-gl-final.trace) / [tooltip PNG](../evidence/q587/final-tooltip-hover.png) / [restore PNG](../evidence/q587/final-full-restore.png) | 日本語glyph不足はdistro font環境。byte転送をglyph/IME成功にしない |
| GTK4 Cairo/Vulkan | 実GskCairoRenderer/GskVulkanRenderer各1回、window/input/scroll。[Cairo](../evidence/q587/cairo-input.log) / [Vulkan](../evidence/q587/vulkan-input.log) / [trace](../evidence/q587/gtk-vulkan.trace) | CPU llvmpipe/wl_shm。物理GPU/dmabuf未試験 |
| 別client clipboard | GL→Cairo Unicode payloadの挿入は観測。[receipt](../evidence/q587/cairo-clipboard.log) | clickがreceiver source entryを指し既存文字列へ挿入。空target exact equality未達、成功扱いしない |
| native Terminal SSD | tabs追加/選択、overflow、max/dock/restore/close。[tabs](../evidence/q587/native-terminal-tabs-menu.log) / [dock PNG](../evidence/q587/native-terminal-max.png) / [close](../evidence/q587/native-terminal-close.log) | 専用phantom-release wire coverageなし |
| native Files SSD | search入力/clear、list/overflow、max/dock/restore/close。[controls](../evidence/q587/native-files-controls.log) / [dock](../evidence/q587/native-files-dock.log) / [PNG](../evidence/q587/native-files-restore.png) | 下記既導入native binaryを使用 |
| native Textedit SSD | file open/SSD表示/normal close。[PNG](../evidence/q587/native-textedit.png) / [close](../evidence/q587/native-textedit-close.log) | editing/save/controls/max/dockの回帰未実施 |

## 環境・実物provenance・build/review

Debian13.7、distro GTK4 `4.18.6+ds-2`。GCC14.2.0 Debian14.2.0-19、Python3.13.5、clang-format19.1.7、target clang23.1.0 revisiond7f1bbaca898fb5f4cc373b082e915ec1a07310f。[environment](../evidence/q587/environment.log)。Linuxはx86-64 PIE、interpreter `/lib64/ld-linux-x86-64.so.2`、RUNPATH `/opt/keiland/lib`、libkeiland/libtruetype/libpngcompat/libvulkan/libm/libcをhost file/readelfで確認。guestにfileがない初回receiptをELF合格証拠と混同しない。

| 実物 | SHA256 / 状態 |
| --- | --- |
| 最終source19452fe8 Linux wayland | `fd3b0fcf21f951c16834dcaa0173cc77991099df9f1dfbad6fea56daaec4312f`、build exit0/warnings0、**guest未導入**。[build](../evidence/q587/linux-final-build.log) / [SHA](../evidence/q587/linux-final-sha.log) |
| 最後に実測したguest wayland | `1e298dfde69e9154bde0f6836a8cde9a627df9f0f2ba650de46618dc5f99d7a9`、release/move-end/teardown動作修正を含む。後のcomment/format差分は未導入。[install](../evidence/q587/final-runtime-install.log) |
| native Terminal | `3bbbaf8fc544f18b64da44657548858bf68d6b45794a1a973762f951d3a44274` |
| native Files | `96a8a0a083954e29221fbd195d37ac9f7306a43fbcafd6a53eb72ee69892852d` |
| native Textedit | `7d62d1a3637445a5c4aad7cf9c46977d55e51a9c8a8a71e363b0f26c5ba80aa1` |
| 中間zedBSD target wayland | `69e6c8d3fe19938a34731bc8633cc1523b4a476001a133609f40ea337d32cf9b`、x86-64 PIE `/lib/ld.so`、exit0/warnings0。[build](../evidence/q587/zedbsd-release-build.log)。最終comment/format後target再build・install・boot未実施 |

Linux command: `make -j16 -f userland/desktop/keiland-linux.mk KEILAND_LINUX_BUILD=build/b1-q587/linux build/b1-q587/linux/bin/wayland build/b1-q587/linux/bin/terminal build/b1-q587/linux/bin/files build/b1-q587/linux/bin/textedit`。

zedBSD command: `make -j16 -o /home/awe/zedBSD-worktrees/b1/build/b1-q587/toolchain-cache/sysroot/.zedbsd-sysroot-complete ZEDBSD_TARGET_SYSROOT=/home/awe/zedBSD-worktrees/b1/build/b1-q587/toolchain-cache/sysroot ZEDBSD_SYSROOT_AMD64=/home/awe/zedBSD-worktrees/b1/build/b1-q587/toolchain-cache/sysroot ZEDBSD_CONFIG=plan/ws102/tests/config-amd64-inset.mk BUILD=build/b1-q587/zedbsd build/b1-q587/zedbsd/bin/wayland`。B2 private cacheをown real copyへ読取入力として複製しcurrent Vulkan external headerを使用、shared cache再生成なし。

変更7 C全文reviewは定義/宣言/prototype、argument行分け、段落/分岐/return comments、3条件以上の行分け、allocation/borrowed lifetime/teardown、mode snapshot/commit、exactly-once dispatch、native/server入力分離、OS/GPU境界を確認。clang-format19をedited rangesへ適用後、必要な引数/条件行を復元。全changed C style candidates0。[final style](../evidence/q587/full-changed-style-final.log)。WS全変更のp006最終conformanceは未実施。

- OS checker **C1–C5/L1–L5 PASS**: own config/private sysrootと `MAKEFLAGS='-o disk-image -o <private sysroot stamp>'`。recursive dryrunのdisk-image再生成をfreeze。[最終receipt](../evidence/q587/os-boundary-final.log)。
- GPU checker **actual target membership54sources PASS**: `ZEDBSD_STANDALONE_CONFIG=/home/awe/zedBSD-worktrees/b1/build/b1-q587/config.mk sh plan/tools/gpu-boundary/v1-check.sh build/b1-q587/toolchain-cache`。[receipt](../evidence/q587/gpu-boundary-membership.log)。
- earlier OS recursive remacs failure、GPU missing header/FreeBSD fallback compile failuresはそのまま原証拠保存。末尾がfixed/finalというfilenameだけでPASSを意味しない。
- raw `release-review.diff` のcontext tab/spaceはdiffそのものの資料であり全asset whitespace check警告がある。製品source/tests/docsのscoped diff-checkと分け、原証拠を保持する。全体make check/physical GPU/portal追加試験なし。

## 未達 / 再開条件

1. 最終source19452fe8のLinux実物をguestへ導入しSHA/ELFを照合、限定wire/GTK/native smokeを行う。過去runtimeのPASSでは埋めない。
2. receiverの実座標を確認した空targetへ別client Unicode pasteし、exact equalityを確認する。
3. Textedit edit/save/controls/max/dock/restore/close、native SSDのclient phantom release無しをfocused確認する。lock/DnDは必要なruntime coverageとmanual reviewを区別する。
4. 最終sourceのzedBSD target warning0再build、専用image install receipt、規定boot-testによるboot PNGと目視確認を行う。現在のLinux `boot.png` / `linux-final-console.png` はzedBSD boot証拠ではない。
5. その実物範囲のfull changed-source reviewを再照合し、p007基準をすべて満たしてからclearance判断。WS acceptance/p006を自動clearしない。

再開は[保存資産と手順](q587-resume.md)に基づく**新しいexact Queue承認**とmainのguest資源grantが必要。これはforecastであり実行投入ではない。

## 停止・引継ぎ

compositor PID1340 TERM→`ZWL EXIT frames=985 error=0 cleanup_failed=0`、PIDなし。[receipt](../evidence/q587/compositor-stop.log)。guest systemctl poweroff→QEMU PID327367 `/proc`なし/alive false/pidfileなし/SSH2249 listener false、08:50:10 UTC。[receipt](../evidence/q587/stop-proof.json) / [poweroff](../evidence/q587/poweroff.log)。host rendererは使用せず、zedBSD guestは起動していない。owned overlay/build/logを保全しmainへ枠返却済み。

Phase・WS・G05 dated follow-upを本MRで保存。Queue/共有Board/歴史/remote同期はmainへ引継ぎ、B1はpushせず終了する。p001/q581 clearedと旧baseline evidenceを保持、p007 unclearedとWS114 incompleteは別々のacceptance状態である。
