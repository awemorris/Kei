<!-- awesome-plan project=zedbsd record=ws128-p004 -->

# ws128-p004: PDF Viewer の文字の検索と選択・copy

Status: planning（p001 で規模の見積もりとユーザーの採否）
Disposition: normal
Parent: [WS128](../ws.md)
Queue: なし
依存: p001。`userland/base/libpdf`（WS079 の source）に文字の抽出（ToUnicode・font の encoding からの Unicode）を足す必要
目安: 4h 以上（2 Phase に分ける見込み: libpdf の抽出 → Viewer の検索と選択）（1 Queue）。実行者の目安: phase-runner
所有 path: `userland/base/libpdf`、`userland/desktop/pdfviewer/`

## 範囲

libpdf で page の文字と位置を Unicode で取り出し、PDF Viewer に Find（Ctrl+F、一致の強調と次・前）と、drag の選択と Ctrl+C の copy を足す。

## 受け入れ

（採用されたら分割して確定）: 代表の PDF（埋め込みの TrueType・CFF・Type1・CJK）で検索が当たり、copy の文字が正しい（host 試験）。

## 検証の方法と範囲

host と QEMU の Venus。 やっていない確認は「未実施」と書く。

## 未決の判断

ベータ1 に入れるか（ユーザー、規模が大きい）。

## Event

2026-10-02 / ws128-beta1-plan: fg019 の計画で新設。
