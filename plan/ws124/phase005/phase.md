<!-- awesome-plan project=zedbsd record=ws124-p005 -->

# ws124-p005: pdump による起動の短縮

Parent: [WS124](../ws.md)
Status: planning
Disposition: normal
Queue / attempts: none
Goal: p004 が `--with-dumping=none` で起動に時間がかかる場合、`emacs.pdmp` を作って起動を短くする。
Prerequisites: p004 cleared。p004 の起動の時間を見て要否を決める（planning の理由）。
Investigation bound: 3 時間。

## 範囲

- `emacs.pdmp` の作り方を選ぶ: (a) build の中で QEMU の guest を起動して `temacs --temacs=pdump` を走らせ、結果を stage に取り込む、(b) 初回起動で利用者の directory に作る、(c) install の後に一度だけ作る service。既定案は (a)（image が読み取り専用でも使え、再現できる）。
- pdump の fingerprint（`make-fingerprint`）が target の binary と合うこと。

## 受け入れ

- QEMU で `emacs -nw` の起動（`*scratch*` が出るまで）が 1 秒以内（目安、p004 の値と並べて記録）。E1〜E4 を再び通す。

## 検証

p004 の試験と時間の計測。boot test。

## 所有 path

`userland/packages/editors/emacs/`、`plan/ws124/`、自分の worktree の `build/`。build の中で QEMU を使う場合の道具は `plan/ws124/tests/` か package の directory。

## 依存・未決の判断

p004。起動の時間を許容するなら canceled（理由を残す）。
