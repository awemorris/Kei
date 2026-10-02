# WS113 p001 / q586-i01: checkpoint evidence ledger

Date: 2026-10-02 UTC。Executor A3 / worktree `/home/awe/zedBSD-worktrees/a3` / branch `codex/a3-display`。
Queue bound: 07:12–08:42 UTC、90分。current userのA/N=3開始指示をmainがq586/approved Phaseへ捕捉。source baseline `0e68854ac`。
Outcome: in-progress。D-ATOMIC user回答/main review待ち。主要技術設計は保存、実装/実機試験は未実施。

## commits / ACK

| MR | A3 commit / base | 内容 | main read-back ACK |
| --- | --- | --- | --- |
| A3-001 | `6e34d1bb9` / `0e68854ac` | source一次能力表18行。後に汎用enumの表現を訂正して20行へ増補 | `8021bc210` WIP |
| A3-002 | `6a0a532b2` / `6e34d1bb9` | contracts/fixtures、fence/ID/座標/設定/失敗/有限matrix | A3-003と累積`8cf8a8ea6` WIP |
| A3-003 | `addd258356` / `6a0a532b2` | ID/完了保証比較、能力20行、design/foreign p002–009/WS events、main通常選択5点 | `8cf8a8ea6` WIP |
| A3-004 | `df2f36ff3` / `addd258356` | A2/旧boot採択、native capability semantic案、p001–008/WS events | `ec870f856` WIP |

本ledgerを含む次checkpointはgit commit log/MR messageで同定する。自commit SHAを内容へ埋める循環を作らない。ACKはmain integrationの証拠で、製品機能/Phase clearanceではない。

## 実行した確認とlimitations

| commands/tools | 結果 / 対象 |
| --- | --- |
| `rg -n` / `rg --files` / `sed -n` / `cat` | AGENTS、Guardrail、queue/approved phase、scoped/C full rules、automation、WS075/089/103 contextと必要actual sourceを読取。詳細source/lineはsource-audit/native-contract/identity-completion |
| Khronos一次ページopen | source-audit/contracts/identity-completion/native-contractのリンクを2026-10-02に取得、URL/revision/validity/latest変動を文書へ。標準適合CTSやactualdriver支持は主張しない |
| `git diff --check` / `git diff --cached --check` | 各checkpoint PASS。scopeはplan/ws113 docsだけ |
| Python local Markdown target存在check | plan/ws113のrelative pathリンクPASS。fragment/remoteIssue公開成功をこれで証明しない |
| Python sample key ASCII length | `zedbsd-port-v1:pci:0000:00:02.0:edp:A` 37byte+NUL38、name[64]内。prod formatter/ABI testではない |
| Phase status/event照合 | p001 in-progress、p002–009 planned、全改訂foreign Phase/WSへlocal eventあり。remote delivery/projectionはmain pending |

未実施: build、production C/HAL/UAPI変更、formatter/static source検査、aggregate make check、toolchain変更/build、hardware占有/lock取得、SSH、VFIO再bind、boot/QEMU/host変更、実機写真取得。過去hardware結果はfixture contextであり今回PASSではない。

## 採択とremaining

- main技術採択source: D-ID A2 local PCI+kind/DDI key→standard displayName、D-BOOT全extended/internal anchor+旧hdmi/edp preferred、D-LAYOUT非重複/辺連結snap、D-REC退避窓奪回無し、D-AUTH active同UID、D-PORT初回eDP+HDMI fixture。私有Vulkan identity API不採用、GPU UUIDqueryは別能力。
- D-ATOMICはuser回答前に通常owner切替/strict erase+gapのどちらも採択しない。standard present_wait/first-pixelとfull physical eraseの限界をcomparisonに保存。
- [q592候補](next-selection.md)はscope/criteria/readinessだけ。new Queue未投入、p002実装は未承認。
- main所有のcanonical Queue/Agent lane/Master/Past Log/standards/GitHub per-target eventとProject projectionを未更新としてrequest済み。local Phase/WS historyをchatで置換しない。
- Queue終端でもuser指示に従い同一agent sessionへcheckpointを返し、明示次Queueを待機する。自発final/次Queue開始をしない。
