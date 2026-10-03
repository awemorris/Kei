<!-- awesome-plan project=zedbsd record=ws134-p001 -->
# ws134-p001: システムモニターの設計

Status: in-progress（q649、P2）
Parent: [WS134](../ws.md)

## 範囲

設計の文書（`plan/ws134/design.md`）を作る。code は書かない（調査の小さな probe は可）。

1. 情報の出どころ: zedBSD で CPU（各 core の使用率・周波数）、GPU（Venus と i915 の Util・VRAM・温度・電力）、memory（Used・Cache・Available・Swap）、Network（interface ごとの RX/TX）、Disk（読み・書き・Latency）、温度・電池が今どこから取れるか（/dev/system、sysctl、procfs 相当、driver の ioctl など）。足りない物は、kernel・libkeiland の公開 API（docs/architecture/keiland.md の方針: app は libkeiland 経由で system に触れる）への追加の案。Linux・FreeBSD の backend での出どころも表に。
2. 画面: 参考の画像の再現ではなく、その要素とコンセプトを取り入れた構成（user「イメージの通りじゃなくてよくて、要素を採用してほしいです。」）。候補の要素（上段のサマリーのプレート、中央の状態コア、CPU core のタイル面、GPU のモジュールカード、Network・Disk の流れ、最近の出来事）、層の奥行き、色と質感の token、文字の大きさ（タブレット）。
3. 3D と動き: 状態コア・タイル面・流れの表現の具体（形、状態との対応、動きの速さ）、数値の更新・閾値・カードの展開の動き。描画の方式（libkeiui の上に Vulkan で描く、compositor の glass との関係）と frame の予算。
4. Phase の分け方と各 Phase の受け入れ（QEMU の Venus と実機の確かめ方）。

## 受け入れ

design.md が上の 1〜4 を含み、design-reviewer の敵対的レビューを通す。ユーザーのレビューに出せる形（画面の案の図を含む）。
