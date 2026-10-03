# 安定版 S1 の実機試験の手順（2026-10-03 17:00、Latitude 5330）

## image

| 項目 | 値 |
| --- | --- |
| path | `/home/awe/zedBSD-claude1/build/s1-pre/hdd-image.img` |
| 大きさ | 2,216,689,664 bytes |
| sha256 | `c637cb6474093cb982780a42aff4267aa6bcdcf5b45c4612cbd3f2848665f7a4` |
| source | main `6ecf801cc`（2026-10-03 の変更を 1 つにまとめた commit。image を作った時点からソースの差は無い） |
| 構成 | `plan/ws075/demo/build-demo-image.sh build/s1-pre`（CI の amd64 構成が土台、passthrough の VBT 無し） |
| boot の行（ESP の `/zedbsd.cfg`） | `kernel=vmunix` `rootpart=PARTLABEL=zedBSD-root` `swap0=PARTLABEL=zedBSD-swap` `logo=logo.ppm` `login=graphical` `kmsg=quiet` `display=edp`（同じ名前の行を 2 回書くと起動が止まる） |
| 利用者 | root / root、kei / kei（kei は起動で自動の login） |
| QEMU の boot-test | PASS（`build/s1-boot/login.png`） |

## 準備

1. USB に書く（例 `sudo dd if=… of=/dev/sdX bs=4M conv=fsync status=progress`）。書いた後に読み戻して sha256 を比べると確実。
2. 5330 を UEFI で USB から起動する。有線は USB の LAN（RTL8156）をつなぐ。
3. IP は DHCP（前例 `10.0.30.5`）。エージェントが入る: `ssh -i plan/tmp/guest/id_ed25519 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@IP`。
4. WiFi の鍵はユーザーが 5330 の console（または Settings）で入れる。エージェントは鍵を扱わない（`net wifi add SSID` は鍵を tty から読む）。

## 試験の項目（WS133 の S1 の表）

| # | 項目 | 誰 | 見ること |
| --- | --- | --- | --- |
| 1 | 起動・LCD の引き継ぎ（WS084） | ユーザー | 黒い画面・文字の console が出ずに greeter／デスクトップ |
| 2 | C1: login → Log Out → Shut Down（BUG-119・122） | ユーザー | 切り替えで黒い画面が出ない、Shut Down で電源が切れる |
| 3 | USB マウス（Logi Bolt、BUG-105）・タッチパッド・keyboard | ユーザー | 動く |
| 4 | WiFi（WS005）: add・誤った鍵の拒否・modify・`--auto no/yes`・AP の切替・off の保持（再起動）・接続中の delete | ユーザーが鍵、エージェントが SSH で `net wifi list`・`net show`・ping | 文言（"did not accept the key" など）、lease、ping |
| 5 | Settings・system bar からの join（BUG-149 の EIO が出ない） | ユーザー | 接続する、エラーが EIO でない |
| 6 | 有線（RTL8156）の DHCP・抜き差し（WS033 L4） | ユーザー | 抜くと外れ、挿すと取り直す |
| 7 | desktop の icon・Files の主な操作（WS094 p012、WS127 p007、BUG-141・142） | ユーザー | 速さと操作 |
| 8 | 標準アプリの通し（WS128 p007）、Notes・PDF Viewer の S8・S9、Terminal で `emacs -nw`（BUG-150）、CJK Ambiguous Width | ユーザー | 開く・編集・保存・閉じる、Emacs が 1 行目から |
| 9 | Settings の S7・透明度（WS089 p011）、IME（A／あ、候補の窓） | ユーザー | 表示と切り替え |
| 10 | 背景の Lakeside・Birch-Lake が Settings の一覧にある | ユーザー | 選べる |
| 11 | （任意）speaker・headphone（WS100 A7） | ユーザー | 鳴る |
| 観察 | AX211 の DHCP・5GHz（BUG-145・134） | エージェント（SSH） | lease が取れるか |

結果は WS133 の p002 に「実機（素の 5330、ユーザー）」として、QEMU の証拠と分けて書く。FAIL は Bug にして元の WS へ。
