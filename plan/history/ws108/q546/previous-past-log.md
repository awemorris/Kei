<!-- awesome-plan project=zedbsd record=past-log -->

# Past Log

Last finished Queue: [q545](queue-q545.md)（WS108 p001 cleared）

## 最新: q545 / WS108 p001

cleared。P1 manifest/version/license/dependency/2OS native guest手順/CI release設計を固定。公式pinned image checksum両方一致、actual QEMU10.0.11/KVM cloud-init/SSH/QMP PNGでDebian13/Ubuntu26.04 amd64確認、自分のguest停止。plan/ws108/design.mdとinputs.json。

GitHub publication/outbox pending、commit WIP / pushなし。

## 前回: q544 / WS107 p004

cleared。B1〜B5 verified。全179file/165move、engine133C/app7C、39exports/C89+C++11 header/include closure340、Wayland無し。public client plain/ASan83各PASS、host-view59/0、Acid2完全一致/goldens81/81、native Venus shell status0、最終boot login PNG。C全文/限定move例外/全diff review。WS107 builds warning0; full image外部既知warnings343行は明記。plan/history/ws107/conformance.md。

GitHub publication/outbox pending、commit WIP / pushなし。

## 前回: q543 / WS107 p003

cleared。有限component修正/API v2のcallback・借用契約、2view/入力/timer/所有/async history/late allocation rollback/実標準Vulkan/caller record-fence-releaseを検証。最終ASan/UBSan client83checks PASS、target/host warning0、既存host-view59/0。14style候補を全文判定（5修正、9critical section false positive）。plan/ws107/component-result.md。

GitHub publication/outbox pending、commit WIP / pushなし。

## 前回: q542 / WS107 p002

cleared。engine165files（133C）のmove/hash/mode確認、うち生成器4filesだけlocator更新、表/shader binary未再生成。libbrowser/appのsource変数とprivate includeを分離、registry/exports/API v2/install保存。target libbrowser133fresh source＋browser7forced source/probe build exit0/warning0。host clean build exit0/warning0、public-only browser-probeのDT_NEEDEDはlibbrowser.so/libcのみ、libraryはhost標準Vulkan/libm/libcのみ（targetもWayland無し）。list-sources140C、runner syntax/diff-check PASS。

GitHub publication/outbox pending、commit WIP / pushなし。

## 前回: q541 / WS107 p001

cleared。179file移動台帳（engine165/133C、残す14）、API v2/標準Vulkan/Wayland無し境界と有限quality修正、target/host/client検証を確定。callbackの同一view変更・破棄をcall後へ延期するユーザー判断を保存。style14候補はp003で全文適合。plan/ws107/design.mdとinventory.json、全変更Phase/WSイベント。

GitHub publication/outbox pending、commit WIP / pushなし。

## 前回: q540 / WS106 p002 partial

29 package＋追加13filesの承認partial scopeはcleared。164filesのhash/mode/参照・registry保存、Linux GCC/Clang clean build/install warning0（ELF24/source331各）、zedBSD28app/POSIX/loader/image warning0、boot-test.sh login PNG確認。ime-probe2filesは非競合回答待ちで未変更。whole p002はuncleared、p003は未実行、WS106はincomplete。

[検証checkpoint](../ws106/verification-checkpoint.md)、[boot PNG](ws106/q540/evidence/login.png)、[BUG-129](../bugs/BUG-129.md)（移動前からのmenu Fonts欠落、未修正）。残る所有確認後に再開。GitHub publication/outbox pending、commit WIP / push無し。

## 前回: q539 / WS106 p001

cleared。30 package＋承認追加13files、全166 tracked fileのhash/mode/source→destinationと参照217fileを棚卸し。menu/package/config/install互換とbuild/boot手順をdesign.mdへ確定。既存styleはユーザーの限定例外を記録。ime-probeは人間作業との非競合回答まで選定から外す条件で、他29件と13filesは実行可能。証拠: plan/ws106/survey.json、programs-before.txt、style-before.txt、design.md。

GitHub publication pending、commit WIP / push無し。

## 前回: q538 / WS105の完了

WS105 L1〜L9と全文規約を最終source c7e8a35aで照合。Linux独立/opt build/install、Vulkan chain/WSI/KMS、root/gdm、主なapp/日本語入力、WiFi/WPA/ALSAをverified。clean gcc/clang warning0、24ELF各/331source、AST32unit0/style0、host同期/独立D-Bus、own fresh guestの実画面・VT/LogOut・fd回収PASS。clean buildのlibrary/program object変数衝突を補完、LINUX.mdと継続道具を整備。

zedBSD build/boot/GPU/forge/fence/glass/Settings/音量PASS。C1/C2/C9は元11PASS/2resizeFAIL、BUG-125へ追加してユーザーの具体的tracking/clear許可で非阻害とした。BUG-127も未修正tracking。修理・全13PASSは主張しない。旧uncleared q526/q529/q534はそのまま。

[WS105](../ws105/ws.md)、[q538の正確な結果](queue-q538.md)、[全文規約](ws105/q538/conformance.md)、[証拠SHA256](ws105/q538/evidence/SHA256SUMS)、[全attempt](ws105/index.md)。fg012達成、MG006/fg010全体は未完了。F-065のLinux分は完了、FreeBSD/design§8はdeferred。Phase/surveyをarchive後に整理、再利用試験はMaster Toolsへ。Q1/N=0、guest停止/overlay破棄、host追加package0・toolchain変更0・host /opt変更0。

GitHub body/comment/Issue close/Projectはoutbox pending、remote close/read-back未実施。commit WIP、pushなし。次Queue未選定。元harness失敗/補正と未実施のhardware/cooperative pause等はconformance/Phaseに記録。source変更後は影響範囲を再検証する。

## 2026-10-01 レビュー後の planning follow-up

q538 の承認/結果/WS105完了は変わらない。ユーザーの新しいレビューから [WS106](../ws106/ws.md)〜[WS109](../ws109/ws.md) を planning で新設。
対象 test/demo は30件（mview/gpudemo追加）、browser の新 source 所有/Wayland無し規則を WS074 に反映、2 distro の deb/CI と native FreeBSD15 を計画。
F-065 の FreeBSD 分は WS109 に promote。他の未指定追加scopeは deferred。既存 fg010/priority を保持、実装と Queue 開始は未実施。
[決定・関連記録](../reviews/2026-10-01-review.md)。GitHub publication/outbox pending、commit WIP、push無し。

## Queue history（直近30、古い順）

| Queue | Scope/outcome |
| --- | --- |
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
| [q539](queue-q539.md) | ws106p001 cleared |

| [q540](queue-q540.md) | ws106p002 partial cleared / whole uncleared、ime-probe回答待ち |


退避した履歴行: [WS106更新前のindex行](ws106/prior-queue-index-rows.md)。
| [q541](queue-q541.md) | WS107 p001 cleared |

| [q542](queue-q542.md) | WS107 p002 cleared |

| [q543](queue-q543.md) | WS107 p003 cleared |

| [q544](queue-q544.md) | WS107 p004 cleared |

| [q545](queue-q545.md) | WS108 p001 cleared |


以前の全summary/index/判断/bugリンクは[q537までのPast Log](past-log-through-q537.md)、さらに[q522まで](past-log-through-q522.md)。元承認scope/hash・attempt結果は各Queueに保持する。
