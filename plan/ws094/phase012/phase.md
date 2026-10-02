<!-- awesome-plan project=zedbsd record=ws094-p012 -->

# ws094-p012: L5 実機（5330）での L1 の手順と L3 の (a)(c)

Status: planned（2026-10-01 に phase.md を作った。手順は下）
Disposition: normal
Parent: [WS094](../ws.md)
Queue: なし
依存: p009（QEMU の値まで）、実機（Dell Latitude 5330）とユーザーの時間
実行: ユーザーが 5330 を起動・操作し、agent が SSH で log を読んで記録する。agent は実機の設定を変えない。

## 範囲と受け入れ

- L1 の操作（icon の表示・double click で開く・右 click の menu・名前の変更・Trash・New Folder）が 5330 で働く（ユーザーの目と写真）。
- L3 の (a) 起動 → `DESKTOP ready` ≤ 1500 ms と (c) click → 選択の frame ≤ 50 ms（3 回以上の中央値）、`SLOW-FRAME` 0。
  Q1 の判断（2026-09-30）で L3 の合否はここで決める。(b) は参考。
- 結果で p009 の扱い（実機で以内なら follow-up で clear、超えれば新しい Phase）を main に報告する。

## 手順（2026-10-01 追記）

素の 5330（USB から単独の起動。passthrough ではない）の手順は [plan/tools/hw5330/README.md](../../tools/hw5330/README.md) §2（流れ・ユーザーに頼む確かめ・返るもの）と §3（demo の image）。
SSH の関数（以下の手順で使う）:

```
IP=<ユーザーから聞いた素の 5330 の Kei の IP（DHCP。前例 10.0.30.5）>
ssh5330() { ssh -i plan/tmp/guest/id_ed25519 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@"$IP" "$@"; }
scp5330() { scp -i plan/tmp/guest/id_ed25519 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "$1" root@"$IP":"$2"; }
```

（[README](../../tools/hw5330/README.md) §2.3 の形。README では「未確認」の command。centris から届かないことがある。**入ってよいかをユーザーに確かめてから**入る。
root の password は `root`、kei は `kei`。）

1. image（agent、host）: WS075 の demo の image に p011 までの Files が入っていることを確かめて作る。
   ```
   mkdir -p build/ws094-p012
   sh plan/ws075/demo/build-demo-image.sh build/ws094-p012-demo > build/ws094-p012/demo-build.log 2>&1; echo "exit=$?"
   ```
   PASS: `exit=0`、最後の行 `demo image: build/ws094-p012-demo/hdd-image.img`。他の image の build と同時に走らせない。
2. ユーザー: USB に書いて 5330 を起動する（kei の autologin で session が始まる）。
3. L1（ユーザーの操作、agent は log を読む）: `~/Desktop` に file を置く。
   ```
   ssh5330 'su kei -c "mkdir -p ~/Desktop/Projects; echo hello > ~/Desktop/notes.txt; cp /usr/share/keiland/wallpapers/*.ppm ~/Desktop/ 2>/dev/null; ls ~/Desktop"'
   ```
   （`su` が無い・効かない時は root で作り `chown -R kei` する。kei の home の path は `ssh5330 'grep ^kei: /etc/passwd'` で確かめる。）
   2 秒の監視で icon が出る。ユーザーが double click（notes.txt → Text Editor、Projects → Files）、右 click の menu、F2 で名前の変更、Delete で Trash を行い、写真を撮る。
   agent は `ssh5330 'grep -a "ZFILES DESKTOP\|ZFILES OPEN\|ZFILES ACTION" /run/user/1000/session.log | tail -40'` で操作の行を確かめる
   （log の path は `userland/desktop/sessiond/session.c:589`。kei の uid が 1000 でなければ `ls /run/user`）。
4. L3 (a): 100 項目を作り、session を起こし直す（Log Out → autologin か login）。100 項目の作り方は `files-desktop-guest.sh` の `perf100`（419 行〜）の
   guest の命令を写す（画像 20・folder 10・text 70）。HOME を kei の home にする。
   ```
   ssh5330 'grep -a "ZWL DESKTOP start \|ZWL DESKTOP drawn \|ZFILES DESKTOP ready items=10[0-9] \|ZFILES DESKTOP startup " /run/user/1000/session.log | tail -8'
   ```
   (a) = `ready items=100` の `at_ms` − 直前の `ZWL DESKTOP start` の `at_ms`。(a') = `drawn at_ms` − `start at_ms`（参考）。Log Out と login を 3 回繰り返して 3 つ取る。
5. L3 (c): ユーザーが folder の icon を 1 つずつ 9 回 click する（1 秒あける）。
   ```
   ssh5330 'grep -a "ZFILES DESKTOP select-frame \|ZFILES SLOW-FRAME\|SLOW-FRAME" /run/user/1000/session.log | tail -12'
   ```
   中央値を出す。`SLOW-FRAME` の行の数を数える。
6. (b)（参考）: `ssh5330 'su kei -c "echo x > ~/Desktop/added-1.txt"'` の後の `ready items=101 … newest_age_ms=` を読む。
7. 記録: この phase.md に、image（commit）、値の表（(a)(a')(b)(c)、3 回と中央値）、写真の場所、**QEMU の証拠と実機の証拠を分けて**書く。

## 完了の条件

- L1 の 6 つの操作が実機で働いた（写真と log の行）。
- (a) の中央値 ≤ 1500 ms、(c) の中央値 ≤ 50 ms、`SLOW-FRAME` 0。超えたら uncleared にし、値と内訳（`DESKTOP startup` の行、`select-frame` の `submit=`・`queue=`・`wait=`）を
  記録して main に報告する（新しい Phase の判断は main）。
- 実機で見つけた不具合は bug の起票を main に頼む（subagent は Bug Board の新しい行を作らない）。

2026-10-02 / ws094-beta1-plan-p012: fg019 の計画で planned を維持。WS099 p012（C1 の目視）・WS089 の S7・WS079 の S8/S9・WS100 の A7 と同じ demo の image・同じユーザーの時間にまとめることを Q1 に提案。結果で p009 の扱い（以内なら follow-up で clear、超えたら p015）を決める。
