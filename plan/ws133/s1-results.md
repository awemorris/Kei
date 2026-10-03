# 安定版 S1 の実機試験の結果（2026-10-03、Latitude 5330、素の USB 起動、ユーザー）

image: build/s1-pre/hdd-image.img（sha256 c637cb64…f7a4、main 6ecf801cc）。証拠は実機（ユーザーの観察）で、QEMU ではない。

| # | 項目 | 結果 | メモ |
| --- | --- | --- | --- |
| 1 | 起動・LCD の引き継ぎ（WS084） | PASS | デスクトップが出た |
| — | 放置の後のフリーズ | **FAIL（新しい不具合）** | 再 login の後 1 分ほどでマウスが効かなくなり、電源ボタンの長押しが要った（ユーザーの見立て: kernel が止まった）。Logi Bolt の M650 を挿したまま放置すると起き、マウスの受信のランプも消える。マウスが無ければ起きない → USB（Logi Bolt の受信機、xHCI・HID）の見込み |
| 2 | C1: Log Out → login → Log Out | 切り替えの黒画面・文字の console の有無は聞き取り中 | 操作は通った |
| 2 | Shut Down（BUG-119） | **FAIL** | 画面は消えたが電源は落ちなかった。強制電源オフ |
| 3 | USB マウス（M650、Logi Bolt、BUG-105） | PASS（動く）／放置で上のフリーズ | pointer・click は動作 |
| 3 | タッチパッド | 一部 FAIL | pointer・click は OK、**スクロールが効かない** |
| 3 | キーボード（App Home の絞り込み） | PASS | |
