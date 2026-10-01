# WS107 設計案: engine と app の所有

## 確認した現状（source01c754a0）

- browser/Makefile は main と shell の7 C source を program として登録し、desktop/libbrowser を依存に持つ。
- libbrowser/Makefile は KEILAND_BROWSER_DIR=desktop/browser として engine source を登録。exports.map が browser_view/offscreen/add_ca/script_tool を公開。
- public header は desktop/keiland/browser.h、API version2。opaque view、標準 Vk target/device、pointer/key/wheel/focus、poll/timer/callback の契約が既にある。
- WS074 p054〜p057 の成果は保持する。新しいレビューは配置と component の品質を改善する目標で、過去の clearance を計画だけで無効化しない。

## 移す / 残す

| 元 browser 内 | 所属先 | 扱い |
| --- | --- | --- |
| base, vm, js, html, dom, css, bind, net, page, view, image, text, layout, paint | libbrowser の同名 module | 全 tracked source/private header/table/shader。js/tool.c は library の CLI support |
| tools | libbrowser/tools | engine table の生成器/固定 hash/notice を維持。build を network 依存にしない |
| main.c, shell | browser | CLI/window/tab/titlebar/presentation/input adapter、public API のみ |
| data | browser/data | start/about/text の app の開始頁 |
| distfiles | 未確定、tracked inventory で分類 | 取得 cache を source として取り込まない。必要な固定仕様データだけ provenance と所有で決める |
| public browser.h | desktop/keiland/browser.h のまま | 所在変更はこの提案で必要ない |
| licenses/browser | 現 install の notices を保つ | source の所属変更で license/provenance を落とさない |

新 source root に合わせ Makefile・private include・生成器・現役 runner を更新。browser/libbrowser の共用変数名は独立にし、二重の compile/登録を防ぐ。
全文の [境界規則](../standards/browser-component.md)が正本。

## 品質点検と修正の上限

create/load/destroy の所有・失敗時 cleanup、viewごとの VM/DOM/timer/network state、callback 中の変更/破棄の扱い、render target と command buffer/fence の lifetime を調べる。
pointer/key/wheel/focus の抽象 API と shell の変換を確認。touch/IME に raw protocol が必要なら adapter/public 契約を設計するが、不要な API 全面改変はしない。
標準 Vulkan だけの第2 client（Wayland library 無し）を使い、実 GPU/offscreen の描画と CPU reference を区別。
発見した不備を列挙し p003 の有限 scope/verification を確定してから実行。未確定の「全部直す」を Queue にしない。

## 他 WS

WS074 p100→p101 の目標/順は保持。今後の WS074 は境界規則を適用し、実 source locator を確認してから Queue を選定。
WS107 移動完了後に WS074 の現役 host runner が新経路を使える状態を渡す。p100/p101 の実装をここへ取り込まない。
WS106 の browser-probe は移動前/後の実所在を使う。同一 runner を両 WS で同時に編集しない。
Linux browser 全体の移植と package 化はこの source 移動の実証に必要か p001 で確認し、追加の production 対応を黙って含めない。
