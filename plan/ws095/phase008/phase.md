<!-- awesome-plan project=zedbsd record=ws095-p008 -->

# ws095-p008: zdesktop の自前の field と Files の field

Status: planning（Files の WS127 と file の調整を main が決めてから planned）
Disposition: normal
Parent: [WS095](../ws.md)
Queue: none
Prerequisites: p005・p006、WS127（Files 最重点）との merge 順
Investigation bound: timebox 3〜4h

## 範囲

- titlebar の検索（`wayland/titlebar-shell.c`、design §4.4）で compositor 内の field に IME を結ぶ（compositor 自身が text input の相手になる経路）。
- Files（`userland/desktop/files/`）の検索・rename の field。

## 受け入れ

titlebar の検索と Files の検索・rename に日本語を入力・確定できる PNG。Files の host 試験（`plan/tools/files/`）に回帰無し、`boot-test.sh`。

## 所有 path

`userland/desktop/wayland/titlebar-shell.c`（compositor）、`userland/desktop/files/`、`plan/ws095/`

## 未決の判断

WS127 の Files の作業と同時に行うか、後にするか（main）

## Standards / evidence

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[design.md](../design.md) を変更前に読む。新しい C は全文規約。zedBSD の起動は `plan/tools/boot-test.sh` だけ、guest の操作は serial/SSH、画面は PNG で目視しユーザーに見せる。serial/console log を判定に使わない。共有 `build/` と toolchain は読取専用。全体の `make check` は禁止。QEMU の証拠と実機の証拠を分け、やっていない確認は未実施と書く。具体コマンド/結果は実行時に記録する（今は計画のみ）。

2026-10-02 / ws095-beta1-plan-20261002: 計画担当が作成。
