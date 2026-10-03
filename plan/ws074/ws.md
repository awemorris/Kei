<!-- awesome-plan project=zedbsd record=ws074 -->

# WS074: zedBSD の Web ブラウザ（`userland/desktop/browser`）

<!-- awesome-plan-current:start -->
Status: incomplete
Primary Milestone: MG006
Related Milestones: MG002
Objectives: O1, O2
Parent: [Master](../master.md)
Queue: browser3 q598-i01 finished/cleared (shared ID collision with Agent A pending mapping); q597-i01 finished/uncleared at user reboot stop; prior attempts/history retained.
Resume point: p172 whole cleared with [final evidence](phase172/import/checkpoint98/README.md): 209/209 reviews/hashes, host/ASan/ABI/Acid2, final target boot and Venus native p014. WS remains incomplete; p100 now has verified prerequisite, but is planned/unselected until a new finite Queue fixes test assets/bounds. Shared planning projections and GitHub publication pending Agent A ID-collision reconciliation.
<!-- awesome-plan-current:end -->

2026-10-02 ユーザー更新: ブラウザ専任のP10枠を固定。p172の取込が実際に統合・検証された後は、p100でAcid3の100/100とpixel完全一致・fail 0を目指す。追加指示によりFile System Access API・OPFS・Interop 2025の100%・JavaScript Test262をこの専任枠に積む。p101のCSS2全件目標は保持する。最新のN=3実行指示でp172/q579をP10に投入。後続目標はp172のwhole-Phase clearance後に有限Queueへ選定する。

## デモの目標（2026-09-28 ユーザー、同日に amazon.co.jp へ変更）

**変更**: ユーザー「…amazon.co.jpに変更しましょう。」→ デモの目標は amazon.co.jp（トップの page と検索、限定的な CSS・基本的な JS、WebGL・動画なし）。以下の Google の記述は経緯として残す。

- 調査（[p067](phase067/phase.md)、2026-09-28）: 目標の版・足りない機能・Phase の列は [amazon-goal.md](amazon-goal.md)。私たちの UA に
  desktop の版が challenge なしに返り、配置は CSS だけで決まる（JS なしの Chromium でもほぼ同じ）。検索は普通の GET の form。
  **デモの順**: p032 → p068 → p061 → p069 → p035 → p060 → p062 → p070 → p071 → p037 → p072 → p027・p065 の一部。下の Google の順と
  「実行の順」より優先する。


「Googleの検索トップページと検索が、レイアウトを崩さずに表示できたら、ゴールにしましょう！限定的なCSSと、基本的なJS、WebGLなし、ビデオなしです。」
- 完了の目安: Google の検索のトップの page と検索の結果の page が、レイアウトを崩さずに表示される。検索の box に文字を入れて検索でき、結果の link を開ける。
- 範囲: 限定的な CSS、基本的な JS。WebGL・動画は無し。インタラクション: link・戻る/進む・scroll・検索の box と form への入力・JS を動かす button。
- touch の慣性の scroll 等は [WS081](../ws081/ws.md)。
- 注意（main）: Google は User-Agent・cookie（同意の画面）・JS の有無で返す HTML を変える。p031・p032 の前に、今の browser が受け取る
  HTML と足りない機能（form・CSS・JS の API）を調べる Phase を置く。
- 調査（[p059](phase059/phase.md)、2026-09-28）: 目標の版・足りない機能・Phase の列は [google-goal.md](google-goal.md)。home は私たちの UA に返る
  基本の HTML の版、結果は SearchGuard の JS の challenge を通った後の page（基本の HTML の結果はもう無い）。**デモの順**:
  p032 → p060 → p037（最小）→ p027 → p065 → p066 → p035 → p061 → p062 → p063 → p031 → p064（Google の目標の順。amazon.co.jp への変更で上の順に置き換えた）。

## 目標

2026-09-27 ユーザー:「新しいWSを作ります。Webブラウザを作成します。userland/desktop/browserです。
HTML5のレイアウトエンジンを大まかに作ったあと、JavaScript実行エンジンをあまり最適化にこだわらないで作成し、接続します。そのあと、CSSの互換性を、
標準準拠テストで100%に近づけつつ、-webkitの拡張などはChromeと実際のレンダリング結果を比較しながら100%に近づけていきます。ただし、レイアウトを
100％準拠にするのは無理だし、Chromeと100％互換にするのも無理ですので、目標値を徐々に上げるのがいいと思います。また、JavaScript実行エンジンは、
Wasm実行エンジンと共通化することで、あとでJITコンパイルを導入するときにやりやすくなると思います。libpngは直接使わず、libpng-compatを使います。
JPEGもlibjpeg-compatを作りましょう。WebPはあとで対応します。SSLは、まずはOpenSSLのライブラリでいいですが、最終的には、libsslやlibcryptoの
互換品をbase/に入れます。動画再生はまだドライバがないので後回しでいいです。音声はogg vorbisくらいなら互換ライブラリをlibvorbis-compatとかとして
作れそうですが、これも後回しでいいです。」

zdesktop（Wayland）の上で動く、zedBSD 自前の Web ブラウザ。HTML5 の parser と DOM、CSS、layout と描画、network、画像、JavaScript と
Wasm を base の中に自前で書く（外部の browser engine は取り込まない）。

## 方針（ユーザーの指示から）

1. **順序**: HTML5 の layout engine を大まかに → JavaScript の実行 engine（最適化にこだわらない）を作って接続 → CSS の互換性を上げる。
2. **CSS の互換性**: 標準の準拠試験（WPT の CSS の部分集合など）の通過率を 100% に近づける。`-webkit-` の拡張などは Chrome の実際の描画と
   比べて近づける。100% は無理なので、**目標値を段階的に上げる**（Phase ごとに通過率・一致率の目標を決める）。
3. **JavaScript と Wasm の実行 engine を共通化**（共通の中間表現・bytecode・値の表現・GC・呼び出し規約）し、後の JIT の導入を容易にする。
4. **画像**: PNG は libpng を直接使わず `libpng-compat`（WS035 p041 の範囲、WS071 p010 で作る予定）。JPEG は `libjpeg-compat` を新しく作る。
   WebP は後。
5. **TLS**: 最初は OpenSSL のライブラリ（既存の package）。最終的に base に libssl・libcrypto の互換品を入れる（別の Phase か WS）。
6. **後回し**: 動画（driver が無い）、音声（ogg vorbis なら `libvorbis-compat` で作れそう）、WebP、JIT。

## 完了の条件（p001 で確定）

- zdesktop で browser が起動し、URL を開き、HTML・CSS・画像・JavaScript を含む一般的な静的・動的な page を描いて操作できる
  （使える page の目安は [design.md](design.md) §15）。
- CSS の準拠試験と Chrome との描画の比較で、段階 M4 までの目標値（design.md §15）を満たす。M5 以降の数は後の WS か Phase で決める。
- JavaScript と Wasm が共通の実行 engine（値・GC・bytecode・interpreter・呼び出し規約、design.md §11）の上で動く。
- `libjpeg-compat` が base の library として入り、browser が使う。
- 変更した source の規約の全文の確認（最後の Phase）。

**追加の互換性目標（2026-10-02 ユーザー）**: origin/browser2の統合後、Acid3の100/100に加え、固定条件の参照画像と出力画像をpixel単位で完全一致させ、fail・crash・timeoutを0にする。[File System Access API](phase174/phase.md)と[OPFS](phase175/phase.md)を実装する。Interop 2025の公式focus areaに属するWPTを固定commitと選定manifestで測り、全対象テストのPASS 100%、fail・error・crash・timeout・未説明のskipを0にする。調査項目はfocus areaの得点対象と混同しない。JavaScriptの[Test262](phase176/phase.md)を全suiteで計測し段階的に改善する（今回の指示で最終達成率は未指定）。p173でInteropの対象分母と実行環境を確定するまでは達成を主張しない。既存のWS受け入れとp101 CSS2目標を削除しない。

## 制約と依存

- 新しい code は `plan/coding-style.md` の全文。外部の試験 suite（WPT、html5lib-tests、test262、Wasm の spec test）は source tree に
  取り込まず、commit の SHA を固定して取得・検証して使う（ライセンスを確認、design.md §14.2）。Chrome の参照の描画は host の Debian の
  `chromium`（2026-09-27 に導入、153.0.8010.52）。
- 画面・窓: zdesktop の titlebar（WS070 の CONTROLS、ws070-p011 の後は TABS）。提示は wl_shm（design.md §8.4）。
- 画像: `libpng-compat`・`libz-compat`（ws071-p010、ws035-p040）、`libjpeg-compat`（この WS）。
- 関係: WS035 の p029〜p032（Chromium の port の計画）とは別の到達目標（自前の engine）。両方を残す。

## 判断が要る点（既定で進めた）

design.md §17 の D1〜D11（process の構成、TLS の `dlopen`、titlebar、仕様・Unicode の表の commit、GIF の置き場、wl_shm、
libpng-compat の `from_memory`、libtruetype の拡張、libjpeg-compat の API、既定の font、段階の目標値）。どれも戻せる既定で進める。
2026-09-28 ユーザー:「ブラウザは、現在の方法で進めてください。文字の表は、生成した表をコミットしていいてす。」→ D1〜D3・D5・D9〜D11 は
今の既定のまま。D4 は生成した表を commit する（2026-09-28 に切り替えた、p030 の phase.md）。

## 2026-09-30 ラップアップと次の作業

- [p097](phase097/phase.md) と q506 は cleared。固定WPT commit `2d66b9b7998bb58c336138c178323ddee857b586`で、
  CSS2 reftestの固定sampleは44/100、Acid2はexact pixel不合格・90.56%、Acid3は9/100・pixel agreement 40.35%。
  数値は [results/acid.txt](results/acid.txt) と [results/wpt-reftest.txt](results/wpt-reftest.txt) に残した。
- Amazonのscript有効なguest版は、plain objectの疎な数値keyをdense arrayの`length`として扱っていたVMの不具合を直し、
  70秒後までprocess生存・追加SIGSEGVなしを確認した。最後の実装修正はcommit `6184fb12`。
- q507 で [p099](phase099/phase.md) を cleared。Acid2 は同じ固定条件で
  90.56% から byte-identical な 100.00% になり、plain/ASan と focused
  regression が通った。blocking p172のq579は部分統合・検証を保存してuncleared。全体clear/実出力確認後にp100→p101とbranch側Phaseを再評価し、
  一つずつQueueへ選ぶ。

1. **ws074-p099: Acid2 100% — cleared（q507）**。p097 と同じ harness、viewport、font、固定 WPT 版で
   `exact-pixel-pass 1`、pixel agreement 100.00%、crash・timeout 0。差分を一般化した focused regression を残した。
2. **ws074-p100: Acid3 100%とpixel完全一致** — p172の後。p099はcleared。履歴的Acid3が画面上で100/100を返し、
   固定参照画像との差が0 pixelで、Uncaught・fail・crash・timeoutが無いことを完了条件にする。参照/viewport/font/runnerは着手時に固定する。
3. **ws074-p101: WPT CSS2 reftest 100%** — p172とp100の後。固定commitで現runnerが列挙するscriptなしCSS2 reftest全5904件を
   対象にし、sample 100件ではなく全件でpass 100%、error 0を完了条件にする。manifestと分母を着手時に固定し、
   WPT metadataに基づかない任意の除外はしない。長時間実行はshard・途中再開・失敗cluster別のreportを使う。
4. **ws074-p174: File System Access API** — p172とp100の後。仕様/WPTと権限・picker・shell/engine境界を確定し、機能群を分けて実装する。
5. **ws074-p175: OPFS** — p174の共有handle/権限契約の実出力の後。origin分離した保存領域を仕様/WPTで確かめる。
6. **ws074-p173: Interop 2025 100%** — 公式focus areaの対象WPTを固定版で列挙し、runnerの対応とbaselineを記録する。
   その結果を基に小さな後続Phaseを設計して全対象PASSへ進める。p101は既存の独立したCSS2全件目標として保持する。
7. **ws074-p176: Test262** — 既存runnerをp172の成果と照合して全suiteを固定/計測し、段階的な改善Phaseを作る。最終達成率は未指定。

4〜7は専任P10の投入候補順。独立項目に不要な技術的依存は作らず、各項目の実行Queueはmainが承認と前提を確認して別途投入する。

既存のp098（ES module script）は削除・再採番せずp101の後へ延期する。p036のCSS2全件部分はp101へ分け、
p036はその後のflexbox・backgrounds・values・selectorsの拡張suiteに絞る。p047（DOM testharness）も今回の3 Phaseの後まで候補のまま残す。

## Phase 一覧

**Blocking gate（2026-10-02）**: [p172](phase172/phase.md)のwhole-Phase clearanceと実際の統合出力を、全ての未実行WS074 source/runner/互換性Phaseの共通の前提にする。下表のplanned行の依存欄に明示した。旧cleared成果に遡及しない。branch側phase100・phase102〜171の同じ論理IDは取込時に証拠を照合し、勝手にrenumber/clearしない。

p001 で分け直した（2026-09-27。p002〜p013 の案は実行前の案だったので、同じ番号を新しい分割に使う）。各 Phase は 1〜3 時間を目標にし、
着手の時に大きすぎれば分ける。

**実行の順**（2026-09-27 ユーザー「正常系でワンパス通すのを優先する」、design.md §18）: p005 → p007 → p010 → p011 → p012 → p014（窓に
実際の page）→ p045（URL の欄と link）→ p022 → p023 → p024 → p025 → p026 → p046 → p030（JS の接続）→ p013 → p048 → p049 → p015 → p016 → p017 → p019 → p020 → p051 → p021 → p053 → p052 → p050 → p058 → p054 → p055 → p056 → p057 → p006 → p008 →
p009 → p018 → p027 → p028 → p029 → p047 → p031 → p032 → p033 → p034 → p035 以降。各 Phase は最小の範囲で通し、残りは phase.md の「後回し」へ。

| Phase | 内容 | Status | 依存 |
| --- | --- | --- | --- |
| [ws074-p001](phase001/phase.md) | 全体の設計（[design.md](design.md)）、Chromium の導入、Phase の分割 | cleared | — |
| [ws074-p002](phase002/phase.md) | 骨組み: directory、build の登録（amd64）、`base/`（arena・配列・文字列 buffer・UTF-8/16・hash）、headless の mode の入口、host の build（ASan の組）、suite の取得の script（固定 SHA とライセンスの確認） | cleared | p001 |
| [ws074-p003](phase003/phase.md) | GC heap の核（非移動の mark-sweep、大きさの class の block、保守的な stack の走査、trace）、VM の string と atom | cleared | p002 |
| [ws074-p004](phase004/phase.md) | HTML tokenizer（全状態、文字参照の表の生成）、html5lib の tokenizer の runner。目標 ≥ 98% | cleared | p003 |
| [ws074-p005](phase005/phase.md) | DOM の核（GC の cell の Node・Element・Text・Comment・Document・DocumentType、属性）と tree builder 1（initial〜in body、adoption agency）、html5lib の tree の runner | cleared | p004 |
| ws074-p006 | tree builder 2（table・select・template・frameset・foreign content）、fragment parsing、serializer。目標 script-off ≥ 90% | planned | p005、p172 |
| [ws074-p007](phase007/phase.md) | CSS の最小（ワンパス）: tokenizer・parser、selector（type・class・id・子孫・子・属性の基本）、cascade（origin・specificity・継承）、約 30 の property、UA stylesheet、`<style>`・`style` 属性 | cleared | p003 |
| ws074-p008 | CSS 2（後回し）: selector の残り（構造・状態・`:is`/`:not`/`:has`、pseudo-element）、rule の索引 | planned | p006、p007、p172 |
| ws074-p009 | CSS 3（後回し）: property の表の拡張、`var()`・`calc()`、`@media`、file の `<link>`、shorthand の全部 | planned | p008、p172 |
| [ws074-p010](phase010/phase.md) | font と text の最小: font の一覧（Inter、日本語は Droid の fallback）、libtruetype（関数を足すなら main に先に伝える）、advance、空白と CJK での改行 | cleared | p002 |
| [ws074-p011](phase011/phase.md) | layout の最小: box tree（anonymous box）、block（幅・高さ・margin の基本）、inline（line box・text run・baseline）、`--dump=layout` | cleared | p009、p010 |
| [ws074-p012](phase012/phase.md) | 描画の最小: display list（背景・border の solid・text）、CPU の参照の描画、`--render`（PPM → PNG）、画面の撮影 | cleared | p011 |
| [ws074-p013](phase013/phase.md) | layout 2a: position（relative・absolute・fixed）、offset、z-index の描画の順と hit test（2026-09-28 に float を p048、overflow を p049 へ分けた。list と marker は p011、単位は p007、replaced の大きさは p021） | cleared | p012 |
| [ws074-p048](phase048/phase.md) | layout 2b: float と clear（行の箱を float の横で短くする、block formatting context） | cleared | p013 |
| [ws074-p049](phase049/phase.md) | layout 2c: overflow と clip（display list の clip、CPU と GPU の描画）、overflow が作る block formatting context | cleared | p013 |
| [ws074-p014](phase014/phase.md) | 窓: Wayland と Vulkan（swapchain、display list の GPU の描画: instance の四角と glyph の atlas）、scroll、guest で実際の page を表示。GPU と CPU の描画の比較の試験（2026-09-27 に URL の欄と link を p045 へ分けた） | cleared | p012（p013 は後回しの順） |
| [ws074-p015](phase015/phase.md) | URL（WHATWG）、`data:`、WPT の urltestdata の runner（896/896、data-urls 72/72） | cleared | p002 |
| [ws074-p016](phase016/phase.md) | HTTP/1.1 の同期の client（chunked、redirect）、cookie、http の page と script、host の test server、guest の http（2026-09-28 に非同期の loader・resolver の thread・持続接続・memory の cache を p050 へ分けた） | cleared | p014、p015 |
| [ws074-p017](phase017/phase.md) | TLS（OpenSSL の `dlopen`、D2）、https、自前の CA の host の server、guest で実在の site（BUG-083 の rtld の dlopen の修正を含む） | cleared | p016 |
| ws074-p018 | encoding: 判定（BOM・HTTP・meta の prescan）、UTF-16・legacy の single-byte、Shift_JIS・EUC-JP・ISO-2022-JP（表の生成、D4） | planned | p006、p172 |
| [ws074-p019](phase019/phase.md) | `libjpeg-compat` 1: baseline（huffman、任意の subsampling、restart、grayscale・YCbCr）、library の登録、host の試験（Pillow と比較）。2026-09-28 に base の group・全 platform、libpng-compat の header を `compat/png/` へ | cleared | p002 |
| [ws074-p020](phase020/phase.md) | `libjpeg-compat` 2: progressive、CMYK/YCCK、`jpeg_save_markers`（EXIF の向き）（`JCS_EXT_BGRA` は p019 で済み） | cleared | p019 |
| [ws074-p051](phase051/phase.md) | `libgif-compat`（2026-09-28 ユーザー、D5 の変更）: `userland/base/libgif-compat`、`include/libc/compat/gif_lib.h`（giflib 5.2 の decode の部分集合）、全 platform | cleared | p002 |
| [ws074-p021](phase021/phase.md) | browser の画像: `<img>`（replaced box）、JPEG・PNG・GIF（base の compat の library）、画像の表、CPU と GPU の描画（2026-09-28 に CSS の背景画像を p052 へ分けた） | cleared | p016、p020、p051、ws071-p010 |
| [ws074-p052](phase052/phase.md) | CSS の背景画像（p021 から分けた）: `background-image: url()`、repeat、position、size、`background` の shorthand、canvas の背景画像 | cleared | p021 |
| [ws074-p053](phase053/phase.md) | 部品としての browser の設計（design.md §19）: engine と shell・Wayland の依存の棚卸し、C API（create・destroy・load・resize・呼ぶ側の VkImage への描画・入力の event・callback）の header の案（[browser_view.h](phase053/browser_view.h)）。.so の分割はまだしない。shell の bind・net への直接の依存を page の API に | cleared | p021 |
| [ws074-p022](phase022/phase.md) | VM の核 2: 値（NaN-boxing）、object と shape、配列の elements、関数、realm の骨組み | cleared | p003 |
| [ws074-p023](phase023/phase.md) | 共通の bytecode と interpreter、呼び出し規約、例外の unwind、native 関数（手で組んだ JS 型と Wasm 型の命令の試験） | cleared | p022 |
| [ws074-p024](phase024/phase.md) | JS の lexer と parser（ES2024 の構文 → AST）、test262 の構文の試験（parse だけ。46876/47792） | cleared | p023 |
| [ws074-p025](phase025/phase.md) | JS の compiler（ES5 の核）、`--js` の shell、test262 の runner。最初の数（4992/47792、ES5 1457/8087） | cleared | p024 |
| [ws074-p026](phase026/phase.md) | 組み込み 1a: Object・Function（bind・Function の構築子）・Error の類（engine の誤りも object に）・Boolean・Number（自前の最短の十進表記と十進の読み取り、toFixed 等）・Math・global の関数、native の構築子（2026-09-28 に Array・String・JSON を p046 へ分けた。9327/47792、ES5 4822/8087） | cleared | p025 |
| [ws074-p027](phase027/phase.md) | RegExp の engine と String の regex の method | cleared（2026-09-29。自前の backtracking の engine、RegExp と String の match・replace・replaceAll・search・split。test262 14255 → 15762、ES5 6786 → 7592。v flag・`\p`・modifier・matchAll・Symbol の差し替えは残り） | p026 |
| ws074-p028 | ES2015 の意味 1（2026-09-29 に let・const・TDZ・arrow・template を p078 へ、destructuring・default・rest・spread・optional chaining を p079 へ、class を p085 へ、Symbol・iterator・for-of・Map・Set・Weak* を p087 へ分けた）: 残りは WeakMap・WeakSet の弱さ（collector の ephemeron）、Set の ES2025 の method、iterator helper | planned | p026、p087、p172 |
| ws074-p029 | ES2015 の意味 2（2026-09-29 に generator・Promise・async function を p086 へ分けた）: async generator と for await、`yield*`、Proxy・Reflect、TypedArray・ArrayBuffer・DataView、BigInt | planned | p028、p086、p172 |
| [ws074-p030](phase030/phase.md) | DOM の binding（interface の表、生成器は後回し）、window・document・Node・Element・Event の基本、console、`<script>` の実行、timer、event loop と microtask の queue（2026-09-28 に WPT の testharness の runner を p047 へ分けた） | cleared | p014、p046（p029 から縮めた: microtask の queue はこの Phase で作った） |
| [ws074-p031](phase031/phase.md) | Amazon の script が要る DOM の API（2026-09-29 に絞った）: querySelector・querySelectorAll・matches・closest、getBoundingClientRect・getClientRects・DOMRect・client/offset/scroll の大きさ・scrollX/Y（問われた時に layout）、classList・dataset、inline style（CSSStyleDeclaration）。getComputedStyle・innerHTML・offsetTop/Left・scrollTo は残り | cleared（2026-09-29。Amazon top の DOM の Uncaught 4 → 0、DOM の試験 4 つを追加、guest 12/12） | p030 |
| [ws074-p032](phase032/phase.md) | form（2026-09-28 に範囲を絞った。fetch・XHR・Location・History・localStorage は p064 へ。p067 で `<select>` の最小と `box-sizing` を足した）: `<input>`（text・submit・hidden・button・checkbox）・`<button>`・`<textarea>`・`<select>` の最小の描画、focus と caret、文字の入力と編集、Enter と submit による送信（GET・POST の urlencoded、page の encoding）、script が有効なときの `noscript` を隠す | cleared | p017、p056（focus と key の入力。p031 は要らない） |
| ws074-p033 | Wasm: decoder・validator・共通 bytecode への compiler、JS API、spec test の runner（wabt の wast2json） | planned | p029、p172 |
| ws074-p034 | Wasm の MVP の後: bulk memory、reference types、multi-value、sign-ext、非 trap の変換、SIMD。**M2 の計測** | planned | p033、p032、p172 |
| [ws074-p035](phase035/phase.md) | flexbox（最小） | cleared（2026-09-29。ASan・guest（QEMU、live の Amazon の検索）を通し、入れ子の flex の測定で % の幅が測定の幅になる不具合を直した） | p013 |
| ws074-p036 | CSS の段階 M3-1（2026-09-30にCSS2全件をp101へ分割）: WPT flexbox・backgrounds・values・selectorsの拡張suiteを測り、失敗の多い塊を直す | planned（p101後） | p034、p035、p101、p172 |
| [ws074-p037](phase037/phase.md) | table の layout（最小。CSS2 の auto layout、border-collapse は近似） | cleared（2026-09-29、amazon-goal の「最小」の範囲。test page 60/73 box、Amazon のトップの footer が Chromium と同じ 4 列。rowspan・fixed・collapse の解決・column の box は残り） | p036（デモの列の判断で先に最小） |
| ws074-p038 | transform（2D）、transition・animation、gradient、box-shadow、角丸の clip、opacity、`@font-face`（TTF/OTF） | planned | p036、p172 |
| ws074-p039 | grid | planned | p036、p172 |
| ws074-p040 | Chrome との比較の拡大（corpus と実在の site の保存、box と画素の指標）、`-webkit-` の表（別名、`-webkit-box`、`-webkit-line-clamp` 等）。**M3 の計測** | planned | p037〜p039、p172 |
| ws074-p041 | shell 2: TABS の titlebar と窓の中の toolbar、System Menu、context menu、履歴、ページ内検索、zoom、view-source | planned | p032、ws070-p011、p172 |
| ws074-p042 | CSS の段階 M4: 失敗の塊から機能を選んで直す | planned | p040、p172 |
| ws074-p043 | JS・Chrome の段階 M4: test262 と比較の失敗の塊を直す。**M4 の計測** | planned | p042、p172 |
| ws074-p044 | 変更した source の規約の全文との照合、fuzz（時間を区切って）、回帰、boot test（最後） | planned | 全て、p172 |
| [ws074-p046](phase046/phase.md) | 組み込み 1b（p026 から分けた）: Array・String（正規表現の要らない method、UCD 16.0.0 から生成する大文字・小文字の表）・JSON（14255/47792、ES5 6786/8087） | cleared | p026 |
| [ws074-p045](phase045/phase.md) | 窓 2（p014 から分けた）: CONTROLS の titlebar の URL の欄、link の click（`file:`）、戻る・進む・再読み込み | cleared | p014 |
| [ws074-p050](phase050/phase.md) | 非同期の loader の核（p016 から分けた）: non-blocking な socket と TLS、resolver の thread、redirect、page の画像と shell の navigation の非同期化、Esc の中止。部品化の手順 4（`page_net_*`、p053）。2026-09-28 に持続接続と cache を p058 へ分けた | cleared | p016、p017、p053 |
| [ws074-p058](phase058/phase.md) | 持続接続と memory の cache（p050 から分けた）: host ごとの接続の pool（6 本まで）、長さ・chunked での応答の終わり、Cache-Control（max-age・no-store）、ETag と If-None-Match の再検証、304 | cleared | p050 |
| [ws074-p054](phase054/phase.md) | 部品化 1（p053 の手順 1）: view の型。scroll・履歴・timer・題名の変化を shell から engine の view へ、shell と main の headless の mode は view の API だけを呼ぶ。headless の settle・CPU の描画・dump も view に（2026-09-28） | cleared | p053、p050 |
| [ws074-p055](phase055/phase.md) | 部品化 2（p053 の手順 2）: GPU の描画を `browser_target` と `_record`・`_draw` の形に（view 内の image view ごとの framebuffer）、present.c は swapchain と同期だけ。`--render-gpu` は engine の offscreen に `_draw` | cleared | p054 |
| [ws074-p056](phase056/phase.md) | 部品化 3（p053 の手順 3）: 入力を DOM の key・code・text の形に（evdev の変換は shell）、scroll・link・focus の既定の動作を engine へ。KeyboardEvent・WheelEvent・FocusEvent、Tab の focus と ring。form の欄は無い（p032） | cleared | p055 |
| [ws074-p057](phase057/phase.md) | 部品化 4（p053 の手順 5）: `libbrowser.so` への分割、公開の header `include/libc/browser.h`、2 つ目の使い手 `browser-probe`（Wayland なしで PPM に描く） | cleared | p056 |
| ws074-p047 | WPT の testharness の runner（p030 から分けた: testharness.js は arrow・let・const・class と Promise を使う）、WPT dom/nodes の計測（M2 の目標 ≥ 40%） | planned | p028、p029、p030、p172 |
| [ws074-p059](phase059/phase.md) | Google の調査（デモの目標の前提）: 返る HTML の版（UA ごと）、Chromium との比較、足りない機能、目標の版と Phase の列（[google-goal.md](google-goal.md)） | cleared | p057 |
| [ws074-p060](phase060/phase.md) | `inline-block` を atomic な inline に（shrink-to-fit、baseline、`vertical-align` の top・middle・bottom・baseline） | cleared（2026-09-29。検索の結果が 4 列の card に。`direction: rtl` の不足を発見） | p032 |
| [ws074-p061](phase061/phase.md) | CSS の値と selector（p008・p009 から Google が使う部分）: `@media`、`var()`、`calc()`、`!important`、`:not()`・`:hover`・`:active`・`:focus`・`:visited`、`::before`・`::after`、`box-sizing`、`-webkit-` の別名、`text-overflow`、`letter-spacing`、`text-transform` | cleared（値の部分: var・calc・@media・@supports・論理 property。selector と pseudo-element は p069、`box-sizing` は p032） | p007 |
| [ws074-p062](phase062/phase.md) | 描画（p038 から）: `border-radius`（CPU と GPU）、`opacity`、`box-shadow`、`outline` | cleared（2026-09-29。display list の矩形で表し CPU と GPU が一致、`clip-path: inset()` を足した。gradient・object-fit は残り） | p014 |
| ws074-p063 | （デモの列から外した: Amazon は sprite の PNG）inline の SVG の最小（`svg`・`path`・`circle`・`rect`、`viewBox`、`fill`・`currentColor`） | planned | p012、p172 |
| ws074-p064 | fetch・XHR（same-origin・CORS）、Location・History、localStorage（p032 から分けた） | planned | p031、p032、p172 |
| ws074-p065 | （Google の目標のため。デモの列から外した、2026-09-28）Google の challenge と ES5 bundle の JS の環境（2026-09-29 に `Date` を p076 へ、`navigator`・`screen`・`performance`・`location` の読み取り・`document.cookie` を p077 へ分けた）: `Date`・`Promise`・`Symbol`・`Map`・`Set`・`WeakMap`・typed array の最小、`encodeURIComponent` の類、`Error.stack`、`atob`・`btoa`、`navigator`・`screen`・`performance`・`sessionStorage`・`CustomEvent`、`document.cookie` の書き込み、`location.replace`。保存した challenge の page が `SG_SS` を置いて開き直すまで | planned | p027、p172 |
| ws074-p066 | （Google の目標のため。デモの列から外した）Google の結果の page: 保存（`build/` だけ）、Chromium との比較、足りない CSS と DOM の直し（取得後に分ける） | planned | p065、p172 |
| [ws074-p067](phase067/phase.md) | amazon.co.jp の調査（デモの目標の変更）: UA ごとの HTML、Chromium との比較、足りない機能、Phase の列（[amazon-goal.md](amazon-goal.md)） | cleared | p059 |
| [ws074-p068](phase068/phase.md) | 外の stylesheet（`<link rel=stylesheet>`・`@import`、非同期の loader、読み終えてからの再計算）と rule の索引（最右の id・class・tag） | cleared | p032 |
| [ws074-p069](phase069/phase.md) | selector と pseudo-element: `:not()`・`:is()`・`:where()`・`:has()` の最小、`:root`、構造の pseudo-class、`:hover`・`:focus`・`:disabled`・`:checked`、`::before`・`::after` の `content` | cleared | p068 |
| [ws074-p070](phase070/phase.md) | `@font-face`（WOFF、libz-compat）と Amazon Ember | cleared（2026-09-29。WOFF・TrueType の web font、WOFF2 は残り） | p068 |
| [ws074-p071](phase071/phase.md) | 大きな page の速さ（1.4 MB の HTML、2 MB の CSS）: 測って直す | cleared（2026-09-29。class の atom と計算した style の cache、sheet の到着をまとめる。guest の live の検索が 90 s → 30 s で揃う） | p068、p061 |
| [ws074-p072](phase072/phase.md) | grid の最小（`repeat(N,1fr)`、`grid-column`） | cleared（2026-09-29。test page 44/44 box、トップ 34.16% → 34.72%） | p035 |
| [ws074-p073](phase073/phase.md) | direction の最小（rtl の flex row と `text-align: start`）。2026-09-29 main の判断で追加（Amazon の検索の本体の行が rtl） | cleared（2026-09-29。検索 画素 68.83% → 77.16%、ink 20.06% → 34.68%） | p035 |
| [ws074-p074](phase074/phase.md) | block の幅の intrinsic の keyword（`max-content`・`min-content`・`fit-content`）。2026-09-29 main の判断で追加（Amazon のトップの carousel `.gwm-window-wrapper`） | cleared（2026-09-29。calc の入れ子の clamp、並ぶ float の max-content も。トップ 画素 34.72% → 79.12%、ink 24.43% → 75.06%） | p060 |
| [ws074-p075](phase075/phase.md) | container query の単位（`container-type: inline-size` の container の幅で cqi・cqw 等、要れば `@container` の最小）。2026-09-29 main の判断で追加（Amazon のトップの card の `143cqi`） | cleared（2026-09-29。前の layout の container の大きさで解決し、変われば layout をやり直す。トップ 画素 79.12% → 80.40%） | p061 |
| [ws074-p076](phase076/phase.md) | JS の `Date`（p065 から分けた。amazon-goal の 12 の一部、Amazon の script の最初の blocker）: 構築子・`now`・`parse`（ISO と legacy の形）・`UTC`、local と UTC の getter と setter、文字列の形、`toJSON`、Annex B | cleared（2026-09-29。test262 15762 → 16393、ES5 7592 → 7732、`built-ins/Date` 480/618。Amazon の top の `Date is not defined` 24 → 0。`Symbol.toPrimitive`・Temporal・Intl は残り） | p026、p046 |
| [ws074-p077](phase077/phase.md) | Amazon が要る window の環境の最小（p065 から分けた）: Uncaught の位置（bytecode の位置の表）、`navigator`・`screen`・`performance`・`location`（読むだけ）・`Image` と `HTMLImageElement`・`document.cookie`・`URL`・`hidden` 等、window の plain な property | cleared（2026-09-29。AUI の `P` が定義され、Amazon の top の Uncaught 49 → 10、search 44 → 15、search 画素 72.92% → 76.04%。location の移動は p064） | p030、p076 |
| [ws074-p078](phase078/phase.md) | ES2015 の構文 1a（p028 から分けた）: let・const（block scope、TDZ、反復ごとの binding、script 間の global の lexical な record）、arrow function、template literal（tagged を含む）、Uncaught の早期の誤り（二重の宣言） | cleared（2026-09-29。test262 16393 → 17190、Amazon の Uncaught top 10 → 7、search 15 → 9） | p025、p077 |
| [ws074-p079](phase079/phase.md) | ES2015 の構文 1b（p028 から分けた）: destructuring（宣言・代入・引数・catch・for-in）、default・rest の引数、spread（配列・呼び出し・new・object）、optional chaining。反復は配列・arguments・文字列だけ（Symbol.iterator は p028） | cleared（2026-09-29。test262 17190 → 18558。Amazon の default・rest・optional chaining の誤り 0、次は async・for-of・class） | p078 |
| [ws074-p080](phase080/phase.md) | Encoding API（TextEncoder・TextDecoder、UTF-8）と Web Storage（Storage・sessionStorage・localStorage、origin ごと、localStorage は file に保存、5 MiB の上限）。bind/・page/ の側（p031 の後の Amazon の blocker） | cleared（2026-09-29。Amazon の TextEncoder・sessionStorage の Uncaught 0、guest の live の top は Uncaught 0、DOM の試験 2 つを追加） | p030、p031 |
| [ws074-p081](phase081/phase.md) | HTML の断片の parse と直列化（innerHTML・outerHTML・insertAdjacentHTML・insertAdjacentElement・insertAdjacentText）。Amazon の script での使用が最も多い DOM の API（調べは phase.md） | cleared（2026-09-29。html5lib の fragment 206/206、template.content・namespaceURI、DOM の試験 markup を追加、host 15/15・guest 15/15。Amazon の画素は不変） | p005、p030、p031 |
| [ws074-p082](phase082/phase.md) | getComputedStyle（resolved value、73 の property、live、読むだけ）、offsetParent・offsetTop・offsetLeft、window の scrollTo・scroll・scrollBy と Element の scrollIntoView・scrollTo・scrollBy・root の scrollTop（page が clamp、view が移す）。box の索引（node → 最初の box） | cleared（2026-09-29。DOM の試験 computed を追加、host 16/16・guest 16/16。Amazon の Uncaught と画素は不変） | p031、p081 |
| [ws074-p083](phase083/phase.md) | DOMException（interface・name・code、bind_throw_dom を DOMException に、試験の頁の期待値の見直し）。main の指示（2026-09-29） | cleared（2026-09-29。name・code・instanceof が Chromium と一致、DOM の試験 exception を追加し 5 つの頁を name で見るように、host 18/18・guest 18/18） | p030、p081、p082 |
| [ws074-p084](phase084/phase.md) | Amazon の画素の差の原因の調べ（DOM・layout）と Phase の案。flex の row の item の自動の最小の大きさ（min-width:auto）と凍らせる縮みを直した | uncleared（2026-09-29、wrap up。調べと案は済み: 動的な script が走らない（top の約 10〜13 点）、百分率の高さ、太字の幅ほか。flex の変更の ASan・guest・boot が残り） | p031、p082 |
| [ws074-p085](phase085/phase.md) | class（p028 から分けた。p080〜p084 は DOM の側と衝突しないように空けた）: 宣言と式、constructor（既定を含む）、method・accessor・static、extends・super・new.target、public と private の field・method、static block | **cleared**（2026-09-29。test262 18558 → 21806、新しく落ちた 0。ASan・guest の js・dom の試験・boot 済み。Amazon の class の SyntaxError 2 → 0、Uncaught top 2・search 7、画素は不変） | p078、p079 |
| [ws074-p086](phase086/phase.md) | async function と、それに要る generator・Promise（p029 から分けた）: generator function と yield（`yield*` を除く）、Promise と組み合わせ、microtask の反応、未処理の reject の報告、async function と await | **cleared**（2026-09-29。test262 21806 → 24995、新しく落ちた 0。Amazon の async の誤り top 1・search 4 → 0、Uncaught search 7 → 3） | p030、p085 |
| [ws074-p087](phase087/phase.md) | Symbol・iterator の protocol・for-of と Map・Set・WeakMap・WeakSet（p028 から分けた）、symbol を使う操作（toPrimitive・hasInstance・toStringTag・species・RegExp の委譲）、URI の関数 | **cleared**（2026-09-29。test262 24995 → 28381、新しく落ちた 0。Amazon の for-of・`Set`・`encodeURIComponent` の誤り 0、search の Uncaught 0） | p079、p086 |
| [ws074-p088](phase088/phase.md) | 動的に挿入された `<script src>` の取得と実行、script の load・error の event（bind/・page/・dom/）。Amazon の AUI（`P.load.js` が 41 本を挿入）が初めて走る。p084 の調べの案 1（top で約 10〜13 点の見込み、新しい Uncaught と XMLHttpRequest（p064）の不足が出る見込み） | **cleared**（2026-09-30。dynamic top 69.23%/ink 61.20%、Web API不足をp092へ） | p084 |
| [ws074-p089](phase089/phase.md) | 百分率の高さ（`height`・`max-height` の %、layout 全体で「高さが定まっているか」を渡す。float・inline-block・grid の `1fr` の中を含む）。p084 の案 2 | **cleared**（2026-09-30。top 82.96%→84.06%、ink 79.01%→80.56%。search 75.85%→76.32%、ink 32.97%→34.48%） | p084 |
| ws074-p090 | 合成の太字（Latin）の advance を Chromium に合わせる（約 9% 広い）。p084 の案 3（1 点前後） | planned | p172 |
| ws074-p091 | flex の残り（overflow で隠れる項目の自動の最小、blockification、column の最小）と CSSOM の小さな不足（`cssFloat` など）。p084 の案 4・5 | planned | p084、p172 |
| [ws074-p092](phase092/phase.md) | Amazonの後続scriptが使うWeb API: `fetch`・observer・`document.elementsFromPoint`・`atob`/`btoa`と有界なheadless settle | **cleared**（2026-09-30。非同期fetch、MutationObserver、sign-in tooltipの幅とstacking order。top 71.81%/ink 64.58%、約32秒・Uncaught 4） | p087、p088 |
| ws074-p093 | JS: 型付き配列（ArrayBuffer・TypedArray・DataView） | planned | p087、p172 |
| [ws074-p094](phase094/phase.md) | Chromiumとの再現可能な比較手順: 固定capture、隔離profile、入力・環境・出力のhash、JSON reportとbaseline回帰 | **cleared**（2026-09-30。top 82.96%/ink 79.01%、search 75.85%/ink 32.97%） | p067 |
| [ws074-p095](phase095/phase.md) | Amazon検索欄の文字の位置: flex itemのcross-axisのbox sizing、form controlの`text-indent` | **cleared**（2026-09-30。検索文字の範囲がChromiumと同じ`x=435..574, y=22..36`） | p035、p032、p092 |
| [ws074-p096](phase096/phase.md) | 公開サイトの固定比較corpus: Mozilla日本語topから同一originのpageをたどり、固定Chrome User-AgentでChromiumと比較して一般化できる差を最大2件修正 | **cleared**（2026-09-30。4 page、`@supports`、button空白。top 78.23%/ink 69.54%） | p094、p095 |
| [ws074-p097](phase097/phase.md) | 指定10公開siteの画像・box・DOM tree比較、阿部寛のホームページのguest full-screenデモ、WPT reftestとAcid、一般化できる差を最大4件修正 | **cleared**（2026-09-30。WPT 44/100、Acid2 90.56%、Acid3 9/100。`postMessage`、table rowspan、presentational hint、intrinsic auto margin） | p094、p096 |
| ws074-p098 | ES module scriptのgraph取得・link・評価、`import`・`export`、GitHubのhydration比較 | planned（2026-09-30ユーザー指定によりp101後へ延期） | p087、p088、p097、p172 |
| [ws074-p099](phase099/phase.md) | Acid2 100%: 固定harnessでexact pixel一致、crash・timeout 0 | **cleared**（2026-09-30、q507。90.56% → 100.00%、plain/ASan） | p097 |
| [ws074-p100](phase100/phase.md) | Acid3の100/100、固定参照とのpixel完全一致、fail・Uncaught・crash・timeout 0 | planned（p172後、p099 cleared） | p099、p172 |
| ws074-p101 | WPT CSS2 reftest 100%: 固定commitのscriptなし全5904件でpass 100%、error 0 | planned（p100後、p172も必須） | p100、p172 |
| [ws074-p172](phase172/phase.md) | origin/browser2のbrowser変更をWS107後のlibbrowser/browser配置へ取込み、関連計画/試験/bugを意味的に照合するblocking gate | cleared / browser3 q598（209/209 review、whole final gates） | WS107の実成果、p099、branch snapshot |
| [ws074-p173](phase173/phase.md) | Interop 2025の公式focus area対象WPTを固定・baseline化し、全件PASSへ向けた失敗群と後続Phaseを設計 | planned / Queueなし | p172、p100 |
| [ws074-p174](phase174/phase.md) | File System Access APIの仕様/WPT・権限・picker/handle契約を確定し、機能群ごとの実装Phaseへ分割 | planning / Queueなし | p172、p100 |
| [ws074-p175](phase175/phase.md) | OPFSのorigin分離・永続保存・handle契約を確定し、機能群ごとの実装Phaseへ分割 | planning / Queueなし | p172、p174の共有契約の実出力 |
| [ws074-p176](phase176/phase.md) | Test262固定全suiteのbaselineと失敗群/runner coverage、段階的改善Phaseの設計 | planning / Queueなし | p172、JS runnerの実出力 |
| [ws074-p177](phase177/phase.md) | BUG-133: `wb_units_reserve` の幾何の増長後の byte 数の overflow を need に落として防ぐ（Q1 が割り当て、2026-10-03、P2） | in-progress（実装と host 試験済み、判定は Q1） | — |

## 後の WS・Future Work の候補

- base の libssl・libcrypto の互換品（OpenSSL の置き換え）。
- WebP、`libvorbis-compat`（ogg vorbis）と音声、動画（driver の後）、JS・Wasm の JIT。

## ユーザーの判断（2026-09-29 に master から移した）

| 項目 | 決定 | 記録先 |
| --- | --- | --- |
| 部品としての browser（2026-09-28） | ユーザー:「…libbrowser.soというファイルに将来的に分割できるようにして、コンポーネントにしましょう。ブラウザの構造体へのポインタをハンドルにして、Vulkanレンダリングターゲットを指定してイベントも送ってやるとと描画してくれて、Wayland依存はない、みたいなのがいいと思います！これはいきなり実現しなくても、徐々にそういう設計に変えていきましょう。」→ [WS074 design §19](ws074/design.md)（engine と shell の分離、不透明な handle、Vulkan の描画の先、将来の libbrowser.so） |
| ブラウザのデモの目標と touch（2026-09-28） | ユーザー:「Googleの検索トップページと検索が、レイアウトを崩さずに表示できたら、ゴールにしましょう！限定的なCSSと、基本的なJS、WebGLなし、ビデオなしです。インタラクションはそれでいいです。」→ WS074 のデモの目標。「タッチについては…スクロールの操作みたいに余韻のあるやつとかも実装が必要で…HIDドライバ、Waylandコンポジタ、ブラウザの3つに渡る…計画は1カ所で…タッチのfpsが低い廉価な機種でも、ある程度数式で補間して利用できるようにすることを目標に」→ [WS081](ws081/ws.md) |
| ブラウザのデモの目標の変更（2026-09-28） | Google の検索の結果の page のボットの判定（/sorry）の件でユーザー:「それはuser agentを正直に回答したせいであって、Chromeのものを使えばまず問題ないと思うのですが、amazon.co.jpに変更しましょう。」→ WS074 のデモの目標を **amazon.co.jp** に変更（Google は目標から外す）。p059 の Google の調査と form 等の Phase は流用 |


## 2026-10-01 レビュー: libbrowser の source 所有

ユーザーが engine source の libbrowser への移動、browser の window/tab shell、標準 Vulkan を使える Wayland無しの library、抽象入力 interface を指示。
[WS107](../ws107/ws.md) と [境界全文](../standards/browser-component.md)へ反映。配置の旧設計を置き換える。
WS074 は incomplete のまま。p099 の既存証拠/clearance、p100→p101 の順と互換性目標は保持する。
次の WS074 の Queue は新規則と実 source locator/WS107 の移動状況を確認し、同じ source/runner の移行と並行しない。
engine の new source は libbrowser が所有する設計で生成し、Wayland は shell の abstract input adapter に閉じる。
p100/p101 の個別 Phase record はこの2026-10-01記録時点で未作成だった（p100は2026-10-02追加、p101は未作成）。新しい implementation Queue や clearance はこの方針記録から発生しない。
2026-10-01 / review-20261001-policy-ws074: 新設 WS107 と policy 改訂/影響をこの WS に保存。GitHub の WS comment は公開保留。

2026-10-02 / ws107-q541-design: [WS107](../ws107/ws.md)の165engine file移行で現役host/guest/list-sources runnerを新rootへ更新する。独立componentの有限quality修正はWS107、p100→p101の既存互換性scope/順/acceptanceは保持。今後はlibbrowserrootの実sourceを使い、移行と同時編集しない。詳細[設計](../ws107/design.md)。公開outbox pending。

2026-10-02 / ws107-completed: engine165の新root/Makefile/header/ELF/clientをverified、host-view59/Acid2差0/goldens81/native shell PASS。DOM golden position/valuesのfixture追記漏れ2件のみ修正、旧build/newbuild出力一致。WS074のp100→p101/互換性scopeと順は不変。[WS107結果](../history/ws107/conformance.md)。GitHub comment deferred。

## 2026-10-02 / origin/browser2 blocking import

Event ws074-browser2-gate-20261002: ユーザー「GitHubのorigin/browser2を取り込みます。libbrowserへの移動によりストレートには当たらないが、一貫したファイル移動のルールで取込める。Phaseを追加し後続のブラウザ作業をblocking」と指示。[p172](phase172/phase.md)を追加。WS107の[移動inventory](../history/ws107/inventory.json)で旧engine→libbrowser、shellはbrowserと対応させる。branch tip `e53ef03b80113aec959deb67f828cba21d68d4be`を観測、branch Phase IDは171まで使用（p172を選択）。p100→p101の内部順は保持するが、着手の前にp172 whole-Phase cleared/実統合出力が必要。全27 planned行にp172依存を反映、branch取込後の既存/branch Phase結果を証拠で再照合。q507/p099と現在のQueueは履歴のまま、実装/merge/cherry-pick/pushは未実施。GitHub WS/foreign Phase event投稿は保留。

## 2026-10-02 / ブラウザ専任と追加の互換性目標

Event ws074-dedicated-interop2025-20261002: ユーザーが3人案の1人をWS074専任として固定し、branch取込後のAcid3をpixel完全一致・fail 0まで高め、WPT Interop 2025をクリアする目標を追加。P10を未起動の予約枠として[台帳](../agents/registry.md)に記録。p100の受け入れを強化し、新しい[p173](phase173/phase.md)を追加。p101のCSS2全件は保持するが、専任枠での次の互換性計測はp173を先にする。p172→p100→p173、p100→p101。実装Queue/merge/pushは未実施、remote WS/Phase/Project eventはpublication保留。

Event ws074-browser-next-goals-20261002: ユーザーが専任P10の次の目標としてFile System Access API、OPFS、Interop 2025 100%、JavaScript Test262を指定。新しい[p174](phase174/phase.md)・[p175](phase175/phase.md)・[p176](phase176/phase.md)を追加し、p173の数値目標を全対象PASS 100%と明示。専任の投入候補順はp172→p100→p174→p175→p173→p176。p101 CSS2全件は独立候補として保持。Phaseの最初のbaseline/design clearanceは製品目標達成を意味しない。実装Queue/agent起動/merge/pushは未実施、GitHub publication保留。

2026-10-02 / n3-execution-start: current userのN=3継続指示。P8 q577 BUG-125/p017、P9 q578 C10/p014、P10 q579 browser2/p172を有限3時間で開始。既存focus/順位を保持、browser後続はp172 whole-Phase clearance待ち。[Queue](../queue.md)を参照。GitHub publication保留。

2026-10-02 / q579-wrap-uncleared: p172のbranch統合と広い回帰証拠を保存したが、全209対象の最終reviewは8完了、C/header残141、その他残60。userのagent停止指示でq579をuncleared終了。A1が再開候補を所有し、後続browser Phaseは引き続きblocked。GitHub publication保留。

2026-10-02 / c-table-forward-order-20261002: [p172](phase172/phase.md)全文レビューによりC§2の定数callback tableと先行宣言のcompile矛盾を確認、userが必要宣言だけ先行させる例外を承認。全文標準/Guardrail/automationに記録。import gate/他の全文条件/後続依存は不変。GitHub comment未公開。

2026-10-02 / q584-checkpoint06-integrated: A1-001 4da5eb2e5をmain c2743455cへ統合・ACK。4全文reviewと規約修正、worker warning0 host build/.text同一の証拠、main所有権/例外契約/diff/source hash照合。[checkpoint06](phase172/import/checkpoint06/README.md)。p172はin-progress、review12/209でwhole gateは閉じたまま。

2026-10-02 / q584-A1-wrap-uncleared: Latest user requested normal all-agent wrap-up. [p172 checkpoint14](phase172/import/checkpoint14/README.md) retains all209 current hashes,97 full reviews,112 pending C/header files, warning0 affected build/compiles, identical168 object texts and117 actual native checks. q584-i01/whole p172 end uncleared; WS remains incomplete and downstream gates stay closed. Explicit finite p172 continuation and final conformance are required after main merge/reconciliation. GitHub WS/Phase comments remain unpublished under main ownership.

2026-10-02 / browser3-direct-continuation: current userがサブエージェント無しでWS074継続、browser3へのWIP commit/pushを指示。[p172 q590 scope](phase172/browser3/q590-approved-scope.md)を残112件全文reviewへ投入。q58497/209と全209hashを照合、origin/main261903952へ更新。p172全体基準・p100等の後続gateを保持、Issue/Project publication保留。

2026-10-02 / q590-browser3-checkpoint15: [p172 checkpoint15](phase172/import/checkpoint15/README.md) reviews child styles, DOMImplementation and real viewport fixture fully, fixes callback-retired child cache publication under the existing lifetime contract, and retains188 passing native checks/warning0 build. Reviewed100/209, remaining109; WS incomplete, p172 in-progress and downstream acceptance/dependencies unchanged. Internal amendment belongs only to p172; remote Issue comments deferred.

2026-10-02 / q590-browser3-checkpoint16: [p172 checkpoint16](phase172/import/checkpoint16/README.md) completes image geometry/images/CSSOM/select native fixture review,44 native checks and warning0 compile/link. Reviewed104/209, remaining105; no production change, WS incomplete and downstream gates unchanged.

2026-10-02 / q590-browser3-checkpoint17: [p172 checkpoint17](phase172/import/checkpoint17/README.md) reviews click and timer contracts fully, with native171 checks,57 click-page PASS and existing settle22 checks/warning0 build. Reviewed106/209, remaining103; WS incomplete and downstream gates unchanged.

2026-10-02 / q590-browser3-checkpoint18: [p172 checkpoint18](phase172/import/checkpoint18/README.md) fixes reproduced legacy data-resource error leak under existing ownership, retains before/after LeakSanitizer and181 unique native observations/warning0 build. Three full reviews bring109/209, remaining100; WS incomplete, p172 in-progress, downstream dependencies/criteria unchanged. Internal amendment02 changes only p172; GitHub events deferred.

2026-10-02 / q590-browser3-checkpoint19: [p172 checkpoint19](phase172/import/checkpoint19/README.md) finishes2 full C/component reviews, preserving bootstrap order/class/private contracts with native51 checks and47 exact expected JS lines/warning0 build. Reviewed111/209, remaining98; WS incomplete, p172 in-progress, downstream criteria/dependencies unchanged.

2026-10-02 / q590-browser3-checkpoint20: [p172 checkpoint20](phase172/import/checkpoint20/README.md) reviews DOM subscription/form state fully with147 passing native checks/warning0 build. Reviewed113/209, remaining96; WS incomplete, p172 in-progress, downstream criteria/dependencies unchanged.

2026-10-02 / q590-browser3-checkpoint21: [p172 checkpoint21](phase172/import/checkpoint21/README.md) reviews serialization/Symbol fully, with69 exact expected lines/native77/warning0. Reviewed115/209, remaining94; WS incomplete, p172 in-progress, unchanged downstream criteria/dependencies.

2026-10-02 / q590-browser3-checkpoint22: [p172 checkpoint22](phase172/import/checkpoint22/README.md) completes3 native XML/namespace fixture reviews, native73/warning0. Reviewed118/209, remaining91; WS incomplete, p172 in-progress, unchanged downstream criteria/dependencies.

2026-10-02 / q590-browser3-checkpoint23: [p172 checkpoint23](phase172/import/checkpoint23/README.md) reviews Page geometry fully, with exact DOM43 lines/child-style30/native160/warning0. Reviewed119/209, remaining90; WS incomplete, p172 in-progress, unchanged downstream criteria/dependencies.

2026-10-02 / q590-browser3-checkpoint24: [p172 checkpoint24](phase172/import/checkpoint24/README.md) reviews child resource tasks fully, actual loader52/native148/page14/warning0. Reviewed120/209, remaining89; WS incomplete, p172 in-progress, unchanged downstream criteria/dependencies.

2026-10-02 / q590-browser3-checkpoint25: [p172 checkpoint25](phase172/import/checkpoint25/README.md) repairs two reproduced native MIME parsing defects within full review, final data80/resource105/LSAN80/warning0. Supporting fixture checkpoint11 review revalidated; partial publication050031f14 management records completed here. Reviewed121/209, remaining88; WS incomplete, p172 in-progress, unchanged downstream criteria/dependencies.

2026-10-02 / q590-browser3-checkpoint26: [evidence](phase172/import/checkpoint26/README.md), NodeIterator full review, native46/page60/own warning0/style0; original cursor/prototype/weak ownership preserved; reviewed122/209, remaining87 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint27: [evidence](phase172/import/checkpoint27/README.md), Full traversal/removal fixture reviews, native46/scoped warning0/style0; unchanged GC/adoption corpus; reviewed124/209, remaining85 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint28: [evidence](phase172/import/checkpoint28/README.md), Media full review/reproduced six wrong matches fixed, native39/page38/own warning0/style0; supporting fixture revalidated; reviewed125/209, remaining84 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint29: [evidence](phase172/import/checkpoint29/README.md), JSON full review,116 exact JS lines/LSAN9/own warning0;48 permitted cleanup findings, ordinary error guards; reviewed126/209, remaining83 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint30: [evidence](phase172/import/checkpoint30/README.md), CSS model/mutation fixture full reviews, native56/scoped warning0;14 permitted cleanup findings; original corpus unchanged; reviewed128/209, remaining81 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint31: [evidence](phase172/import/checkpoint31/README.md), Select/table full review, native140/page213/own warning0/style0; constant registries and native contracts preserved; reviewed130/209, remaining79 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint32: [evidence](phase172/import/checkpoint32/README.md), Full select/option/table collection fixture reviews, native123/scoped warning0;19 permitted cleanup findings, original GC corpus unchanged.; reviewed133/209, remaining76 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint33: [evidence](phase172/import/checkpoint33/README.md), Realm full review, native82/JS79 exact lines/own warning0/style0; primary/managed ownership and microtask behavior preserved.; reviewed134/209, remaining75 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint34: [evidence](phase172/import/checkpoint34/README.md), Click fixture full review, actual native10/scoped warning0/style0; unchanged no-stack-root activation/cancellation corpus.; reviewed135/209, remaining74 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-terminal: [p172 terminal](phase172/browser3/q590-wrap/README.md)とarchive（別セッションの記録、main には入れない）を検証。38追加全文review/4組の内部defect修正・有限検証を保存しq590/p172 unclearedで終了。残74全文review/whole最終検証が必要でWS incomplete、p099 retained cleared。受け入れ/後続依存/priorityは変更せず、後続未選定、Issue/Project公開保留。

2026-10-02 / q594-direct-start: [p172 scoped five reviews](phase172/browser3/q594/selection.json) selected under latest user continuation,90min. Whole acceptance/downstream dependencies unchanged, WS incomplete; WIP/browser3 push and post-next-commit cleanup.

2026-10-02 / q594-browser3-checkpoint35: [evidence](phase172/import/checkpoint35/README.md), XML binding/parser/stream fixture full reviews, native51/scoped warning0;24 permitted cleanup findings; original native scripts and GC contracts unchanged.; reviewed138/209, remaining71 C/header. q594/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q594-browser3-checkpoint36: [evidence](phase172/import/checkpoint36/README.md), CSS rule model and Range deletion full reviews, final own warning0 build/native67;18 permitted shared-cleanup jumps; original CSS sources/native scripts and GC root exclusion preserved.; reviewed140/209, remaining69 C/header. q594/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q594-browser3-terminal: [terminal/verified archive](phase172/browser3/q594/README.md). q594 finished/item cleared for exact five-file partial scope, whole p172 uncleared/WS incomplete. Full review135→140/209,69 C/header remain, all209 hashes match, scoped118 native checks/warning0. Requested441-file/~107MiB cleanup completed after next WIP. New Queue/downstream unselected, next unreserved q595/q591–q593 reservations retained; WIP/normal browser3 push, Issue/Project publication deferred.

2026-10-02 / q595-autonomous-start: current userしばらく自走 instruction applied to [finite ten-file selection](phase172/browser3/q595/selection.json), at most3h. All209 source hashes matched q594 terminal. No subagents, whole criteria/downstream dependencies unchanged, shared toolchain untouched, WIP/browser3 push, Issue/Project deferred.

2026-10-02 / q595-browser3-checkpoint37: [evidence](phase172/import/checkpoint37/README.md), Range boundary/data full reviews; native89+100/scoped warning0;36 permitted cleanup jumps; original native GC/root exclusion and mutation scripts preserved.; reviewed142/209, remaining67 C/header. q595/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q595-browser3-checkpoint38: [evidence](phase172/import/checkpoint38/README.md), Range clone/extract full reviews; native15+14/scoped warning0;53 permitted cleanup jumps; original6000/1800 participant corpus and allocation-GC root exclusions preserved.; reviewed144/209, remaining65 C/header. q595/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q595-browser3-checkpoint39: [evidence](phase172/import/checkpoint39/README.md), Range insert/mutation/surround full reviews; native92+36+16/scoped warning0;43 permitted cleanup jumps; original direct native conversion/host callback/1800-node GC and weak tokens preserved.; reviewed147/209, remaining62 C/header. q595/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q595-browser3-checkpoint40: [evidence](phase172/import/checkpoint40/README.md), Collection native/form full reviews plus actual verified unpublished-state and argument-conversion GC repairs; native37+13 and affected12+25+10+65+56=218, own private build/scoped warning0;33 permitted forward cleanup jumps; original caller-root exclusions/corpora and all production strings/23 constant tables preserved.; reviewed150/209, remaining59 C/header. q595/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q595-collection-ownership-amendment01: verified missing callee roots during unpublished-wrapper allocation and four user conversion families repaired within selected collection binding/fixtures. Original failures and exact snapshots retained; final218 actual native checks/warning0 and retirement reclamation verified. Internal design only, foreign scope/interfaces/dependencies unchanged, whole p172 uncleared/WS incomplete and59 remaining full reviews/final gates. Issue/WS remote event delivery deferred. [design/evidence](phase172/browser3/q595/amendment01.md).

2026-10-02 / q595-browser3-terminal: [verified terminal/archive](phase172/browser3/q595/README.md), exact ten-file partial cleared/Queue finished. Review140→150/209,59 C/header remain/all209 current hashes verified, actual native580/warning0. Two reproduced collection GC ownership defects repaired; whole p172 uncleared/WS incomplete/final gates and downstream unchanged. All WIP/browser3 normal push, main only/own invoked builds and fixtures returned, Issues/Project deferred. Next Queue unselected, next free q596/q591–q593 reservations retained.

2026-10-03 JST / q596-autonomous-acid3-start: current user「では、その目標に向かって自走をお願いします。」selects [existing p172 finite continuation](phase172/browser3/q596/selection.json), at most3h/remaining59 full reviews and original whole final checks, from all209 verified current hashes. Goal remains Acid3 score100/100/zero differing pixels; p100 gate and fg017/WS relative priority retained, no new product choice. Main only/all WIP/browser3 push, remote Issues/Project deferred.

2026-10-03 JST / q596-function-ownership-amendment01: [internal design/fix](phase172/browser3/q596/amendment01.md); actual factory/reentrant-GC failures reproduced and input/unpublished/suspended-state ownership revised in the selected files. Scope/interfaces/dependencies/whole criteria unchanged; final checkpoint41 verification pending, p172 in-progress/WS incomplete.

2026-10-03 JST / q596-browser3-checkpoint41: [evidence](phase172/import/checkpoint41/README.md), VM function/factory full reviews; actual parent/code/closure and reentrant-native GC repairs, default factory56/interp45/realms30/ownership27/heap31 plus retired-realm PASS, fixed JS14/14, own warning0;83 permitted forward cleanup findings and original production strings/tables preserved.; reviewed152/209, remaining57 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint42: [evidence](phase172/import/checkpoint42/README.md), Realm/context ownership fixture full reviews; native30+27/scoped warning0/style0; immediate checked execution before teardown/source release, output brand guards and preserved actual GC/root exclusions/corpora.; reviewed154/209, remaining55 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint43: [evidence](phase172/import/checkpoint43/README.md), Form-owner/native clone/table mutation/row fixture full reviews; native56+26+42+75=199/scoped warning0, 37 permitted cleanup jumps, original actual GC/root exclusions/native scripts/6000/512 cloning corpus and C strings retained.; reviewed158/209, remaining51 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint44: [evidence](phase172/import/checkpoint44/README.md), Insertion/mutation-record/style-sheet/submit full reviews; native29+16+21+13=79 plus prior table42+75=117/scoped warning0; 85 allowed single forward cleanup jumps; original native GC/root exclusions/scripts/C strings retained; prior split-call format claim corrected and revalidated.; reviewed162/209, remaining47 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint45: [evidence](phase172/import/checkpoint45/README.md), Native SVG length409/text347 full reviews; actual60+42=102 native checks/scoped warning0; 34 permitted forward cleanup jumps; original scalar/text/child XML corpora, precise GC/sole native roots and C strings retained.; reviewed164/209, remaining45 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint46: [evidence](phase172/import/checkpoint46/README.md), XML model518/projector541 full reviews; actual236+54=290 native checks/scoped warning0; 20 permitted forward cleanup jumps; original XML syntax/bounds/pinned resources/native graphs, actual30000 wide corpus/precise GC roots and C strings retained.; reviewed166/209, remaining43 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint47: [evidence](phase172/import/checkpoint47/README.md), Native property617/shared-heap probe337 full reviews; actual65+8=73 native checks/scoped warning0; 0 permitted forward cleanup jumps; original native property policies/errors/GC and8 realm identity cases/C strings retained.; reviewed168/209, remaining41 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint48: [evidence](phase172/import/checkpoint48/README.md), Actual HTTP/local child-loading584 full review; native52/scoped warning0; 25 permitted forward cleanup jumps; original asynchronous/delayed/cancellation/timer/native GC/8MiB pressure/root interval corpus and C strings retained.; reviewed169/209, remaining40 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint49: [evidence](phase172/import/checkpoint49/README.md), Frame lifetime1190 full review; original148 native checks/scoped warning0; 79 permitted forward cleanup jumps; all29 native saved graph families/capped failure/owner counters/precise GC root intervals and C strings retained.; reviewed170/209, remaining39 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-terminal: [terminal/verified archive](phase172/browser3/q596/README.md). q596/q596-i01 finished/uncleared at the unchanged 3h deadline, p172 uncleared and WS074 incomplete. Checkpoints41–49 finished20 full reviews, now170/209 with39 production C/header pending. Checkpoint50 captured the real vm_iter_rest GC crash and a scoped repair with warning0 compile and unchanged actual probe exit0; spread.c full review and one style issue remain. Whole p172 manifest/ABI/client/native/ASan/target/boot gates remain unverified, and the whole host builder exit is unknown. p100 remains planned/unselected; last fixed Acid3 comparison is37.04% agreement,302208/480000 differing pixels. Next q597 is only a candidate, q591–q593 reservations retained; no subagents, Issues/Project publication deferred.

2026-10-03 JST / q597-autonomous-start: current userの既存Acid3自走指示により、q596 archive read-back後、[39 source/whole original p172 exact selection](phase172/browser3/q597/selection.json)を有限3hで開始。全209 current hashesとclean HEADを確認、他executor無し。p100はwhole clearance/実出力待ち、WS incomplete、A/B予約と後続依存は維持。main単独、WIP/browser3 push、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint51: [evidence](phase172/import/checkpoint51/README.md), VM spread.c全文review完了。実GCで修正前の引数展開NaN/オブジェクトspread失敗/配列spread SIGSEGVを確認し、同じ3経路とrest/protocolの計5probeは修正後exit0。full host build warning0、JS14/14、関連host56+100+31+37、styleは許容forward cleanup54のみ。他の38 C/headerとwhole manifest/ABI/client/ASan/target/bootは未達。review171/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint52: [evidence](phase172/import/checkpoint52/README.md), JS Function builtin628行全文review。修正前の実GCでapply引数喪失→NaN、bind未公開関数回収→SIGSEGVを再現し、修正後同じ2probe exit0/期待値42・live=1。full host build warning0、JS14/14、host56+100+31、styleは許容forward cleanup30のみ。他の37 C/headerとwhole gatesは未達。review172/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint53: [evidence](phase172/import/checkpoint53/README.md), DOM mixin631行全文review。検索文字列/中間・結果配列とappend/prepend未挿入Textの実GC所有を修正、full host build warning0、DOM23/23、native insertion29/mutation16/tree exit0、styleは許容forward cleanup19のみ。残36 C/headerとwhole gates未達。review173/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint54: [evidence](phase172/import/checkpoint54/README.md), native table row655行全文review、source変更なし。scoped compile warning0/style0、元のhost-row75/75・mutation42/42・collection25/25。残35 C/headerとwhole gates未達。review174/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint55: [evidence](phase172/import/checkpoint55/README.md), CharacterData/Text742行全文review、source変更なし。scoped compile warning0/style0、host Text20/tree exit0/mutation16/range-data100/range-insert92。残34 C/headerとwhole gates未達。review175/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint56: [evidence](phase172/import/checkpoint56/README.md), HTML reflection803行全文review/属性setter GC所有と宣言順修正。full host build warning0、HTML probe40/40/object URL44/44/DOM23/23/host-form29/url exit0、style許容forward cleanup5のみ。残33 C/headerとwhole gates未達。review176/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint57: [evidence](phase172/import/checkpoint57/README.md), HTML input原本814行・修正後896行全文review。属性setterの一時文字列/atom GC所有と宣言順を修正、full host build warning0、入力probe94/94・controls100/100・DOM23/23・関連native130/130、styleは許容forward cleanup7のみ。残32 C/headerとwhole gates未達。review177/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint58: [evidence](phase172/import/checkpoint58/README.md), page sheets853行全文review。非同期URL解決失敗時の部分sheet破棄を修正、full host build warning0/style0、専用page75/75・HTTP async19/19・DOM23/23・native33/33。OOM注入は未実施。残31 C/headerとwhole gates未達。review178/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint59: [evidence](phase172/import/checkpoint59/README.md), iframe binding890行全文review。table初期化に不要なforward宣言を規約位置へ移動、source意味変更なし。full host build warning0/style0、専用page43/43・関連native242+retirement・DOM23/23。残30 C/headerとwhole gates未達。review179/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint60: [evidence](phase172/import/checkpoint60/README.md), page lifecycle/dump1038行全文review。dumpの全fallible appendを即時check、失敗layoutの部分tree破棄と前回成功layout復元を修正。full host build warning0、sheets DOM/style golden完全一致、native relayout21/position22、DOM23/23、style許容forward cleanup4のみ。OOM注入未実施。残29 C/headerとwhole gates未達。review180/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint61: [evidence](phase172/import/checkpoint61/README.md), browser shell main906行全文review。dump/--run console/PPMの出力失敗を非zero終了へ修正、normal4経路と/dev/full失敗3経路確認。full host build warning0/style0、DOM/style golden一致、DOM23/23・JS14/14。残28 C/headerとwhole gates未達。review181/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint62: [partial evidence](phase172/import/checkpoint62/README.md), DOM nodeのdoctype生成中3文字列rootと属性容量overflowを修正。full host build warning0、native204/204・DOM23/23、style許容forward cleanup2のみ。ただしbind/implementation.cとhtml/modes.cの連続文字列変換中所有が未解決で、dom/node.c全文reviewは未計上、181/209・残28を維持。次は呼び出し元を追跡・修正し再検証。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint63: [evidence](phase172/import/checkpoint63/README.md), DOM node全文reviewを呼び出し元のGC所有修正後に完了。bind/implementation.cの既レビュー箇所を再検証、html/modes.cのDOCTYPE経路を修正し同ファイル全文reviewは保留。全209hash一致、review182/209・残27。full host build warning0、XML probe62/62・native204/204・DOM23/23、style許容forward cleanup26のみ。html5lib corpus未配置、強制GC再現は未主張。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint64: [evidence](phase172/import/checkpoint64/README.md), CSS tokenizer1104行全文review。CRLFのbad-string source offset誤りを修正前後の実token比較で確認し、lookahead加算overflowを防止。全209hash一致、review183/209・残26。full host build warning0/style0、native CSS116/116・DOM23/23・values style golden完全一致。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint65: [evidence](phase172/import/checkpoint65/README.md), TreeWalker全文review・NodeFilter namespace/生成state/callback値/受理nodeの一時GC所有を修正。全209hash一致、review184/209・残25。full host build warning0、traversal実GC9/9・page70 PASS・DOM23/23、style許容forward cleanup7のみ。修正前crash再現は未主張。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint66: [evidence](phase172/import/checkpoint66/README.md), layout box tree全文review・極端な長さ/NaNのlayout unit変換を定義済み範囲へ制限。全209hash一致、review185/209・残24。full host build warning0/style0、UBSan変換5値、native relayout21/position22/table75・DOM23/23・style golden4件完全一致。下流geometry算術は別の制限。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint67: [evidence](phase172/import/checkpoint67/README.md), computed style binding全文review・公開前stateのGC所有を修正。全209hash一致、review186/209・残23。full host build warning0/style0、frame lifetime148/148・関連page68 PASS・DOM23/23。修正前GC再現は未主張。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint68: [evidence](phase172/import/checkpoint68/README.md), page script/event loop全文review・定数表位置と外部script失敗診断の確保結果を修正。全209hash一致、review187/209・残22。full host build warning0/style0、resource105/105・settle14+8・write3経路・child loading52/52・DOM23/23。resource runner初回binary指定誤りは修正後結果のみ採用。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint69: [evidence](phase172/import/checkpoint69/README.md), Window/realm全文review・定数表に必要な宣言だけ前置き、postMessageのoptions/origin/event/callbackをtimer登録までGC rootで保持。全209hash一致、review188/209・残21。full host build warning0、styleは許容のforward cleanup8件のみ。frame lifetime148/148・realm30/30・DOM23/23・settle14+8・postMessage origin5経路（4受信/1拒否）PASS。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint70: [evidence](phase172/import/checkpoint70/README.md), CSSOM1203行全文review。source変更不要、全209hash一致、review189/209・残20。GCC scoped warning0/style0、cp69 whole host build warning0。native CSSOM12/12・mutation12/12・rule model44/44、font指定のpage22/22・23/23、DOM23/23。font省略の初回pageは15 FAIL、必要fontで再実行後の結果のみ採用。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint71: [evidence](phase172/import/checkpoint71/README.md), DOM query/classList/dataset全文review・一時selector文字列、結果配列、token/map/accessorのGC rootを公開まで保持。全209hash一致、review190/209・残19。full host warning0、styleは許容のforward cleanup25件のみ。DOM23/23・child style30/30・3000要素/400 class add/remove cycles/dataset probe PASS。強制GC位置の検証なし。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint72: [evidence](phase172/import/checkpoint72/README.md), strict UTF8/XML parser1524行全文review。source変更不要、全209hash一致、review191/209・残18。GCC scoped warning0/style0、cp71 whole host build warning0。XML model236/236・DOM54/54・binding34/34・node32/32・document GC16/16。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint73: [evidence](phase172/import/checkpoint73/README.md), VM object/array全文review・length非確保判定、symbol/accessor生成、named property/reshape/array shrinkのGC rootを修正。全209hash一致、review192/209・残17。full host warning0、styleは許容のforward cleanup3件のみ。object100/0・array length4/4・VM factory GC56/56・JS14/14・DOM23/23。OOM fault injection未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint74: [evidence](phase172/import/checkpoint74/README.md), HTML tokenizer2481行全文review。source変更不要、全209hash一致、review193/209・残16。GCC scoped warning0/style0、cp73 whole host warning0。9caseのwhole/UTF16単位stream一致、token/error明示検査pass。html5lib corpusはlocalに存在せず未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint75: [evidence](phase172/import/checkpoint75/README.md), HTML insertion modes2119行全文review。source変更不要、全209hash一致、review194/209・残15。GCC scoped warning0/styleは許容cleanup5件、cp73 whole host warning0。tree focused5/5（doctype/head/body/table foster parenting/template）。html5lib corpusなしで未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint76: [evidence](phase172/import/checkpoint76/README.md), HTML parser全文review。detached elementのattribute/template割当中GC rootを追加、C literal/ABI維持、全209hash一致、review195/209・残14。full host/scoped warning0、styleは許容cleanup3件。native parser8/8・stream9/9・table row75/75・DOM23/23・tree3/3。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint77: [evidence](phase172/import/checkpoint77/README.md), VM property access全文review。literal accessor/descriptor accessor/string wrapperのGC rootを追加、C literal/ABI維持、全209hash一致、review196/209・残13。full host/scoped warning0/style0。object100/0・VM factory GC56/56・JS14/14・DOM23/23。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint78: [evidence](phase172/import/checkpoint78/README.md), Element binding全文review。hidden/script.async一時stringのGC rootを追加、C literal/ABI維持、全209hash一致、review197/209・残12。full host/scoped warning0、styleは許容cleanup4件。DOM23/23・form29/0・image geometry10/10。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint79: [evidence](phase172/import/checkpoint79/README.md), Node binding全文review。共有node-array appendでcaller arrayとowning wrapperのGC rootを追加、C literal/ABI維持、全209hash一致、review198/209・残11。full host/scoped warning0/style0。DOM23/23・insertion29/29・clone26/0・collection GC37/37・XML binding34/34。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint80: [evidence](phase172/import/checkpoint80/README.md), Document binding全文review。Cookie無host時の未初期化statusを修正し、namespace/nameと未公開node、streamから除去したchildのGC rootを追加。既存literal維持、error pathの"complete"のみ1個追加、ABI維持、全209hash一致、review199/209・残10。full host/scoped warning0、styleは許容forward cleanup11のみ。DOM23/23、namespace12/12、stream9/9、parser8/8、XML document16/16、XML binding34/34、collection GC37/37。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint81: [evidence](phase172/import/checkpoint81/README.md), inline style binding全文review。一時宣言配列とname/value・未公開accessorのGC rootを追加し全成功/失敗経路で解放。既存literal/ABI維持、全209hash一致、review200/209・残9。full host/scoped warning0、styleは許容forward cleanup35のみ。CSSOM12/12・mutation12/12・rule44/44・style source21/21・DOM23/23・collection GC37/37。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint82: [evidence](phase172/import/checkpoint82/README.md), VM interpreter全文review。arguments object/key/accessor、rest iterator、super array引数をGC保護。generator resume失敗値とloose equalityの未初期化読取りを修正。既存literal/ABI維持、全209hash一致、review201/209・残8。初回host buildのmaybe-uninitializedを修正後、full/scoped warning0、styleは許容forward cleanup13のみ。object100/0・VM factory56/56・array4/4・JS14/14・DOM23/23。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint83: [evidence](phase172/import/checkpoint83/README.md), Object builtin全文review。native vector/descriptor、converted wrapper、未公開結果、tag一時文字列をGC保護。既存literal内容/ABI維持、全209hash一致、review202/209・残7。full/scoped warning0、styleは許容forward cleanup28のみ。object100/0・native properties65/65・VM factory56/56・JS14/14・DOM23/23。全getter窓の強制GC未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint84: [evidence](phase172/import/checkpoint84/README.md), CSS parser全文review。未閉鎖属性/関数selectorと属性flag後の余剰tokenを拒否、nth数値範囲を検証、@supportsのENOMEMを伝播。DOM query異常系5件追加。既存literal内容/ABI維持（s flag literal追加）、全209hash一致、review203/209・残6。full/scoped warning0、style0、CSSOM12/12・rule model44/44・mutation12/12・JS14/14・DOM23/23。ENOMEM注入未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint85: [evidence](phase172/import/checkpoint85/README.md), Event binding全文review。新規Event/EventTarget状態とwrapper、constructor option取得、dispatch listener snapshotのGC root寿命を補強し、getter成功/失敗経路を明示。bind/input.cの隣接Keyboard/Wheel callerに未root箇所を発見したが、q597対象外のため未修正。全209hash一致、review204/209・残5。full/scoped warning0、styleは許可されたcleanup goto15件のみ、click10/10・JS14/14・DOM23/23。全経路の強制GC未試験、whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint86: [evidence](phase172/import/checkpoint86/README.md), Array helper部分review。array_hasのerror時未初期化出力、sort/joinのGC root寿命、scratch割当失敗を修正。全209hash一致、review204/209・残5（Arrayは部分review）。full/scoped warning0、styleは許可されたcleanup goto5件のみ、JS14/14。全callback強制GC・ENOMEM注入未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 / q597-terminal: archive（別セッションの記録、main には入れない）, ユーザー再起動停止によりq597-i01/p172 uncleared、WS074 incomplete。全209hash一致・204/209全文review・残5、Array部分root修正、最後のhost warning0/JS14/14/DOM23/23/click10/10。p172 whole gate未達のためp100未選定。再開はcheckpoint87照合と次の有限Queue選定。

2026-10-03 / q598-start: [selection](phase172/browser3/q598/selection.json)、ユーザーの再開指示でq597未達を有限3hへ再選定。残5全文review、input caller限定修正、元のp172 whole gate。WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint88: [evidence](phase172/import/checkpoint88/README.md), bind/input.cのKeyboardEvent/WheelEvent wrapperを追加getter中にGC rootで保持。209 inventoryの対象外なのでreview countは204/209・残5のまま、全209hash一致。whole/scoped warning0、JS14/14・DOM23/23、styleは許可cleanup goto7のみ。強制GC getter全経路未試験、whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint89: [evidence](phase172/import/checkpoint89/README.md), Array constructor/prototype、新規copy配列、pop/shift戻り値、reduce accumulatorの一時GC rootを追加。全209hash一致、Arrayは部分reviewで204/209・残5のまま。whole/scoped warning0、JS14/14・DOM23/23、styleは許可cleanup goto29のみ。全再入経路の強制GC未実施、whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint90: [evidence](phase172/import/checkpoint90/README.md), Array helperのreceiver/value、flat再帰mapped値、reverse一時値をGC rootで保護。全209hash一致、Arrayは部分reviewで204/209・残5のまま。whole/scoped warning0、JS14/14・DOM23/23、styleは許可cleanup goto33のみ。primitive wrapper等の残reviewと全経路強制GCは未実施、whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint91: [evidence](phase172/import/checkpoint91/README.md), js/builtin_array.c 2858行の全文C/component reviewと再入GC root補強を完了。全209hash一致、review205/209・残4。GCC14.2 whole/scoped warning0、JS14/14・DOM23/23、focused Array5例、styleは許可cleanup goto47のみ。全callback強制GC/ENOMEM注入は未実施。whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint92: [evidence](phase172/import/checkpoint92/README.md), bind/environment.c 3123行の全文C/component reviewを完了。MutationObserverのjob enqueue失敗時にrecordsを失わず再試行可能に修正。全209hash一致、review206/209・残3。GCC14.2 whole/scoped warning0、JS14/14・DOM23/23、style指摘0。enqueue ENOMEM注入/全callback強制GCは未実施。whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint93: [evidence](phase172/import/checkpoint93/README.md), bind/range.c 3219行の全文C/component reviewを完了、source修正不要。全209hash一致、review207/209・残2 CSS。GCC14.2 whole/scoped warning0、Range native8本373検査・JS14/14・DOM23/23、style指摘0。全allocation失敗注入は未実施。whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint94: [evidence](phase172/import/checkpoint94/README.md), css/values.c 4533行の全文C/component reviewを完了。色指定末尾の余分なtoken拒否と出力capacity検査を実装。全209hash一致、review208/209・残cascade.c。GCC14.2 whole/scoped warning0、CSS host5本128検査・JS14/14・DOM23/23、無効色4例期待通り、style指摘0。capacity故障注入未実施。whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint95: [evidence](phase172/import/checkpoint95/README.md), css/cascade.c 4227行の全文C/component reviewを完了。メディア条件の上限超過を黙って切り捨てずE2BIGに修正。全209hash一致、review209/209・残0。GCC14.2 whole/scoped warning0、CSS host128/128・JS14/14・DOM23/23、style指摘0。元のwhole final gatesは未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint96: [evidence](phase172/import/checkpoint96/README.md), 209/209最終hash・追加input.c全文確認。browser2 manifest569件はapplied209/archive356/excluded4/unresolved0、原Phase71件は履歴として保持。host plain/ASan各98回帰+81golden・Acid2 120000画素一致、独立client CPU/GPU描画、39exports/167source/377include・禁則依存0。target初回image/boot PNG合格、browser source警告0・外部package警告233行。最終warm buildのimage書込/最終boot/guest shellを残し、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-p172-clearance: [p172](phase172/phase.md)を[最終証拠](phase172/import/checkpoint98/README.md)でwhole cleared。209/209対象の全文review、branch manifest/ABI、host+ASanとAcid2、最終target bootとVenus shellを確認。初回cold p014の5秒待ち失敗・再試行PASSを保持。WS074固有のacceptance（Acid3ほか）は未達なのでincompleteのまま。p100は前提解消のみで実行Queue未選定。Agent A側とのQueue ID/共有投影・GitHub投稿は保留。
