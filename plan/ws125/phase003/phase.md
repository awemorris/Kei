<!-- awesome-plan project=zedbsd record=ws125-p003 -->

# ws125-p003: image への導入と guest の受け入れ

Parent: [WS125](../ws.md)
Status: planning
Disposition: normal
Queue / attempts: none
Goal: vim を menuconfig で選べる package にし、image に入れ、guest で V1〜V6 を確かめる。
Prerequisites: p001 cleared、p002 の成果が main に統合済み、D1・D2 の判断（既定案で進めてよい）。
Investigation bound: 3 時間。

## 範囲

- `ZEDBSD_USERLAND_PACKAGE` で `editors/vim` を登録、binary を `--file`、runtime を p002 の tree の指定で入れる。license を `/usr/share/licenses/vim/`。
- 試験 `plan/ws125/tests/`: SSH の pty で起動・挿入・`:wq`・照合、`:syntax on` と `:help`、日本語の往復。

## 受け入れ

[WS125](../ws.md) の V1〜V6。image の増分（MiB・file 数）を記録。

## 検証

`plan/tools/guest/guest.sh`、Keiland の terminal の画面、`plan/tools/boot-test.sh`（PNG をユーザーに見せる）。

## 所有 path

`userland/packages/editors/vim/`、`plan/ws125/`、自分の worktree の `build/`。

## 依存・未決の判断

p001、p002、D1・D2。
