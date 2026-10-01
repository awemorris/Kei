# Browser component の境界（全文の scoped standard）

Authority: 2026-10-01 ユーザーのレビューコメント第2項。[出典](../reviews/2026-10-01-review.md)。
以前の WS074 §19 を具体化し、engine source を browser に残す配置方針を置き換える。
C 全文 coding-style.md は変更しない。この全文規則は browser/libbrowser とその client/試験の設計・生成・最終レビューに追加適用する。

1. `desktop/libbrowser/` が HTML/DOM/CSS/JS/VM/network/image/text/layout/paint/view の implementation を所有する。private header、生成済み表、shader と生成器もその所属。
2. `desktop/browser/` は library のブラウザコンポーネントを窓とタブに包むアプリ。main、shell、app の開始頁等のデータを所有する。engine を app に再コンパイルしない。
3. browser は libbrowser.so を利用し、public `<browser.h>` を通す。shell は engine の private header/struct を参照しない。
4. libbrowser は **Wayland を利用不可**。public/private の header/source・直接の link・実行時の呼出しに Wayland/xdg/Keiland の窓・protocol の依存を入れない。標準 Vulkan の実装の内側の WSI/driver 依存は別の境界だが、libbrowser はそれらの Wayland API を要求/呼出ししない。
5. libbrowser は **標準 Vulkan を利用可**。呼び出し側が device/queue/target/command buffer を与える契約と、両者の lifetime/sync/resource ownership を明記する。Wayland WSI は caller または Vulkan library の下の責務。
6. shell が Wayland のイベントを coordinate/key/text/button/scroll/focus 等の抽象化された public input に変換する。libbrowser の API に wl_* 型・protocol event/opcode/OS device を漏らさない。
7. opaque view、callback、入力、poll/timer、load/navigation と render の public 契約を文書化し、window/tab ごとの状態と解放を独立させる。ABI を変更するなら影響・version と利用側の変更を計画に記録する。
8. private 実装/公開 exports は境界を守る。Wayland を link しない第2の client、標準 Vulkan renderer、shell の入力 adapter、複数 view と破棄/resize を検証する。

## 自動化と手動確認

現在の public browser.h は Vulkan/stddef/stdint だけ、browser は既に libbrowser.so に link。
engine dirs の include/name 走査で Wayland/shell/uapi/vkZed の直接参照は見つからなかった（2026-10-01 source01c754a0、これは全依存の証明ではない）。
WS107 p001 で exact source/link closure を固定し、p004 で public-header standalone compile、ELF DT_NEEDED/undefined/export、include dependency、独立 client の実描画を確認。
runtime の隠れた依存・所有・callback/再入・tab 状態・Vulkan の仕様遵守は全文/manual review が要る。新しい checker の実装は WS107 の承認済み Phase 内だけ。
簡約版は無し。全文を直接読む。source 移動は WS107 未実施なので、現 source が配置規則を満たしたとは記録しない。

## Callback / ownership 契約の確定（2026-10-02）

ユーザー回答「同じ view の変更・破棄は callback 後に行う契約にする」。callbackは外側engine call内で同期実行。pure queryと別viewの操作は許可、同じviewのmutation/draw/layout/process/destroyは外側callが戻った後。layoutを誘発するdocument_height/scroll_rangeも再入対象。query/callbackのstringは借用、後まで保つならcopyする。options/callback構造体はcopy、font文字列とstack_baseはview lifetimeを超える。API v2のstruct/exports/SONAMEは維持する。public headerに具体的query一覧、寸法/stride/target入力契約とcaller fence/target/deviceの所有を明記する。
