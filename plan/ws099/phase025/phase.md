<!-- awesome-plan project=zedbsd record=ws099-p025 -->
# ws099-p025: BUG-146 — Venus の guest 試験が app の client の番号を IME の有無に関わらず求める

Status: in-progress（q645、P2、2026-10-03。実装済み・T1 の試験待ち）
Disposition: normal
Parent: [WS099](../ws.md)
Bug: [BUG-146](../../bugs/BUG-146.md)

## 範囲（Q1 の q645、user 2026-10-03「P2を立てて、併走で異なるバグを修正してください」）

zdesktop は keiland-ime が入っていると READY の前に IME を自分の接続（`ZWL CLIENT client=N fd=D ime=1`）で起こし、
client の番号は 1 本の連番なので、試験が最初に起こした app は client 2 になる。試験は `client=1` などを決め打ちして
いて、IME 入りの image では全て外れる。直しは試験の側だけ（compositor は変えない）。

## 直し

- 共有の helper `plan/tools/guest/zwl-clients.sh`:
  - `zwl_app_clients [LOG]` が `zc1`〜`zc4`（試験が起こした 1〜4 番目の app の client の番号）を決める。log の `ZWL CLIENT client=` の
    行のうち `ime=1` の無いものを接続の順に数え、まだ接続していない app は見えた最大の番号の次から割り当てる。client の行が無ければ
    1〜4（これまでと同じ）。IME の行は READY の前に出るので、READY を最大 30 秒待ってから読む。後から起こし直された IME の番号も飛ばす。
  - `zwl_app_client N [LOG]` は N 番目を印字する（client の index を引数に取る試験の関数の中で使う）。
  - guest の command は `$ZWL_RUN`（無ければ `guest`）で流す。
- 試験の書き換え（81 file）: `plan/tools/{files,titlebar,x11}/`、`plan/ws035/tests/`、`plan/ws068/tests/`、`plan/ws079/tests/`、
  `plan/ws099/tests/`（cursor-owner.sh・p020-late-request.sh）、`plan/ws127/tests/`、`plan/ws128/tests/` の `client=N`（N=1〜4）を
  `client=$zcN` にし、使う前に `zwl_app_clients`（zdesktop を起こし直す行・関数の境で計算し直す）。単一引用の中は二重引用に
  （中に `"`・`$` があるものは `'"$zcN"'` で継ぐ）。client の index を取る関数（`window`・`place`・`tile`・`tab_line`・`item_x` など）
  の中の `client=$1` は `client=$(zwl_app_client $1)`。`zdesktop-p084.sh` の `drag drop … target=2` も `target=$zc2`。
  `plan/ws089/tests/` は ws089-p010 の `find_window` のまま。`plan/ws0NN/phaseNNN/evidence/`・`temp/` の古い script は履歴なので変えない。
- IME 入りの Files の image の config `plan/tools/files/config-amd64-files-ime.mk`、`build-files-image.sh` は `FILES_CONFIG` で config を替えられる。

## 確かめ

- host: helper を偽の log 5 通り（IME 無し・IME だけ・IME と app 1 つ・IME の起こし直し・IME 無しの app 2 つ）で、期待の番号（`1 2 3 4`・
  `2 3 4 5`・`2 3 4 5`・`2 4 5 6`・`1 2 3 4`）。`zwl_app_client` も同じ。書き換えた 81 本は `sh -n` を通る。
- QEMU（T1 に依頼）: IME 入りの Files の image で files-p002・titlebar-p011・zdesktop-p084・zdesktop-p062 が PASS、IME 無しの image で
  files-p002 が PASS。結果は未着。
