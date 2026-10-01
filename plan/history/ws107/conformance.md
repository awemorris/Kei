# WS107 最終 conformance / B1〜B5

2026-10-02 JST、source commits efcc8d6f / 2770e481 / 0283edb0、q544の最終変更はpublic clientのparagraphコメントと古いDOM golden2件のみ。production codeは0283edb0のまま。

| WS criteria | Verified evidence |
| --- | --- |
| B1 | 179 tracked files、engine165の移動/旧path不在/modeを全照合。158はbytes不変、生成器locator4＋view/handler/loader3だけ差分。app14のうちMakefile所有commentのみ差分。生成表/バイナリshader未再生成、legaltext維持。 |
| B2 | Makefileのengine133C/app7C各1回、target app/probeのDT_NEEDED libbrowser.so。shell private include無し、engineをappへ再linkしない。 |
| B3 | 全133engine compiler include closure340、project uapi/shell/Wayland無し。Linux libcのsocket用system uapiは間接依存でGPU/protocolではない。sourceの直接禁止名無し。target/plain/ASan ELF39exportsが公開宣言と完全一致、Wayland NEEDED/undefined無し。public header C89/C++11 -Werror standalone PASS。 |
| B4 | public-only動的第2client plain/ASan+UBSan各83checks PASS。2view/input/timer/callback/async failure/cancel/late allocation rollback/invalid dimensions,stride,target、actual lavapipe draw/readback/caller record-fence/resize/release/survivingview。 |
| B5 | host-view59/0、Acid2 pinned 120000pixels完全一致、DOM/style/layout/paint CLI。goldens81/81（position.dom/values.domは古い期待値で、旧build/newbuild出力一致＋fixture追記と同じ差分をreviewして2goldenのみ更新）。native zedBSD Venus shell p056 status0、key/focus/scroll/canceledkey/wheel/click/history/close/noERROR。最終boot-test.sh login PNG PASS。 |

## Full rules review

AGENTS/Guardrail/C全文§1〜13/browser-component全文/WS107限定例外を適用。全移動hash/mode/全変更diffを照合、所有/ABI/dependency/評価順/借用/fallible results/cleanup/synchronizationを点検。未変更moveはユーザー承認の既存style維持、qualityの変更関数/新clientには全文。clang-format19.1.7 edited範囲＋definition引数/clause layout手動復元。q544では新clientの各file-wide variable/各初期化paragraph commentを補足。client/view/handler style-check0、全133C残9はloaderのcritical section (§5 blank after lock/before unlock) false positive。具体位置はevidence/style.txt、未解決の違反として免除したものではない。

public ABI v2/struct/signatures/SONAME/39exportsは維持。query/callback strings借用、同じview mutation/layout/draw/process/destroyは外側call後、pure query/別view操作可。shell callbacksはflags/window title/pure queryだけ、同じviewを書換えない。GPU callerはrecorded commands completion/fenceを待ち、view targetsをreleaseしてからimage/view/deviceを破棄。2viewのDOM/VM/load/history/timer/rendererは独立、cookie/CA/localStorageはprocess profile共有。

## Commands / versions / limits

- make -j16 -W <app7sources> build/amd64/dynamic/libbrowser.so build/amd64/bin/browser build/amd64/bin/browser-probe、後続最終make: WS107 target source warning0/exit0。
- BROWSER_HOST_BUILD=build/ws107-host sh plan/ws074/tests/host-build.sh plain、sh plan/tools/browser-component/run.sh plain / asan: host source warning0/exit0。GCC14.2.0、clang-format19.1.7。
- cc -M全133、nm -D/readelf -d、cc -std=c89 / c++ -std=c++11 -Werror public header、style-check.py全133+client、git diff --check。
- Acid2: 既存run-acid-tests.pyのAcid2/server/render/dump/image_resultだけを利用。WPT pin2d66b9b7998bb58c336138c178323ddee857b586。shared partial cloneは未取得objectで失敗したため、既存cacheから自分のtempへacid2/notice7filesをコピーしgit blob hashesと固定treeを照合。元cache変更無し。Acid3は今回のscope外。
- make -j16 ZEDBSD_TEST_EXTRA_FILES=<guest SSH harness+existing browser fixture pages> disk-image exit0。全imageでは既存Noct/外部OpenSSL/OpenSSH/libcxx等のwarnings343行がある。WS107 source warning0、全image warning0とは報告しない。既知外部warningsの扱いは既存BUG-100/q498記録を継承し、toolchainや無関係packageを修正しない。
- OUTPUT=build/ws107-p004/boot BOOT_TIMEOUT=180 plan/tools/boot-test.sh、PNGのloginを目視。
- GUEST_RUNTIME=build/ws107-p004/venus sh plan/ws074/tests/browser-guest.sh start IMAGE; guest.py wait; timeout180 sh plan/ws074/tests/browser-p056.sh build/ws107-p004/shell; guest.py stop。自分のguestだけ停止。古いhost executableはp056のlink位置の参照に使用、現shared client/target codeの動作は別途実検証。

make check、console/serial logによる判定、実機、production Linux browser、Acid3/CSS2/amazon改善は未実施。QEMU Venus/lavapipeと実機を混同しない。GitHub publication/comment/closure/Projectはdeferred、local outbox pending。pushなし。
