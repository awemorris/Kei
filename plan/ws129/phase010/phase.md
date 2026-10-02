<!-- awesome-plan project=zedbsd record=ws129-p010 -->
# ws129-p010: desktop の全 app を CI と試験の config に入れる

Status: planned
Disposition: normal
Parent: [WS129](../ws.md)

## 範囲（2026-10-02 user「イメージにSettingsアプリが入っていませんでした。CIもテスト用も、desktopのアプリはすべてコンフィグを追加しておいてください。ただしCIではテストは含みません。」）

1. `userland/desktop/` の app（Settings・audiod など、App Home の apps.conf に載る全 app とその daemon）を一覧にし、`config/ci/config-amd64.mk`（と他の CI の desktop を持つ config）の `ZEDBSD_USER_PROGRAMS` に足す。CI には `userland/tests/` の試験は入れない。
2. 試験用の config（`plan/ws035/tests/config-amd64-userland.mk`、各 WS の試験 config、デモの config）にも全 app を足す（試験は従来どおり）。
3. 足した後の image の build（warning 0）、root partition（1 GiB）・inode に収まるか、`boot-test.sh`。App Home の全 tile が起動できるかを QEMU の Venus で確かめる。
4. `config/ci/` の変更はこの Phase で main が許可する（ユーザーの指示）。

## 所有 path

`config/ci/`、`config.mk`（desktop の app の行だけ）、試験とデモの config、`plan/ws129/phase010/`。


## 2026-10-02 user の追加

「/bin/testを入れてください。というか、テストでないbaseはすべて入れてください。」→ 範囲に追加: `userland/base/` の試験でない program（`/bin/test` を含む）をすべて CI と試験用の config に入れる。`userland/tests/` の試験は CI に入れない。追加した program の一覧と、root partition・inode に収まるかを記録する。
