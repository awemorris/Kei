<!-- awesome-plan project=zedbsd record=past-log -->

<!-- awesome-plan-current:start -->
Active Queue: なし
Last finished Queue: [q537](queue-q537.md)（ws105-p010 cleared）
<!-- awesome-plan-current:end -->

# Past Log

## 最新: 2026-10-01 q537 / ws105-p010

cleared（q537-i01）。source b55b6bf0 + Unix cleanup91343cf2 WIP。p002 placeholdersをown wpa control client / Linux interface backend / ALSA controlへ置換、公開keiland.h / exports / 共通source / appは不変。private network-wpa.hをOS内共有、外部source複写なし。commandとATTACH event socketを別に所有し、request/updateはnonblockingのreply状態遷移、各deadline2秒 / service retry1秒。STATUS・scan・profile table検証、SSIDescape decode / strongest24 / exactly-one request completion、JOINはLIST→ENABLE→SELECT、saveはhexSSID / escapedkey / SAVE_CONFIG、ENABLEせずJOIN後SAVEせず。Linux radio up/downはkernel権限の実errorを返す。

gcc14.2 / clang19.1.7 warning0、28ELF（24本体+chain/display/network/audio fixtures4）、makefile-sync、331source header-check、全新C + probeのstyle-check0 / formatter19 / whitespace PASS。ALSA card/list/info/read/write/subscribeを直接使用（alsa-libなし）、Master→PCM→Speaker、min/max/channel、readback、eventのbounded drain、device再接続。feedbackはconnectedで0 / 無音。

Linux QEMU Debian13 / real mac80211_hwsim2radio + hostapd + wpa_supplicant、HDA Master raw0〜74、一般userkei audio/netdev権限。network-probe PASS（実secured AP scan、save直後に未接続、PROFILES→JOIN keiland-test / WIFI CONNECTED+kindWIFI+IPv4、disconnect、wlan0とenp0s5 / MAC / MTU / traffic、savedprofile、DNS10.0.2.3）。audio-probe PASS（available1/reachable1/device1、40%readback、amixer41%=許容±3、別process70%→fd readable / CHANGED_VOLUME /70、mute off/on、feedback0）。解析用cache値の偽装なし。fixture固定192.0.2.2はguest setupだけ、DHCPはsystemに任せる。fake wpa fallback不要。

Settings QMP操作: scanにsecured keiland-test、key欄へkeiland-pass→Join→wpa_stateCOMPLETED / Settings Connected / systembarWiFi icon。Sound slider→amixer74%、systembar slider→amixer23%（Settings rawfloor22%）、PNG目視と当チャット表示。最初の接続3秒PNGはSCAN中の一時状態、7秒後stablePNGとwpaCOMPLETEDを最終証拠とする。最後にSIGTERM frames124/error0/cleanup_failed0。再起動時openvt7がbusyで拒否された元setup logを保存し、未使用VT8で再実行（source障害ではない）。

終了時一時directory4個残留を発見（common compositorはwatch close呼出なし）。Linux/WPA内single event-loop watch registryとlibrary/process destructorで明示closeされないwatchを同じcloseへ渡す補完。新overlayでprobe2本再PASS、Settings正常closeとcompositor SIGTERM、frames58/error0/cleanup_failed0、owned Unix path0 / console復元を再確認。SIGKILL等destructorを実行しない異常終了と多thread共有は未検証・追加対応なし。

zedBSD: Linuxだけの変更なのでPhase指定どおりdisk-image build warning0 PASS、common libkeiland不変。共通回帰一式の再実行は本Phase不要、p011 finalsource全体回帰で実施する。host追加package0、host /opt未install、host画面/input未使用、toolchain変更なし。Linux guest停止 / overlay破棄。実機・realWiFi device未実施、QEMU証拠。GitHub未公開、event/intendedcloseはoutbox pending、pushなし。次は既存p011全文規約・境界L1〜L5・両OS最終回帰・install文書・WS全体acceptance。

証拠 [q537 manifest](../../history/ws105/q537/evidence/SHA256SUMS)。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `91343cf27a4f6d8a5a34c238657227b30aafa03d`、GitHub 未公開・push なし。[q537](queue-q537.md)。

## 最新: 2026-10-01 q536 / ws105-p009

cleared（q536-i01）。p005のlogind fd修復5012d324を前提に、p009 source3400a098と入力lease補正8b0c6ee4で全7基準を検証。gcc14.2 / clang19.1.7 build warning0、26ELF（24本体+2fixture）、makefile-sync、331source header-check、変更Linux C / DBus fixture style-check0 PASS。公開keiland/OS API不変、compositor DRM ioctl0、toolchain変更なし。

Linux QEMU Debian13 gdm専用guest: 自動loginはuser kei、XDG_SESSION_TYPE=wayland / ID171 / RUNTIME/run/user/1000、escaped logind session path、wallpaper / systembarをPNGで確認。HomeからTerminal起動とecho入力PASS。SwitchTo・chvtの両方で同一PID7648を維持、DRMと4evdevの5leaseすべてのPauseDevice/ResumeDevice、復帰画面 / pointer / echo switch-ok・chvt-ok PASS。kernel revokeがD-Bus通知より先に届く入力ENODEVはleaseを保持しEAGAINとする補正、後のResumeDevice fdへ交換を実測。途中の補正前検証は原ログに保持。

V11の実観測: このsystemd257の両コマンドはtype=force（SwitchToをcooperative pauseと捏造しない）。DRM revocation後のCRTC restoreにPermissionDeniedが記録されるが、quiesce/output閉鎖→resume/swapchain再生成はPASS。pause-type ACK分岐はsource確認のみ、実guest通知未実施。外部deviceの実機hotplug / systemd再起動は未実施。

自動loginを切り、QMPでkei/passwordを入力→Keiland userkei PID8493→Home LogOut→gdm greeterへ戻るPNG PASS。最初のpassword入力はUI遷移待ち不足で拒否、focus後同じpasswordで成功（元PNGとlogを保持）。gdm guest停止、overlay破棄。baseguestも最新版stageをinstallし、root KEILAND_SEAT=direct / --session --glassを起動、wallpaper / systembar、Home Terminal echo keiland-direct-ok PASS。SIGTERM frames62/error0/cleanup_failed0、console復元、guest停止。

D-Bus実production clientの独立wire fixture: byte分割、call中2signal queue、各frameのSCM_RIGHTS分離/CLOEXEC/payload、返信serial、fd所有移管、missing right / 64KiB超過 / partialheader+right EOF / ancillary17fd truncationの拒否と全fd回収 PASS。同じ5caseのASan/UBSanも全PASS。

zedBSD: disk-image warning0、OS boundary / GPU V1、dedicated-host / gpu-zedbsd-host ordinary+sanitize、boot-test loginPNG PASS。C1/C2/C9は全13PASS。forge-guest PASS（3imports / 120frame）、fence-guest PASS（600fences / 600frame / generation1）。全target-regression PASS。

証拠 [q536 manifest](../../history/ws105/q536/evidence/SHA256SUMS)。PNG目視済み、代表画面は当チャットに表示。host追加package0、host画面/入力を使用せず、host /opt installなし。QEMUと実機を区別、実機未実施。BUG-125 / BUG-127は未修正tracking、前のq532 FAILを維持。GitHub publication / close はoutbox pending、pushなし。次は既存p010 network/ALSA、その後p011全文規約とWS最終受け入れ。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `8b0c6ee4c6a26445503b67310d1bdc9f21fefe0b`、GitHub 未公開・push なし。[q536](queue-q536.md)。

## 最新: 2026-10-01 q535 / ws105-p005

cleared（q535-i01、p005のlogind fd ownership修復）。sourceは今回のKMS WIP commit。caller supplied master fdはAUTH_MAGIC magic0でcurrent-masterを確かめ、SET_MASTER不要。非masterは拒否。libraryのdirect取得だけmaster_owned=1とし、release時のDROP_MASTERもその経路だけ。borrowed seat fdのdupはcloseし、callerのfd/master権限は保つ。compositorへのDRM ioctl追加なし、public API不変。

- gcc14.2 / clang19.1.7 warning0、26ELF（24production+2fixture）/ source-sync / 331header PASS。clangformat19 / kms.cと新fixture style-check0、changed KMS ownership scopeを全文規約でreview。host DRMnone chain1MiB・interpose0 / optout・Wayland FIFO/fallback/resize/MAILBOX360frame PASS。
- guestの既存display-probeはseat fd/directの両経路で赤・緑・青全6PNGの4点一致、oldSwapchainの置換と破棄後も表示、各exit0/PASS。gdm停止直後のtty1にはgettyが無くBIOS画面を復元したので、そのPNGも残し、getty@tty1を起動後に両経路を再確認、Linux login prompt復元PNGを目視。guest-only新fixture seat-fdはnonmaster拒否、両callerfileがlive、release後のcaller master維持PASS。
- source3400a098のlogind seatを既存contextとしてgdm自動loginのuser keiでKeiland wallpaper / system barを表示、PNGを目視・提示。q534のSET_MASTER EACCES / frames0から回復した。LD_PRELOAD observerはstaged installで除去、production経路。p009のapp/VT/LogOutはこれから同じ条件で再検証する。
- [証拠](../../history/ws105/q535/evidence/SHA256SUMS)。元build/ws105-p005-logind/とbuild/ws105-p009/kms-*を保持。実機未実施、host package追加0、toolchain / target source変更0。guestはp009再開のため稼働、gdm停止。q528/q530当時の結果とq534失敗は保持。GitHub未公開、pushなし。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `5012d324296ed75de618a7975321e1dddaa9f14b`、GitHub 未公開・push なし。[q535](queue-q535.md)。

## 最新: 2026-10-01 q534 / ws105-p009

uncleared（q534-i01、未達の前提で停止）。sourceはWIP commitに保存。D-Bus AUTH EXTERNAL / UNIX_FD / Hello、bounded frame / 16FD / signal queue / method deadline5s、logind control/device/pause/resume、backend dispatch、common os_paused / old poll snapshot guard、gdm session install / guest setupを実装。gcc14.2 / clang19.1.7 build warning0、26ELF、source-sync、331header、4Linux Cのstyle-check0 PASS。

gdm専用guestを作成し、psはuser kei / XDG session、logには正しいescaped session pathを確認。しかし最初のdisplay acquireでresult=-3、frames0 error5となり自動loginが繰り返されたためgdmを停止。試験用LD_PRELOAD observerでlibraryのDRM_IOCTL_SET_MASTER（0x641e）がerrno13/EACCESと確認した。Linux drm_auth.c の drm_master_check_perm はlogindが開いた共有fdのSET_MASTERに現在processの権限を要求する。現在のkms.cは渡されたmaster fdにも無条件SET_MASTERを行い、root専用のp005試験では発見できなかった。未知の前提を発見したためこのattemptを終了し、修正をp005で選定する。p009のVT / app / LogOut / rootdirect / target回帰は未実施、clear免除なし。

証拠 original build/ws105-p009/{gcc2,clang,elf,sources,headers,session-check,drm-observe,stop-loop}.log / session-first.png（画面が出ない、受け入れFAIL）。gdm停止、Linux専用guestは修正確認のため稼働、hostにpackage追加0、host gdmは触れていない。試験observerはguest overlayだけで本番には入れない。再開条件: p005がlogindから渡されたmaster fdをSET_MASTER不要で扱えることを検証、root direct / CRTC復元の回帰PASS。その後p009同じ基準で再試行。GitHub未公開、pushなし。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `3400a098a66f8ae10875ca458b95d00aff01ac6d`、GitHub 未公開・push なし。[q534](queue-q534.md)。

## 最新: 2026-10-01 q533 / ws105-p008

Linux app の build/install、Home の9app・IME、data を検証し p008 cleared。source `ba46edf8` + Terminal guard `7dd3ad9e`（WIP）。gcc14.2 / clang19.1.7 warning0、26ELF（24 production + 2 test fixture）、source-sync / 329 header PASS、changed common source 4file style-check0。Homeの全appは起動10秒生存、Terminal echo、Files directory、Settingsページ、Text Editor文字、Image Viewer画像、PDF Viewer 2ページ、kuidemo / mviewを確認。IME Alt+Space→kanji→漢字→確定→直接入力PASS。Notesは起動/終了PASS、keyboard文字入力のみ理由つき未実施（既存手書き専用UI）。全appのcloseでchild status0、最後のpsはdesktop FilesとIMEのみ。compositor SIGTERM2310frame error0 / cleanup_failed0、guest停止。辞書SHA256、5gradient、既定wallpaperはtargetとbyte一致、host package追加0。

zedBSD: disk-image warning0、OS boundary / V1（54source）、host dedicated18 / decoder17 ordinary+sanitize、boot login PNG、C1/C2/C9 13/13、forge拒否→3import、fence600すべてgeneration1 PASS。3commonfileのtarget path stringsはWS104 q521と3/3一致。全criteria imageはTerminal guard前（startupを試験しない）；その後final sourceを含むforge imageでTerminalの10秒生存、echo keiland-zedbsd-ok表示、timeout終了（TAB count0）、compositor継続を別に確認、BUG-128 resolved。代表PNGを目視・ユーザーに提示、実機未実施。p072 / p076今回はPASSだが修理とはしない。BUG-125 / BUG-127のtrackingとq532の元のFAILは保持。

証拠: [q533 manifest](../../history/ws105/q533/evidence/SHA256SUMS)、original `build/ws105-p008/`。全guest停止。GitHub未公開、bug disposition / Phase / WS eventはoutboxで保持、pushなし。次はp009 logind / gdm。

ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `7dd3ad9e26639cb9c17517b06d0fe977927b70ca`、GitHub 未公開・push なし。[q533](queue-q533.md)。

## 最新: 2026-10-01 q532 / ws105-p007

cleared（q532）。source `2d4abde1`（WIP）。Linuxのstandard zwp_linux_dmabuf_v1 v3 server / 1-plane validation / SCM_RIGHTSの所有 / Vulkan import / commitごとのimplicit acquire syncを実装。bind・object-free hookを既存OS境界へ追加、zedBSDは空実装。wltestとmviewの独立Linux buildとmodel data、test-only dmabuf-forgeを追加。gcc14.2 / clang19.1.7 exit0・warning0、15ELF / source-sync / 135headers PASS。formatter19・新moduleのstyle-check0、該当全文規約のmanual review。Linux guest: wltest600frame exit0・3import・600acquirefence、窓内部RGB(32,96,208)、mview model（25861vertex / 37000triangle / 13texture）描画。両PNGを目視・ユーザーに提示。forge out_of_bounds6 / IMPORT_ERROR / compositor継続 PASS、5窓fd20→20、SIGTERM error0 / cleanup_failed0、guest停止済み。V5の別process dma-buf importを確認。

zedBSD: disk-image exit0・自前warning0、OS境界C1〜C5 / V1（54source）、dedicated18 / decoder17 ×ordinary/sanitize、login PNG、forge拒否後の120frame / 3import、fence600（generation1全600、62秒）PASS。C1/C2/C9の元13件は10PASS・3FAILを保持。p076のresize416（期待200）は既存[BUG-125](../../bugs/BUG-125.md)、p072の最小化直後PNGは[BUG-127](../../bugs/BUG-127.md)へ未修正trackingとして移管。ユーザーは当チャットで「move/resizeは、Linux移植と関係ないバグの可能性があるので、いったんバグリストに記載するか、既存バグチケットに追記して、先に進みましょう。clear判定に進んでいいです。また、直せそうなら直してもいいですが、時間がかかりそうなら直さなくていいです。」と具体的なclear判断を許可。両件の長い追加調査は実施しない。修理・13/13PASSとは主張しない。cursor-ownerはtitle画像だけFAIL、同じsource・imageの単独1回で全条件PASS（title57 / desktop118 / body0 / body-again0）。[BUG-118](../../bugs/BUG-118.md)に元と追試の証拠を追記、原因と発生率は未調査で、既存修正の無効化は未証明。

[証拠とSHA256manifest](../../history/ws105/q532/evidence/SHA256SUMS)。未実施:実機GPU、非同期hardware wait / FOREIGN queue ownership（design既存V3/V8/V9の制限）、tracked2件の修正。その他のp007必須条件は確認済み。GitHub publication / closeは未実施、eventはoutboxに保持。WS105はincomplete、次はp008。

ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `2d4abde19fa2837bb9b7014d1bf931542d557c21`、GitHub 未公開・push なし。[q532](queue-q532.md)。

## 最新: 2026-10-01 q531 / ws105-p006

cleared（q531-i01）。Linux の seat-direct・入力・session・wl_shm の compositor を実装・検証した。p006 source は `80eea509`、依存 KMS repair は `753b45a0`（いずれも WIP）。q529 の uncleared は元の履歴として保持する。

- gcc14.2 / clang19.1.7 build exit0 warning0、12ELF / source-sync / 126header PASS。Linux の OS module は `linux/` に分離し、compositor は表示を Vulkan だけで扱う。evdev は monotonic clock、KD/keyboard mode は個別保存・復元。全文規約と style-check を点検した（最終 WS conformance は p011）。
- Linux guest: desktop / wlshm / Home / Esc / Wiseview と pointer 2 地点の PNG を確認。bar、wallpaper、青い window の画素、cursor の差分を確認。launcher は実際の top-left click を使い、Super+Tab は Wiseview（検証手順の補正、product code は不変）。
- Home の Log Out: `ZWL SESSION logout`、`ZWL EXIT frames=1081 error=0 cleanup_failed=0`。コンソールで `kei` が入力される PNG を確認。再起動は KEILAND_SEAT 未指定・XDG_RUNTIME_DIR=/run・--socket 未指定で `/run/wayland-keiland` に READY、SIGTERM は `frames=1 error=0 cleanup_failed=0`。PNG はユーザーに表示、guest は停止し overlay を破棄。
- zedBSD: disk-image exit0 / 自前 source warning0、boot-test PASS（login PNG 確認・表示）、OS boundary C1〜C5 / v1 54source PASS、dedicated18・decode17（通常 / sanitize とも）PASS、C1/C2/C9 13/13 PASS（C2 14/14、Wiseview・復元・Files の PNG を目視）、forge-guest PASS（偽buffer拒否後も正しいclient120frame）、fence-guest PASS（600frame、600fence、全generation1、62秒）。guest停止。
- [ログ・PNG・SHA256 manifest](../../history/ws105/q531/evidence/)。Target 回帰は q529 で開始した同一 source の直列実行を q531 へ引き継いだ。q530 は Linux KMS module のみの修正で、target source / toolchain はその間変更していない。
- 実機 GPU / 物理モニタ未実施。Linux GPU client は次 p007、logind は p009、app/data は p008、network/audio は p010 の既存範囲。GitHub 未公開、outbox に証拠・event と intended close を保持、push なし。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `29cf10de919a5629b9e58387b8a5ff32c223cb76`、GitHub 未公開・push なし。[q531](queue-q531.md)。

## 最新: 2026-10-01 q530 / ws105-p005

cleared（q530-i01）。KMS completionのpollを各100ms以下、総期限5秒にした。単発timeout/EINTRはretry、総期限未完了はSURFACE_LOST、EACCES/EPERMとrevoked fdはOUT_OF_DATEを維持した。productionの試験用switchは無し。

- 旧staged implementationにtest-only flip-delay.soで1回250ms遅延＋poll result0を与え、赤→緑の後にFAIL result=-1000001004を再現した。修正後のseat fd/direct両経路は同じinjection (`requested_ms=100 delayed_ms=250 result=0`) 後に青を表示しPASS/exit0。全6色PNG各4点一致、oldSwapchain破棄後も表示、console復元、guest stop。
- gcc14.2/clang19.1.7 build exit0 warning0、12ELF/source-sync/126header PASS（p006の既存Linuxprogramを含む）。style-check0、clangformat19、変更KMS節の全文規約を点検。全WSの最終規約はp011で実施。
- host DRM=none: chain1MiB/PASS、interposebindings0/optoutPASS、Wayland FIFO/fallback/resize/MAILBOX360frameの画素/extent/import/privatewait PASS。
- q528のvkdemoとrootVT切替の結果は保持。今回変更はその単発SETCRTCの経路に影響しない。logind/revoked fdの実動作は既存p009で確認。実機GPU/物理monitor/Valgrind未実施。
- [新旧ログ・PNG・manifest](../../history/ws105/q530/evidence/)。p005のclearanceを復旧し、p006の同じ受け入れを次のQueueで再開する。q529のunclearedは履歴として保持。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `753b45a0fae9ba22d0ef6d0a7fa0f8a4698b951d`、GitHub 未公開・push なし。[q530](queue-q530.md)。

## 最新: 2026-10-01 q529 / ws105-p006

uncleared（q529-i01）。Linux compositor/direct seat/evdev/VT/handoff/GPUなしglobalと、独立Makefile・fonts・wlshmを実装した。gcc/clang exit0 warning0、12ELF / source-sync / 126header / scoped style PASS。

- guest: READY、Virtual-1 1280×800。wallpaper/bar (100,10)=#ebf9ec / (100,400)=#c5dde6。wl_shm center (640,400) background (224,235,248)→(0,0,255)、約6000frame表示。cursor差は(200,200)と(900,600)の付近。App HomeとEsc、Super+TabのWiseviewを確認。
- 手順のmeta単独でHomeという記述は実装と違った。既存c5-transitions.shと同じlauncher click (23,17) に訂正、keyboardはEsc/Super+Tabで確認。受け入れ範囲は同じ。
- WiseviewをEscで閉じる連続描画で `ZWL VULKAN_ERROR operation=submit result=-1000001004`、`ZWL FAILED site=compose_draw errno=5`、EXIT frames=6051 error5 cleanup_failed1。masterはseatのroot fdで保持。KMSが100ms総期限をOUT_OF_DATEとする実装を確認、設計は各pollの上限100msであり総期限の意図ではなかった。タイミングからpoll deadlineが原因と推定、次のp005修正Queueでbounded遅延試験により確かめる。
- Log Outは上記でcompositorが先に終了したため未達。この終了をLog Out成功とは数えない。独立したSIGTERM終了とseat未指定のSSH tty sessionではREADY→EXIT error0、chvt1文字とconsole key 'kei' PNGを確認。
- zedBSD disk-image exit0、C1〜C5 boundary、v1、dedicated/decode host、boot login PNG確認。C1/C2/C9＋forge/fenceの承認済み回帰processは進行中（exec session52085、outputs build/ws105-p006）。既存承認の検証だけを継続して証拠を保存し、再開Queueでterminal結果を確認する。これらを今PASSと扱わない。
- Linux guest stop済み、overlay廃棄。sourceはWIPに保存。mainの委任された技術判断でp005を再開し、poll1回≤100ms＋有限の総期限のKMS待機に直す。p005の修正と再検証後、同じp006を再開してLog Out・SIGTERM・keyboard・zedBSD回帰の全条件を確認する。WS105の受け入れは変更しない。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `80eea509710d51928bf14efd1246c02c80da38f4`、GitHub 未公開・push なし。[q529](queue-q529.md)。

## 最新: 2026-10-01 q528 / ws105-p005

cleared。VK_KHR_display・direct-mode・DRM acquisition の O 10 entry、KMS inquiry/dup master/saved CRTC/double dumb FIFO copy、Linux vkdemo を実装した。

- gcc 14.2 / clang 19.1.7: final build exit0、warning0。elf-check PASS（10 ELF、probe を含む）、makefile-sync PASS、header-check PASS（70 sources）。clang-format19＋定義の改行、scoped style-check（実装・probe）0件。新規 KMS source と変更部分を全文規約で手動点検した。最終全 WS conformance は p011。
- host は DRM=none、display 環境無し。vk-chain-test PASS（API1.0・llvmpipe・1MiB全word一致）、interpose PASS（backend→compat binding0・opt-out）、Wayland FIFO/fallback/resize/MAILBOX 各90 frame、360色/extent/import/private wait PASS。
- guest kernel6.12.107+deb13-amd64、Mesa25.0.7、lavapipe。seat fd duplicate（元fdをclose）とdirectの2経路で Virtual-1 / 1280×800 / 74994mHz。赤・緑・青全6PNG各4点一致、probe exit0/PASS。赤→緑で live oldSwapchain を更新・破棄し、master所有権の継承を確認した。
- vkdemo --time-ms=1000 --hold=10: 320×240、中心 #20c5b0、描画PNGを表示、VKDEMO DONE frames=1。各経路の終了後と最終chvt1はconsole文字のPNGを確認・表示。
- 途中chvt1→5秒→chvt7: direct/root はmasterを失わず、3色を完走してPASS。実際のlogind revoke/OUT_OF_DATE はp009で確認する。ioctlのEACCES/EPERM→OUT_OF_DATEと100ms上限はsourceで確認。
- 完了後 guest stop、overlay廃棄。host package追加0、target toolchain/common zedBSD source変更0。実機GPU・物理monitorのcustom mode・Valgrindは未実施。問合せ/display/mode handleはprocess-lifetime、実機hotplugの動的再列挙は範囲外。
- [ログ・PNG・sha256 manifest](../../history/ws105/q528/evidence/)。不具合残件なし。次はp006（root compositor・wl_shm・入力・VT）、前提WS104とp005を確認。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `18a983dd30b2586f56700113f2a82add518652a5`、GitHub 未公開・push なし。[q528](queue-q528.md)。

## 最新: 2026-10-01 q527 / ws105-p004

- **cleared**。q526 の旧条件が不成立だった履歴を保持し、kernel の stub に依存しない改訂検証を実施した。product API / D6 の implicit sync と CPU fallback は同じ。
- gcc 14.2.0 / clang 19.1.7 の最終 source build warning 0。`elf-check: PASS`（8 ELF）、`makefile-sync: PASS`、`header-check: PASS`（64 source）。`git diff --check` PASS。libvulkan-compat 全 C/header、変更した試験 C と generated forward.inc の style-check 合計0、ANSI 宣言・public/static順・callback storage寿命・fd ownership・error unwindを全文規約で照合。
- `timeout 120 bash plan/tools/keiland-linux/wsi-check.sh`: FIFO/fallback/resize/MAILBOX は各90 frame、client/server exit0。全360 frame の実画素がN%3の赤/緑/青と一致。resizeは320×240→400×300→320×240、MAILBOXも90frame。
- 通常3 run: IMPORT_SYNC_FILE flags=WRITE(2) がそれぞれ90回成功。private waitは各180回（acquire/reuse）、全て成功。fallback: ENOTTY(25)は最初の1回のみ、以後import試行無し。private waitは270回、CPU-before-commitが全90frameに加わり全て成功。observerは試験専用、backend DEEPBINDとproduction引数/戻り値を維持。
- raw SYNC_IOC_FILE_INFO は全runでfences=1、driver/timeline=stub、status=1、waited_ms=0。値を加工して0にせず保存。この値だけをimplicit sync成功の根拠にしない。V3: lavapipeの実際のcreated modifier=0x0、single plane、stride=1280/1600、offset=0を確認。V9はkernel importの成功と実画素で検証。
- `vk-chain-test: PASS`: staged SONAME、surface/wayland拡張有り、XCB/Xlib無し、未enableのWayland procedure=NULL、API1.0、llvmpipe、1MiB fill/copy一致。`interpose-check: PASS`、default backend-to-compat bindings=0、NO_DEEPBIND optoutもPASS。
- `make -j4 disk-image` exit0、warning0。Linux固有library/testだけの変更のためzedBSD runtime回帰はp011の全体回帰で実施。host package追加・target toolchain変更・host /opt install無し。
- swapchain destroy時にGPU資源を先に退役し、未releaseのWayland callback storageだけをsurfaceで保持する。deviceが先に破棄されてもcallback dataが残る。初回acquireより前のapplication queue retrievalを必須にしない。
- [raw evidence](../../history/ws105/q527/evidence/) と [ELF manifest](../../history/ws105/q527/manifest.sha256)を保存。valgrindはhostに無く未実施。実機GPUの非同期待ちは未実施、host lavapipeのみ。GitHub publication/remote closeはdeferred、outboxで保持。commit WIP、push無し。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `cb6a9eacf1dac1a1f6381809ba102558ffc41467`、GitHub 未公開・push なし。[q527](queue-q527.md)。

## 最新: 2026-10-01 q526 / ws105-p004

- **uncleared**: 4 種の client/server がともに exit 0、各 90 frame。FIFO・resize・MAILBOX の全画素/サイズ一致。fallback の画素/サイズは一致したが、承認済み `fences=0` は不成立。
- host kernel `6.12.101+deb13-amd64`、lavapipe。全 run の raw fences=1、driver/timeline=`stub`、status=1、waited_ms=0。既に完了した writer と空の reservation をこの観測だけでは区別できない。fences≥1 だけによる通常経路の受け入れも根拠が不足する。
- Linux 6.12 の一次資料: [dma-buf.c](https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/dma-buf/dma-buf.c) の dma_buf_export_sync_file は空の reservation に dma_fence_get_stub を補う。[dma-fence.c](https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/dma-buf/dma-fence.c) の stub は既に signal 済み。
- gcc の Linux WSI build warning 0。4 run の [観測](evidence/) を保存。clang/全規約/chain 回帰はこの試行では未実施。既存コードの partial resource cleanup と通常 acquire の初期 queue 記録を次試行で見直す。
- 再開条件: 試験だけの ioctl observer で通常 IMPORT_SYNC_FILE の成功/flags=WRITE、fallback ENOTTY 1 回・以後成功 import 0 回、CPU fence 待ち・90 frame 全画素を実測する。raw fence/timeline は加工せず残す。D6 の production 同期方式、L3 の画素/安全性、他 Phase の出力は変更しない。main の委任済み技術判断で検証の観測方法を改訂し、新 Queue snapshot に残す。
- valgrind は host に無く未実施。実機 GPU の非同期 fence 待ちは未実施、lavapipe のみ。push/GitHub 公開なし。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `e4588580404893bdc6f0affd33d68c064ecdeaa3`、GitHub 未公開・push なし。[q526](queue-q526.md)。

## 最新: 2026-10-01 q525 / ws105-p003

Linux 専用 libvulkan-compat を実装。後段への F 222 関数、I 12 関数、禁止する WSI N 51 関数を maintained TSV から生成。dispatchable handle を包まず、後段の dlsym の trampoline を使い、instance / physical-device / device / queue の ownership を保持する。mutex・pthread_once・pthread の thread-local record で publication / 再入 / lifetime を扱う。

- gcc / clang の最終 build exit 0、warning 0。8 ELF の RUNPATH / SONAME / NEEDED が PASS。libvulkan.so.1 の NEEDED は glibc の libc.so.6 だけ（libdl / pthread は glibc 2.34 以降 libc に統合）。初版の __thread の dynamic TLS による ld-linux 直接依存を pthread key に替えて、計画の依存条件を満たした。
- `vk-chain-test: PASS`: staged library の dladdr、Vulkan 1.0 instance、llvmpipe device / driver、1 MiB の fill → copy → fence、262144 word 全一致、GetProcAddr と禁止した X11 surface 名の NULL。
- 我々の library で `vulkaninfo --summary` が llvmpipe を表示。空 XDG_RUNTIME_DIR、Wayland/X を外し、host の DRM は none。
- `interpose-check: PASS`: 既定の backend → compat vk binding 0、NO_DEEPBIND opt-out の command も PASS（V1・V2 verified）。同じ SONAME の別 backend を同 process で利用できた。
- 自分自身の backend 指定と /nonexistent は両方、no backend libvulkan の診断で exit 1、timeout 124 ではない。明示した backend 選択は失敗を既定候補で隠さず、その指定を authoritative とする。
- fake backend の PLT 再入は指定の診断と exit 134。初回 fake は gcc が直接の再帰を local alias にしたため SIGSEGV、試験の extern assembler alias で PLT call を確認して直した。production の再入検出を既定のまま検証した。
- `elf-check: PASS (8 ELF)`、`makefile-sync: PASS`、`header-check: PASS (61 sources)`。system loader が export する WSI 名と TSV を照合し、未分類名 0。
- 新規 source と generated forward.inc の style-check total 0。clang-format19 後、定義引数と3条件を復元、全文を手動レビュー。sh syntax / git diff --check PASS。
- `timeout 600 make -j64 disk-image` exit 0、warning 0。zedBSD の source / toolchain の変更無し、共通 C source の runtime 回帰対象無し。host package 追加無し。

設計上の実装の補い: Layer / version の enumeration は、Phase 本文が要求する「後段が無い場合」の fallback を作るため I の表にも入れた（当初の I 10 個に2個追加）。外部の約束・依存・受け入れは変更無し。
証拠は `build/ws105-p003/`、永続の summary / manifest は `plan/history/ws105/q525/`。WSI の実装はまだ無く、Linux compositor / app / gdm / network / audio は後続。実機 GPU、古い backend の全ての組合せ、musl は未実施。push / GitHub publication は未実施。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `5dcb10992baf2ae7b13e0d9f1af61b9b3a366d1f`、GitHub 未公開・push なし。[q525](queue-q525.md)。

## 最新: 2026-10-01 q524 / ws105-p002

Linux の独立 build の土台と、8 package の `Makefile.linux`（7 shared library と install しない digest archive）を実装。仮 network / audio backend は公開 API の署名を保持し、service 不在を返す。DNS は実際の `/etc/resolv.conf` の dotted IPv4 を読む。

- gcc / clang の最終 build exit 0、`-Werror`・warning 0。host gcc 14.2.0 / clang 19.1.7、GNU Make 4.4.1。target toolchain 変更無し、host package 追加無し。
- install した 7 ELF の `RUNPATH [/opt/keiland/lib]` と SONAME / NEEDED を確認、`elf-check: PASS (7 ELF)`。digest archive は stage に無い。
- source token の比較 `makefile-sync: PASS`、system-inclusive dependency の確認 `header-check: PASS (57 sources)`。system の Wayland / EGL / GLES header 混入無し。
- `lib-smoke: PASS`（version 21、network record / unreachable state、audio unavailable、resolver reader）。
- `make keiland-linux-clean` exit 0。Linux の lib / obj / stage を消し、p001 の guest.img は保持。
- `timeout 600 make -j64 disk-image`: exit 0、warning 0。Linux Makefile は target build に include されず、zedBSD source / toolchain に変更無し。共通 C source を直していないため runtime 回帰対象無し。
- 新規 4 C file は clang-format19（ColumnLimit 0）後、定義引数・3 条件の行を全文規約に従い復元。style-check total 0、手動全文レビュー、sh syntax、`git diff --check` PASS。仮 backend の常に拒む API の最終 return は規定の errno を保持。

証拠: `build/ws105-p002/` の gcc/clang（初回・最終）log、install/clean/zedbsd log、verify.log。永続の試験 summary と source manifest は `plan/history/ws105/q524/`。libvulkan と compositor / app は後続 Phase。host の `/opt/keiland` に install していない。host 試験は `KEILAND_DRM_DEVICE=none`、Wayland/X 環境を外し、timeout 付きで実施。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `c1c9e48ea7ed636d62f3fb20e0c4179423d5a614`、GitHub 未公開・push なし。[q524](queue-q524.md)。

## 最新: 2026-10-01 q523 / ws105-p001

Debian 13 の base guest と操作の道具を作成。`guest.sh` は小さな sh 入口から Python の controller を呼ぶ（QMP JSON と座標変換を shell escaping 無しで扱う）。依存 package は既存 host にあり、host package 追加なし。

- `timeout 600 sh .../build-guest.sh`: exit 0。mmdebstrap 121.9975 秒、raw ext4 8 GiB。mesa-vulkan-drivers 25.0.7-2+deb13u1 / libvulkan1 1.4.309.0-1 / weston 14.0.2-1 / linux-image-amd64 6.12.107-1。guest kernel 6.12.107+deb13-amd64。
- `timeout 200 .../guest.sh start`: 180 秒以内に `guest: ready`。root と kei の loopback SSH 成功。kei の audio/video/input/kvm/render/netdev、`/run/user/1000` を確認。
- DRM card0 / ALSA controlC0 / event0〜5、Vulkan llvmpipe、mac80211_hwsim wlan0/wlan1、ALSA `'Master'` を確認。
- QMP PNG の login prompt を目視・ユーザーに提示。png-probe の size `1280 800`、2 点の色 `#000000`。key / click / type が QMP error 無し。
- run2 / port2226 の同時起動・SSH・停止 PASS。両 guest 停止後 overlay 無し、base image の size / mtime は一致。build の再実行は既存 image を保持。
- 追加の file 転送 / install 確認: 専用 stage の probe.txt を guest に install し、get 後 cmp 一致。guest 停止済み。host の `/opt` は変更していない。
- sh syntax、Python compile、`git diff --check` PASS。Master Tools 登録済み。共通 product code の変更無し、zedBSD 回帰対象無し。

証拠: `build/ws105-p001/`（build.log、start.log、verify.log、devices.txt、user.txt、image-before/after.txt、received.txt）。永続 PNG・版・試験 summary は `plan/history/ws105/q523/`。gdm variant の実行は p009、compositor / app の動作は後続 Phase。console / serial log は読んでいない。


ユーザー「ws105の完了をゴールにして、自走をお願いします。」により既存範囲を実行。実装 `39a0941c272ae3da5091efcba14907610ba5683b`、GitHub 未公開・push なし。[q523](queue-q523.md)。

## 最新: 2026-10-01 q522 / WS104 完了

q522 は WS104 の全文規約の最終 source の確認、OS の境界 checker、全体回帰、完了の整理を実行。ws104-p008 cleared、WS104 は A1〜A6 を確認して completed。
最終 amd64 build exit 0・自前 warning 0、全文規約の変更範囲違反 0（理由つきの保持と tool 誤検出は standards-review.md）。境界 C1〜C5 PASS、故意の uapi include は C1 FAIL / exit 1、同一に復元。GPU V1 54 source、dedicated host 18 case × ordinary / sanitize、decode host 17 case × ordinary / sanitize、forge / fence guest（600 fence、generation 1、600 frame / 64 s）PASS。compositor C1/C2/C9 は 13/13 PASS、glass p059・Notes pen/PDF・Settings host / guest 8 本・host audio 14/14・音量 p004/p005 PASS。必須 PNG は目視し、boot の login PNG をユーザーに提示した。実機・Linux compositor・他 platform は未実施。証拠と各試験の summary は plan/history/ws104/q522/。
実装 `cd48e74d110a1e504c2b87ac288d41cdc928367c`（WIP）。詳細の検証・例外・制限・exact scope は [q522](queue-q522.md) と [WS104](../ws104/ws.md)。

ユーザーの WS104 完了までの自律実行指示により q516〜q522 を依存順に実行した。q515 は別の exact Queue 承認。
Focus は fg012（WS104 → WS105）、fg010 の実機デモも保持。後続 WS105 は planned、実行は未開始。
実機 5330・他 platform・Linux compositor は未実施。commit は全て WIP、push / GitHub 公開は未実施、outbox に記録と event を保持。

## Queue history（直近 30、古い順）

| Queue | 目的（scope・attempt・結果はリンク先） |
| --- | --- |
| [q493](queue-q493.md) | Queue q493: framebuffer object と renderbuffer（ws068-p022） |
| [q494](queue-q494.md) | Queue q494: cube map（ws068-p023） |
| [q495](queue-q495.md) | Queue q495: OpenGL ES 3.0 の API（1）（ws068-p024） |
| [q496](queue-q496.md) | Queue q496: Remacs 用 host Noct の make 失敗 |
| [q497](queue-q497.md) | Queue q497: Remacs の Noct patch の再適用を防ぐ |
| [q498](queue-q498.md) | Queue q498: smoke を外し通常の make を完走する |
| [q499](queue-q499.md) | Queue q499: Windows Venusの実画面 |
| [q500](queue-q500.md) | Queue q500: WS074 Chromium比較手順 |
| [q501](queue-q501.md) | Queue q501: Amazonトップのpercentage height |
| [q502](queue-q502.md) | Queue q502: 動的に挿入された外部script |
| [q503](queue-q503.md) | Queue q503: Amazonの後続scriptが使うWeb API |
| [q504](queue-q504.md) | Queue q504: Amazon検索欄の文字の位置 |
| [q505](queue-q505.md) | Queue q505: 公開サイトの固定比較corpusと一般化修正 |
| [q506](queue-q506.md) | Queue q506: 複数の公開siteの画像・DOM比較とlayout改善 |
| [q507](queue-q507.md) | Queue q507: WS074 Acid2 exact rendering |
| [q508](queue-q508.md) | WS103（compositor を libvulkan だけにする）の調査と設計。 |
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

以前の全要約・古い Queue の index・判断・bug への参照は [q522 までの Past Log](past-log-through-q522.md) に保持。WS104 の Phase は history/ws104/q515〜q522 へ保存済み。


2026-10-01 WS105完了後の所在地: 元summary/outcomeはこのまま保持する。旧ws105/phaseNNN参照の現在所在地は[WS105全attempt索引](ws105/index.md)。新しい最終summaryは[Past Log](index.md)。
