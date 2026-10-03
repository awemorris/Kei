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
| — | タッチパッドで窓を移動中のフリーズ | **FAIL** | M650 の受信機を抜いた後、タッチパッドで窓を移動している途中で止まり、戻らなかった。→ フリーズは M650 だけの問題ではない |
| 4 | WiFi の Join | PASS | 接続できた |
| 4 | WiFi の AP の切替 | **FAIL（要修正）** | 別の AP を選んでも「Connecting...」のような表示が無く、何をしているか分からないまま約 10 秒後にチェックが付いた。user「これは問題なので修正が必要。Connecting...は必要。」 |
| 4 | WiFi の off → on で保存済みの AP に自動で再接続 | PASS | |
| 4 | 間違えた鍵の文言 | **FAIL** | 「did not accept the key」が出ず「Could not join (Network is down)」（ENETDOWN）。QEMU の RTL8822BU では EACCES だった（ws005-p020）。5330 の内蔵 AX211 の経路で鍵の拒否が ENETDOWN になっている見込み |
| 4 | off の保持（Log Out → login） | PASS | off のまま。再起動（Restart）を挟んだ保持は未実施 |
| 4 | off → on で自動接続 | PASS | |
| 5 | Terminal で emacs（BUG-150） | 対象外 | zedBSD の image に Emacs は無い（BUG-150 は FreeBSD で見つかった。Q1 の手順の誤り） |
| 5 | Terminal で日本語の入力 | **FAIL（要修正）** | Terminal で日本語が打てない。user「IME非対応なんじゃない？これは要修正。」 |
| 5 | Terminal の Treat Ambiguous-Width Characters as Wide | PASS | チェックが付く。日本語が打てないので表示の確認は未実施 |
| 6 | Text Editor で日本語・英語の入力と保存 | PASS | Desktop に保存 |
| 6 | Desktop の icon の表示（WS094） | PASS | 保存した file が 1 秒以内に icon で出た |
| 6 | Notes の起動 | PASS | 一瞬 |
| 6 | PDF Viewer の起動 | PASS | 約 1 秒（F-072 の速さの候補） |
| 6 | Settings の Wallpaper（Lakeside・Birch-Lake） | 一部 FAIL（要修正） | 2 枚とも一覧にあり、切り替えもできる。ただし Wallpaper の頁を click すると約 10 秒止まる。user「画像読み込みはマルチスレッドにしないとだめだね。要修正。」 |
