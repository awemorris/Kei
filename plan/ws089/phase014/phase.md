<!-- awesome-plan project=zedbsd record=ws089-p014 -->

# ws089-p014: Network の頁を 5330 の Wi-Fi で

Status: planning（WiFi の driver の成果（BUG-134 の AX211、5330 の WiFi）が要る）
Disposition: normal
Parent: [WS089](../ws.md)
Queue: なし
依存: ネットワークの WS の成果、実機とユーザーの時間
目安: 2h + ユーザー 20 分（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `userland/desktop/settings/network.c`・`page-network.c`（直しが要るときだけ）、`plan/ws089/`

## 範囲

5330 で Wi-Fi の一覧・新しい network への鍵の入力・接続・切断・入り切り（受け入れ 2 の実機の分）。不具合は直すか、networkd 側なら担当の WS へ渡す。

## 受け入れ

ユーザーの目視と SSH の networkd の状態で、接続・切断が通る。QEMU と実機の証拠を分ける。

## 検証の方法と範囲

5330 の素の起動。 やっていない確認は「未実施」と書く。

## 未決の判断

なし（前提の成果の待ち）。

## Event

2026-10-02 / ws089-beta1-plan: fg019 の計画で新設。
