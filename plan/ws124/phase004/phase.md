<!-- awesome-plan project=zedbsd record=ws124-p004 -->

# ws124-p004: stage・image への導入と guest の受け入れ

Parent: [WS124](../ws.md)
Status: planning
Disposition: normal
Queue / attempts: none
Goal: lisp・etc・DOC を stage し、menuconfig で選べる package として image に入れ、guest で E1〜E6 を確かめる。
Prerequisites: p003 cleared、[ws125-p002](../../ws125/phase002/phase.md)（staged tree の image への導入）の成果が main に統合済み、D1・D2 の判断。
Investigation bound: 4 時間。

## 範囲

- host の Emacs で `.elc`・DOC・charsets・leim を作り、`DESTDIR` の staged tree（`/usr/bin/emacs`、`/usr/share/emacs/<版>/`、`/usr/libexec/emacs/`）を作る。`.el` を入れるかは容量を実測して決める（既定案: `.elc` だけ、`.el.gz` は入れない）。
- `ZEDBSD_USERLAND_PACKAGE` で `editors/emacs` を登録（既定は D2）。license を `/usr/share/licenses/emacs/` に。
- 試験 `plan/ws124/tests/`（SSH の pty で起動・編集・保存・終了を自動で行い、内容を照合する）。

## 受け入れ

[WS124](../ws.md) の E1〜E6。起動の時間と image の増分（MiB・file 数）を記録する。

## 検証

`plan/tools/guest/guest.sh` による guest の試験、Keiland の terminal での起動の画面、`plan/tools/boot-test.sh`（PNG をユーザーに見せる）。console・serial の log では判定しない。

## 所有 path

`userland/packages/editors/emacs/`、`plan/ws124/`、自分の worktree の `build/`。

## 依存・未決の判断

p003、ws125-p002、D1（端末版だけで良いか）、D2（image の既定）。
