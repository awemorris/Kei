<!-- awesome-plan project=zedbsd record=ws129-p004 -->
# ws129-p004: release の image の config と CI の release の job

Status: planning（p001 と BUG-134 の結果を待つ）
Disposition: normal
Parent: [WS129](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 3h

## 範囲

1. release の config（`config/release/config-amd64-beta1.mk` など、p001 で決める）: fg019 の成果の program（Settings・audiod・インストーラ・各 app・packages の Emacs/vim/python3 は各 WS の到達しだい）、
   AX211 の driver（BUG-134 が解決すれば y、未解決なら n で既知の問題）、demo の利用者・自動 login を入れない、初回の利用者の作り方（インストーラ／live の利用者）。
2. `.github/workflows/ci.yml` に release の job を足す（tag の push か `workflow_dispatch` でだけ、nightly の job は変えない）。img.gz・zip・SHA-256・license の一覧を載せ、本文は release notes の file。
3. local の確認: release の config で `make` と `plan/tools/boot-test.sh`、YAML の構文の確認、job の shell の部分を local で再現（gzip・sha256sum・file の名前）。GitHub での実行は p008（ユーザーの指示）。

## 受け入れ

上の 3 の結果（PNG をユーザーに見せる）。push はしない。

## 所有 path

`config/release/`（新規）、`.github/workflows/ci.yml` の release の部分（WS112 と同じ file なので main が順を調整する）、`plan/ws129/`。

## 依存

p001、BUG-134（ws004-p051）の結果、fg019 の各 WS の到達（program の一覧は凍結の前に最終化）。

## 未決の判断

p001 の判断（配布物の範囲・prerelease）。
