<!-- awesome-plan project=zedbsd record=ws119-p003 -->
# ws119-p003: Wayland の frontend

Status: planning（p001 の判断、p002 の interface を待つ）
Disposition: normal
Parent: [WS119](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 3〜4h

## 範囲

p001 で決めた画面の流れを、Keiland の Wayland の client として C で作る（libkeiland/libkeiui、Settings・Files と同じ窓の作り、名前の規則: 画面の文字は「Kei」）。
disk の一覧と大きさ、消去の確認（disk の名前と大きさを示して明示の確認）、利用者などの入力、進捗（p002 の backend の行を読む）、失敗の理由、完了と再起動。

## 受け入れ

QEMU（GPU 無しの framebuffer の Keiland、必要なら Venus）で各画面の PNG、偽の backend（進捗・失敗を返す）で全部の分岐を通す。build warning 0、全文規約。

## 所有 path

新しい frontend の directory（例 `userland/desktop/installer/`）、`plan/ws119/`。

## 依存

p001、p002 の interface（p002 と並行してよいが、interface が固まってから）。

## 未決の判断

p001 の結果による。
