<!-- awesome-plan project=zedbsd record=ws074 -->

# WS074: zedBSD の Web ブラウザ（`userland/desktop/browser`）

<!-- awesome-plan-current:start -->
Status: incomplete
Primary Milestone: MG006
Related Milestones: MG002
Objectives: O1, O2
Parent: [Master](../master.md)
Queue: q584 / A1（q579の残review、last: q579-i01 uncleared）
Resume point: p099 cleared. p172 integrated the pinned origin/browser2 changes and retained build/runtime evidence. q579 ended uncleared; q584 is now reviewing the remainder. Integrated checkpoint06 reviewed12/209, remaining137 C/header and60 other. Downstream p100/p174/p175/p173/p176/p101 remain gated by whole p172 clearance.
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
| [ws074-p172](phase172/phase.md) | origin/browser2のbrowser変更をWS107後のlibbrowser/browser配置へ取込み、関連計画/試験/bugを意味的に照合するblocking gate | in-progress / q584 A1（q579 unclearedの続き） | WS107の実成果、p099、branch snapshot |
| [ws074-p173](phase173/phase.md) | Interop 2025の公式focus area対象WPTを固定・baseline化し、全件PASSへ向けた失敗群と後続Phaseを設計 | planned / Queueなし | p172、p100 |
| [ws074-p174](phase174/phase.md) | File System Access APIの仕様/WPT・権限・picker/handle契約を確定し、機能群ごとの実装Phaseへ分割 | planning / Queueなし | p172、p100 |
| [ws074-p175](phase175/phase.md) | OPFSのorigin分離・永続保存・handle契約を確定し、機能群ごとの実装Phaseへ分割 | planning / Queueなし | p172、p174の共有契約の実出力 |
| [ws074-p176](phase176/phase.md) | Test262固定全suiteのbaselineと失敗群/runner coverage、段階的改善Phaseの設計 | planning / Queueなし | p172、JS runnerの実出力 |

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
