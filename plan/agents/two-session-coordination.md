# Temporary A/B coordination / 2026-10-02

Agent A checkout: `/home/awe/zedBSD-claude1`, current branch `codex/agent-a` (user/environment branch choice; replaces startup document's main branch assumption).
Agent B checkout: `/home/awe/zedBSD-claude2`, independently owned by B.

For this first N=3 cycle, reserve q581–q583 for B and q584–q586 for A. B's local records remain B-owned drafts; A does not mutate B's checkout. Subsequent numeric IDs are allocated by A after read-back of both lane records. If B already allocated different IDs, preserve both records and reconcile; never renumber history cosmetically.

A active scope: q584/WS074 p172 final review, q585/WS112 p001 contract research, q586/WS113 p001 display design research. A2/A3 edit only their WS documentation and evidence. A1 edits browser/libbrowser and WS074 source/test evidence. B owns its assigned desktop/GTK/bug work; no A runtime claims shared hardware.

No next Queue is automatically authorized. WIP commits, no push. Shared plans/GitHub remain A-owned; B projection changes are handed to A in commits for review/integration.

Each `approved-phase.md` is a byte-preserved approval snapshot. Its relative links retain the original Phase directory as their base, specified by the `Phase:` link in the accompanying lane Queue; resolve links there instead of rewriting the approved bytes. The current Phase and lane are the navigable records. All six snapshot hashes were checked on A after B record import.

2026-10-02 / B-next-ID-reservation: current userが「q581の後、WS114の装飾モード実装・GTK4確認を追加承認」とBから共有し、次Queue IDの予約を依頼。**q587をB1専用に予約**する。q581の後続で、Bがexact Phase/scope/criteria/承認原文/snapshot/依存を作成してAへcheckpointを送る。予約だけではactive attemptを投影せず、現在のB1 Queueはq581のまま。次の未予約main Queue IDはq588。browser2-evidence内の旧branch q587等はbranch取込の出典記録であり、本main Queueの履歴やclearanceと混同しない。

2026-10-02 / continuous-dispatch-and-B-next-IDs: current userが同じsubagentへ後続Queueを連続投入し長時間稼働を指示。q581/q583の終端でもB workerを終了させず、same runtime/contextに次Queueを送る。Bの依頼により**q588をB2 WS094最終全文規約レビュー、q589をB3 BUG-125追加切り分け用に予約**する。各exact scope/criteria/Phase/snapshotはBが作成してAへ送付、予約だけでは実行状態を主張しない。Aの後続IDとしてq590〜q592を確保し、次の全体未予約IDはq593。旧branch browser2 evidenceの同番号は出典namespaceで別記録として保持。

2026-10-02 / B2-wallpaper-next-ID: current userの追加共有指示により **q593をB2専用、q588後続のWS099 p019背景資産・3 OS収録用に予約**。既存の白樺・湖の背景を共通sourceへ収録し、見つかった場合は直線的な抽象版も含める。対象はzedBSD/Linux/FreeBSD共通収録の計画、発見条件・出典/利用条件・資産一覧・各OSの登録/build/install検証をBがPhaseとexact snapshotに定める。Phase草稿はB最新4b655803に未収録、予約で開始/clearanceを主張しない。q588完了/ACKとexact scope/criteria確認後、同じB2 runtime/contextに投入する。q587入力release修正は既存GTK4確認内amendment01としてBが記録、Aは差分とsnapshotを照合する。次の全体未予約IDは **q594**。旧browser2出典namespaceの同番号は既往記録として別に保持。
