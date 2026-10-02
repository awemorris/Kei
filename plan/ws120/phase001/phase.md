<!-- awesome-plan project=zedbsd record=ws120-p001 -->

# ws120-p001: 設計

Parent: [WS120](../ws.md)
Status: planned
Disposition: normal
Queue / attempts: none
Goal: 音楽アプリの設計（`plan/ws120/design.md`）を作り、D1〜D4 の選択肢をユーザーに提示して決定を記録する。
Prerequisites: なし
Investigation bound: 3 時間。code は書かない。

## 範囲

- D1（形式）・D2（decoder の方針）・D3（再生の API と他 OS）・D4（機能）の選択肢・利点・危険・工数を、ws.md の既定案を出発点に design.md にまとめ、main 経由でユーザーに提示する。
- app の構成: `userland/desktop/music/`（名前は案）、libkeiui の部品（canvas・titlebar・menu・list・touch）、画面の案（一覧・再生の bar・cover）、keyboard の操作、Files からの起動の引数。
- 再生の API の案（libkeiland）: stream の open（format・rate・channels）、write（非 block、poll できる fd）、pause・resume・flush・drain、再生位置（audiod の `played_position`）、underrun の通知、audiod の切断からの回復。`KEILAND_VERSION` と exports.map。
- decoder の library の案: 置き場所（例 `userland/base/libaudio-decode`）、API（open・情報・metadata・frame ごとの decode・seek）、format ごとの file の分け方、cover の画像を compat の library に渡す方法。
- 試験: 素材の作り方（host に sudo で flac・lame・vorbis-tools を導入し、正弦波・無音・境界の長さを encode、参照の PCM を作る）、M5 の数値（MP3 の許容差）、QEMU の WAV の判定。
- [設計の敵対的レビュー](../../../.claude/agents/design-reviewer.md) の方針に従い、design.md の誤り・欠落を見直す。

## 受け入れ

- design.md があり、D1〜D4 のユーザーの決定（またはユーザーが既定案を承認したこと）と出典が書かれている。p002〜p006 の範囲・所有 path・試験が決まり、それらを planned にできる。

## 検証

文書の review（link・ID）。build・試験は無い。

## 所有 path

`plan/ws120/`。

## 依存・未決の判断

依存なし。D1〜D4 はこの Phase で提示し、決定が来るまで p001 は uncleared で待つ（他の作業は止めない）。
