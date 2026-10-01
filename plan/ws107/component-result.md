# WS107 q543 component 結果

API v2/SONAME/exports/struct layoutを保ち、create/resize/pixel stride/span/GPU target入力を検証。navigationのpath/historyのallocationをcommit前に確保し、async history_indexを到着時だけ公開、INT_MINをwide negationで扱う。同期/非同期commit失敗をload callbackへ報告。callback借用/同一view再入を外側call後へ延期する契約はユーザー判断をpublic headerと全文境界へ記録。

新public-only動的client: plain（最終timer追加前の基本検査とtimer追加後PASS）/ASan+UBSan（最終83checks、0failure、0sanitizer reports）。loopback transport失敗とcancel、2view DOM/VM/input/timer/callback独立、外側call後destroy、late strdup2点のENOMEMで旧URL/forward history/commit保持、framebuffer creation failure EIOとVkResult、actual lavapipe draw/readback/record/caller fence、CPU全pixel一致、release/recreate/resize/survivingviewを確認。

Commands: sh plan/tools/browser-component/run.sh plain / asan、make -j16 build/amd64/dynamic/libbrowser.so build/amd64/bin/browser build/amd64/bin/browser-probe、host-view PAGES 3FONT。target/host/ASan build exit0 warning0、既存host-view59checks/0failed。
Logs: build/ws107-final-{component,asan,target}.log、build/ws107-host-view.log。Compiler初回は64bit上のalways-false width比較をWerrorで検出、unsigned multiplication round-trip checkへ直して最終再検証PASS。未修正bugではない。

## 規約候補の判定

133Cの旧style14候補: handlerの条件内call1件とbrace後3件を評価順維持して修正、const tablesをprototype前へ移動。loaderのwhile intent comment1件を追加。残9は既にC全文§5のcritical section（lock後/ unlock前の空行と段落comment）の形を満たすものをstyle-checkがparagraph欠落として検出したfalse positive。同期箇所をreviewし、新しい規約違反と扱わない。新client/view/handlerはstyle-check0。全文/manual reviewと9箇所の根拠はp004で再確認。

移動不変部分はユーザーのWS107限定例外、意味変更関数/新試験は全文規約とedited-range clang-format19.1.7＋定義引数/clause layoutの復元。Wayland/GPUhardware/production Linux browser移植を追加しない。実画面のshell回帰と最終boot/全文acceptanceはp004の関門。
