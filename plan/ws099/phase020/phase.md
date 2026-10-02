<!-- awesome-plan project=zedbsd record=ws099-p020 -->

# ws099-p020: BUG-125 の原因の特定と compositor の直し（ベータ1 の blocking）

Status: planned（2026-10-02 ベータ1の計画。Queue なし）
Disposition: normal
Parent: [WS099](../ws.md)
Queue: なし
依存: [p017](../phase017/phase.md) の資産（q577・q583・q589 の証拠、`plan/ws099/tests/p017-popup-low-overhead.py`・`p017-popup-observe.py`・`p017-popup-timeline.py`、`phase017/p076-framed.sh`）。Venus の renderer `build/ws035-sq-venus/install`（読むだけ）
目安: 4h（1 Queue）。実行者の目安: phase-runner（high、compositor の bug の修正）
所有 path: `userland/desktop/wayland/` のうち原因の file（見込み: `shell.c`・`popup.c`・`menu.c`・`menu-shell.c`・`toplevel.c`・`seat.c`・`compose.c`）、`plan/ws035/tests/zdesktop-p076.sh`（試験の同期の直しが要るときだけ。main に一言）、`plan/ws099/tests/`・`plan/ws099/phase020/`、[BUG-125](../../bugs/BUG-125.md) と Bug Board の該当の行

## 背景

2026-10-02 user: BUG-125 は WS099 の blocking、WS099 の担当が直す。p017 は試験の同期の切り分けに限った Phase（compositor は読むだけ）で、
q577・q583・q589 の 3 attempt が uncleared。残る症状は 3 つ（[q577 結果](../phase017/q577-result.md)、[q583](../phase017/q583-result.md)、[q589](../phase017/q589-result.md)）:

1. popup: flip した submenu が configure・map されるが、最初の PNG が暗い（orange でない）。frame の log が無い。
2. popup: 負荷の下で flip した submenu の configure も press も無く、次の click で遠い menu が map される（準備前の操作）。
3. resize: left の resize の後の geometry が期待に届かない（WS105 q532 416×400 / x674、期待 200×400 / x890。q538 の C2 の settle の不一致とも近い）。

p017 の accepted-request の同期の修正（`fb8732b9`）は main に入っている。

## 範囲

1. 現在の main で基準の image を作り（`build-criteria-image.sh`）、q589 の low-overhead helper で p076 の stage 1〜4 を最大 5 回、全体の p076 を 5 回流し、3 症状の再現と再現率を得る。
   一時的に `--log-frames` と compositor の内部の印（configure・map・commit・最初の compose・resize の settled）を log に足してよい（取った後に外すか、既定で off の log にする）。
2. 各症状について、compositor の状態の遷移（xdg_popup の configure → ack → commit → map → compose、resize の request → configure → ack → settled → compose）のどこで遅れ・取りこぼしが起きるかを log の時刻で示す。
3. compositor の defect なら compositor を直す。試験が準備前に操作している（例えば popup の map の前の click）だけと証明できたら、試験に「map と最初の frame の log を待つ」同期を足す（待ちは最大 0.5 秒 × 6、期待の geometry・pixel は弱めない）。
4. 直した後の受け入れの試験を流し、BUG-125 を resolved にする（下の条件を全て満たしたときだけ）。p017 の受け入れ（単独 20・C9 5 で FAIL 0）もここで満たせば、main が p017 を follow-up で clear にできる。

範囲の外: C2 の top-right の増分・bottom-left の settle（p021）、BUG-127（p021）、BUG-120・BUG-124、速さ（C6）。

## 受け入れ

- 同じ image で `plan/ws035/tests/zdesktop-p076.sh` の単独 20 回が FAIL 0、`sh plan/ws099/tests/criteria.sh IMG OUT C9` の 5 回で p076 を含め FAIL 0（他の本の FAIL は別に記録し、新しい FAIL なら原因の見当を書く）。
- 3 症状のそれぞれに、原因（log の時刻と code の位置）と直し（または試験の同期だけで足りる証拠）がある。待ちを伸ばして隠していない（直しの前の image で同じ試験が FAIL を出すことを 1 回以上示す）。
- 変えた compositor の source の全文規約（[coding-style.md](../../coding-style.md)、`plan/tools/style-check.py` の違反 0）、`build/amd64/bin/wayland` の build の warning 0、C2・C3・C4・C8 の回帰 PASS、boot test PASS（PNG を Q1 経由でユーザーに見せる）。
- 4h で届かなければ uncleared。症状ごとの到達点・再開条件・次の Phase の案を残す。

## 検証の方法と範囲

- QEMU の Venus だけ（[guide.md](../guide.md)「コマンド」）。QEMU の console・serial の log では判定しない。判定は zdesktop の log（SSH、`/run/user/1000/session.log`・`/tmp/zdesktop.log`）・QMP の画面・試験の exit。
- runtime・port・build は担当の worktree の中に分ける（`build/<担当>-p020-*`）。共有の `build/` と toolchain は読むだけ。
- 実機は範囲の外（実機での resize は p011 の `resize-hw.sh` が既に PASS。必要になったら別 Phase）。

## 未決の判断

- なし（直し方は通常の技術の裁量）。compositor の外（libwayland・kernel の poll）に原因があると分かったら、その時点で止めて Q1 に報告する（他の WS の source）。

## Event

2026-10-02 / ws099-beta1-plan-p020: fg019 の計画で新設。p017 の 3 attempt の資産を引き継ぎ、compositor の編集を許す Phase として分けた。p017 の記録と受け入れは不変。
