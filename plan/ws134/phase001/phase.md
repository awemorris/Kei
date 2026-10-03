<!-- awesome-plan project=zedbsd record=ws134-p001 -->
# ws134-p001: システムモニターの設計

Status: cleared の提案（q649、P2、2026-10-03。design.md を作り design-reviewer の 25 項目を反映。user「確認しなくていい」）
Parent: [WS134](../ws.md)

## 範囲

設計の文書（`plan/ws134/design.md`）を作る。code は書かない（調査の小さな probe は可）。

1. 情報の出どころ: zedBSD で CPU（各 core の使用率・周波数）、GPU（Venus と i915 の Util・VRAM・温度・電力）、memory（Used・Cache・Available・Swap）、Network（interface ごとの RX/TX）、Disk（読み・書き・Latency）、温度・電池が今どこから取れるか（/dev/system、sysctl、procfs 相当、driver の ioctl など）。足りない物は、kernel・libkeiland の公開 API（docs/architecture/keiland.md の方針: app は libkeiland 経由で system に触れる）への追加の案。Linux・FreeBSD の backend での出どころも表に。
2. 画面: 参考の画像の再現ではなく、その要素とコンセプトを取り入れた構成（user「イメージの通りじゃなくてよくて、要素を採用してほしいです。」）。候補の要素（上段のサマリーのプレート、中央の状態コア、CPU core のタイル面、GPU のモジュールカード、Network・Disk の流れ、最近の出来事）、層の奥行き、色と質感の token、文字の大きさ（タブレット）。
3. 3D と動き: 状態コア・タイル面・流れの表現の具体（形、状態との対応、動きの速さ）、数値の更新・閾値・カードの展開の動き。描画の方式（libkeiui の上に Vulkan で描く、compositor の glass との関係）と frame の予算。
4. Phase の分け方と各 Phase の受け入れ（QEMU の Venus と実機の確かめ方）。

## 受け入れ

design.md が上の 1〜4 を含み、design-reviewer の敵対的レビューを通す。ユーザーには Q1 が知らせるが、実装はレビューを待たずに始めてよい（2026-10-03 user「P2はコードを書いてOKだと思います。衝突しないです。」）。

## 結果（2026-10-03、P2）

- [design.md](../design.md): 情報の出どころ（zedBSD・Linux・FreeBSD の表、kernel の追加 K1〜K5、WS131 の backend・拡張・libkeiland の API の案と重なり）、
  stub の model と stub の項目の表（印は付けない、user の決定）、画面の構成（[layout-mock.png](../design/layout-mock.png)）、3D・動き・操作、描画の方式と
  frame の予算（隠れた窓の frame の許可を含む）、Phase（p002〜p010、Q1 が ID を割り当て）と受け入れ、試験の道具。
- 調べ: Explore の subagent 2 つ（zedBSD の統計の出どころ、libkeiland・libkeiui・app の描画の構成）。probe の code は書いていない。
- design-reviewer（敵対的レビュー）: 高 5・中 13・低 7 の 25 項目。全て design.md に反映した（§8 の表）。
- 未実施: Linux・FreeBSD の出どころは一般の知識と Linux の host の確認だけ。`KERN_SYSTEM_GET_VMSTAT` の所要時間、i915 の timestamp の対応は未測定。
