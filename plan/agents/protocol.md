# サブエージェント別 Queue 運用

この文書は[AGENTS.md](../../AGENTS.md)のzedBSD固有規則を具体化する。固定版Awesome Planの「1 project / 1 active Queue / 1 executor」「次Queueを自動開始しない」は、ユーザー指示（2026-10-02）の範囲で置き換える。実装の権限は**各Queueの承認済みscope**に残る。

## 役割と並列数

- **main（Q1）**: `/home/awe/zedBSD-claude1` の `main` で動く単一の Claude Code セッション。共有計画・同期キャッシュの唯一の書き手、Queue ID の割当者、依存/ファイル所有の調整者、成果のレビュー・merge 担当。
- **サブエージェント**: 固定名 **P1〜P8** の8つ（2026-10-02 ユーザー）。Claude Code の Agent tool で起動し、定義は `.claude/agents/`（既定 phase-runner、試行の多い WS は phase-runner-mid）。同じ名前の担当は一度コンテキストを埋めたら、なるべく長く動かし続ける。Q1 はその担当へ次の仕事を絶え間なく依頼・予約する（Queue の終わりで担当を終了させない）。
- **N**: 同時に動かす担当の数。ユーザーが指定する（2026-10-02 N=4。ユーザー「週間利用制限はあってないようなもの」）。P1 から順に使い、N を超える名前は起動しない。利用制限を観測できなければ推測で増減しない。
- 可能なら担当ごとに WS を固定して文脈を継続する。WS 完了後は別 WS へ再配属できる。過去の P1〜P10 の割当・Queue は [history](../history/index.md) の履歴であり、同名の新しい担当の権限や状態にはならない。

## Queue と割当

- [Queue Board](../queue.md)は全担当の索引と全体Outlook。各担当の詳細Queue laneは `plan/agents/P<番号>/queue.md` に作る（未配属なら作らない）。`qNNN`は全体で一意、履歴は `plan/history/queue-qNNN.md`。1担当にactive Queueは最大1、1Queueは原則1Phase/attempt。他の担当は独立したactive Queueを持てる。mainはQueue Boardに担当・dispatch順・承認元・scope・依存・状態・merge checkpointを投影する。担当laneにはmerge request ID、提出SHA、統合SHA、ACK/未統合理由を順に残し、chatだけを台帳にしない。
- 起動指示には担当名、WS、worktree/branch、すぐ実行するQueueのID/承認scope/Phase/時限、読み込む規則、所有path、必要な前提成果と検証、予約済みの次Queue、連絡方法を入れる。Queue未承認、scope変更、未検証依存、競合source、キャンセルがあれば開始せずmainへ返す。
- 予約済みの次Queueはmainの明示的な投入であり、同じ担当が現在Queue終了後も継続できる。ただし次Queue自体の正確な実装承認を再確認する。依存Queueの成果を使う場合はmainによる統合commitと必要な検証/clearanceを確認してから開始する。独立Queueなら前のcommitの統合待ち中も継続可能。Queueが空なら勝手に次Phaseを選ばず、同じ担当のまま main の指示を待つ（待機は新Queueの実行ではない）。
- mainは依存グラフを先に見る。次にfocused goal/WS優先度、WS affinity、利用枠・回帰資源を使って割り当てる。同じPhase attemptを二重割当しない。重なるsource、QEMU/共有build、toolchain等は衝突を調整し、片方を待たせる。
- 承認はuserの具体指示または既存のexact-scope承認を引用する。mainのdispatchは承認済みQueueの担当/順番を指示するもので、新しい製品scopeや例外を自動承認しない。Phase clearanceとQueue item clearanceは成果/試験の証拠で別々に判定する。
- 既読の文脈を毎Queue全文再投入せず、差分の規則と新しいscopeだけを補足する。

## 頻繁な統合

1. 各担当は独立worktree `/home/awe/zedBSD-worktrees/p<番号>`、branch `agent/p<番号>`（固定）を使う。worktreeはmainが作り、絶対pathを起動指示で渡す。既存の古い worktree（`.claude/worktrees`、旧 `codex/` branch）を消したり再利用したりしない。担当はmain checkoutや所有外ファイルを編集しない。build directory、QEMU port/socketは担当ごとに隔離し、共有build/toolchainを書かない。
2. Phase完了を待たず、意味がまとまり、検証範囲を説明でき、コミット可能になった時点で担当pathだけを `git commit -m WIP -- <path>...` する。未完了のPhaseをclearedに見せない。mainへ担当/Queue/attempt、base SHAと先頭/末尾commit SHA、対象path、行ったcheck/未実施check、現在のPhase状態、残りと次の安全な地点を送る。
3. mainは依頼の順序/重複を照合し、sourceと証拠をレビューする。mainの清潔なcheckoutで提出時点のSHAを固定し、`git merge --no-ff --no-commit <SHA>`で統合する。競合はmainが解決して必要な有限回帰を行い、成功後に`git commit -m WIP`とする。試験失敗時は統合を確定せず原因と再開条件を残す。統合commit/検証/保留を台帳とQueue/Phaseへ記録し、担当にACKを返す。commitしただけではPhase clearanceでも他WSの依存解除でもない。
4. mainが競合解決でbranchとの差を作ったら担当へ伝え、次の依存作業前にmainの結果を取り込ませる。新たなmerge requestは前回統合済みSHAを明記し、同じcommitを二重適用しない。pushはしない。

## ラップアップと障害回復

- **通常**: mainから依頼が届いたら、新しいQueueの開始を止める。現在の小さな作業を安全なコミット可能地点まで進め、担当pathのWIP commit/必要なcheck/証拠/未達/再開点と未着手の予約Queueを返して終了する。Queue基準未達ならmainがattemptをunclearedとして保存し、Queueを証拠付きで終了する。
- **urgent**: 新しい実装や長い検証を始めず、現在の差分をすぐ保存する。未追跡の担当fileは`git add -N`で差分へ含め、`git diff --binary HEAD -- <担当path>`をworktreeの`plan/wsNNN/temp/`へ保存する。base SHA、patch SHA-256、patch path、未追跡asset、実行中process、未実施check、再開手順をmainへ送って終了する。patchは未検証の救出物であり、clearanceや統合commitと扱わない。
- 強制終了/利用制限時はmainがworktree・branch・未commit差分・最後のmerge requestを調べ、重複適用を避けて回収する。中断したattemptの結果と再開条件はmainがQueue/Phase/historyへ残す。急停止を成功扱いしない。
- コンテキスト残量/実行時間が限界ならチェックポイントを作ってmainに知らせ、残りは同じ名前の担当の新しい世代へ引き継ぐ。

## 記録の最小項目

[Registry](registry.md)は担当名/世代、WS、worktree/branch、current Queue、ordered next Queues、状態、最終checkpoint、wrap-up指示、最終merge ACKを持つ。Queue laneは承認元/範囲/Phase/attempt/依存と結果を持つ。成果のGit commit、統合、Queue/Phase clearance、WS completionは別イベント。shared cache/outboxはmainだけが書き、GitHub publicationの保留とローカル実行成果を混同しない。
