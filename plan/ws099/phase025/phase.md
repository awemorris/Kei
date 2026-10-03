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

## T1-026 の結果（2026-10-03）と直し

- QEMU（T1-026、SHA 32bf5d810）: IME 入りの image で files-p002・zdesktop-p084・zdesktop-p062 PASS、`ime=1` の行 1 本（IME が client 1）、IME 無しの image で
  files-p002 PASS。**titlebar-p011 だけ FAIL**（2 回とも `GLASS undock surface=8 MISSING`、他の項目は client の番号の置き換えで全て ok）。
- 原因: client の番号でも surface の番号でもなく、undock の double click の固定の座標 `double 185 17`。docked の system bar では窓の tab が x=157 から
  並び（README.md 157〜270）、185 は tab の上で、tab が press を取るので bar の title の double click（`shell.c` の `bar_press`）に届かない。IME とは関係なく、
  docked の tab の strip が入った後の試験の古さ（試験の直し）。titlebar-p013 の `pointer move 185 17` と titlebar-p010 の `double 190 17` も同じ形。
- 直し: 3 本に `docked_free_x CLIENT_INDEX`（log の `where=docked` の strip・control の右端の最大 + 40、無ければ 190）を足し、undock の double click をそこに。
  T1-026 の log で 560（"+" の右端 520 + 40、最小化の button の手前）。host で helper を確かめた。再試験は T1 に依頼。
