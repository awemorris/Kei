<!-- awesome-plan project=zedbsd record=ws119-p001 -->
# ws119-p001: Wayland のインストーラの要件の案と設計の案

Status: planned
Disposition: normal
Parent: [WS119](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none（未承認）
目安: 2〜3h（調査と設計、実装はしない）

## 範囲

2026-10-02 user「Waylandで動くインストーラを実装する。」

1. 旧 `/bin/zedinst`（`userland/retro/zedinst/`）の流れ・安全の仕組み（admission、preflight、transaction、atomic な公開、検証）と、再利用できる契約（[WS019](../../ws019/ws.md) の p013〜p028）を読む。
2. 現在の amd64 native の image の構成（`Makefile` の disk-image、ESP・UFS の root・swap、`zedbsd.cfg` の root の選択子）を読み、導入先へ何を複写し何を作り直すか（root の大きさを disk に合わせる、`zedbsd.cfg` の選択子、`/etc/fstab`）を決める案を書く。
3. base の道具（`diskpart`・`mkfs`・`mkswap`・`blkid`・`mount`・`pax`/`cp`）で足りるか、足りない機能を洗い出す。
4. frontend の作り: libkeiland/libkeiui の C の Wayland の client（Settings・Files の作り）に倣う案を基本とし、backend を非対話の command に分ける（frontend は進捗を読むだけ）設計案を書く。
5. 画面の流れの案（welcome → disk の選択 → 消去の確認 → 利用者・password・hostname → 進捗 → 完了・再起動）、live の session からの起動の仕方。
6. 試験の方法: QEMU の NVMe の空の disk に導入し、USB を外して起動する。

成果は `plan/ws119/design.md`（要件の案、選択肢、推奨、p002〜p004 の範囲と interface、試験の方法）。

## 受け入れ

design.md と、ユーザーへの質問の一覧（下の未決の判断）を main に返す。source は変えない。

## 所有 path

`plan/ws119/`。読むだけ: `userland/retro/zedinst/`、`userland/base/{diskpart,mkfs,mkswap,blkid,mount}/`、`Makefile`、`userland/desktop/`。

## 依存

なし。

## 未決の判断（ユーザーへ）

- 導入の方式: whole disk の専用だけか、既存の OS との共存（partition の縮小は無し、空き領域への導入）も要るか。
- 起動の方式: UEFI・GPT だけでよいか（BIOS は外す案）。
- 画面: 利用者・password・hostname の入力の要否、言語・keyboard の配列・time zone の画面の要否、UI の言語（英語のみか日本語も）。
- 起動の入口: live の session の App Home の tile か、greeter（login の前）に「Install」を出すか、専用の install の image か。
- 実機の試験の導入先（5330 の内蔵 NVMe は Linux の host なので、別の disk・USB の SSD を使うか）。
- 間に合わないときの扱い（ベータ1 から外す等）。
