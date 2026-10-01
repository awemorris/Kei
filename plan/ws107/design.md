# WS107 確定設計 / q541

Source: 6071c459。[全179ファイルのhash/mode/移動表](inventory.json)。165 engine files（133 C）をlibbrowserへ、main/shell/data/Makefileの14 filesをbrowserに保持。tracked distfiles無し。生成表/シェーダーは再生成せず、既存provenanceコメントを保存。

## 依存とbuild

libbrowser用LIBBROWSER_DIRとbrowser用KEILAND_BROWSER_DIRを分離する。platform/amd64のengine includeを新rootへ、shell includeはbrowserのまま。exports.map/API v2/SONAME/install/licence/package/config選択を保つ。
WS074のlist-sources/host-build/guest-buildと現役生成器の経路を更新。既存履歴・generated provenanceは当時の記録として保存。WS106移動済みのuserland/tests/browser-probeを利用。WS074 p100→p101/Acid3/CSS2のscopeと順は変更しない。
Linux production browser対応は追加しない。hostにfPICのlibbrowser.soを作り、public headerしかincludeしない第2clientとCLIを動的linkする。標準host Vulkan/lavapipeを使い、Wayland無しの実描画/ELF依存を確認。production zedBSDは既存target/toolchainを使う。

## p003の有限修正範囲 / 点検結果

1. create/resizeの0/INT_MAX超寸法、無効fetch、CPU/offscreen readのNULL/短いstride/行span overflow、GPU targetのNULL/空handle/無効寸法を拒否。既存invalid inputsは未定義/破壊可能、public API v2のstruct/signatureは保持しerrnoを文書化。
2. view_show_pageがhistory用strdup失敗前に旧page/historyを捨てる。必要なpath/historyコピーを先に確保し、失敗時は旧page/履歴を保持するtransactionへ。
3. 非同期history移動が応答前にhistory_indexを動かし、失敗時のrollbackが無い。pending_indexに行先を保持し、showのcommit時だけhistory_indexを更新。INT_MINの負号overflowもwide cast後に処理。
4. 非同期showのENOMEMを無視しているのでload failureへ報告。
5. callback契約は2026-10-02ユーザー回答「同じ view の変更・破棄は callback 後に行う契約にする」を採用。同期callbackはborrowed stringsをその間のみ使い、queryと別view操作可、同一viewのmutation/render/process/destructionは外側呼出しが戻ってから。layoutを誘発するqueryもsame-view再入として避ける。ABI v2の既存未定義範囲を明記する。shellの現callbacksを点検し適合、call中にfree/導航しない。
6. style baselineは14候補（handler.c 4、loader.c 10）。該当箇所を全文に合わせ、移動による旧実装の一括reformatは行わない。新/意味変更のviewと試験はC全文を適用し編集範囲をformat/manual review。

Cookie/CA/localStorageは同じprocess/browser profile内の共有資源で、DOM/VM/loader/timer/scroll/history/rendererはview毎。tab間の同origin cookie/storage共有をbugとして個別化しない。GPUのcaller-owned device/target/command/fenceはcompletion前に破棄しない。viewのdestroy/set_gpuはdevice idleを待ち、recordの次frame前はcaller fenceを待つ。

## 検証と上限

p002 最大60分: move全hash/mode、新registry source配分、実source/privateinclude/ELF、zedBSD engine133C新objectとbrowser7sourceをbuild warning0、現役runner syntax。
p003 最大120分: public-only動的clientの2view/入力/callback/query/deferred destroy/resize/reload/失敗時所有、allocationとVulkan呼出しのlink interposition（productionにtest環境変数無し）、実lavapipe GPU pixels/CPU reference、draw/record/caller-fence/release-targets、header standalone/NEEDED/exports。
p004 最大120分: C全文とbrowser-component全文で全WSsource/hash/diffをreview、style補助、target build/image warning0、host-view入力/Acid2/CLI/golden等関係する既存回帰、最終boot-test.sh PNG。GPU/CPU証拠と実機未実施を分離。source移動とcomponent品質以外の互換性修正へ拡張しない。

Event: ws107-q541-design。ユーザーcallback判断を記録し、全変更Phase/WS/WS074へlocal eventを届ける。GitHub publicationはdeferred。
