# サブエージェント別 Queue 運用（2026-10-02 ユーザー指示）

**最新（2026-10-02 / single-session-claude-subagents: current user「AGENTS.mdを読んで、オンボーディングしてください。ただし、エージェントA,Bに分けて実行するルールは採用しません。単一のエージェントセッションであるあなたが、サブエージェントをN個使って作業します。」）**: A/B 2セッション分担は採用しない（[session-a](session-a.md)・[session-b](session-b.md)・[協調記録](two-session-coordination.md)は履歴）。`/home/awe/zedBSD-claude1` の `main` の単一 Claude Code セッションが main（Q1）。子は Claude Code の Agent tool のサブエージェント（`.claude/agents/`、既定 phase-runner / 試行の多い WS は phase-runner-mid）で、本文の `gpt-6.1-sol`・`codex/` branch・「子は同時最大3」の指定を置き換える。N はユーザー指定、未指定なら実行可能な非競合 Queue 数で main が決める。worktree は `/home/awe/zedBSD-worktrees/<担当>` に main が作り、branch は `agent/<担当>-<ws>`。Queue・WIP commit・merge・wrap-up・記録の規則は以下を引き続き適用する。担当IDは P11 から（P1〜P10・A1〜A3・B1〜B3 は履歴）。

この文書は[AGENTS.md](../../AGENTS.md)のzedBSD固有規則を具体化する。固定版Awesome Planの「1 project / 1 active Queue / 1 executor」「次Queueを自動開始しない」は、このユーザー指示の範囲で置き換える。実装の権限は**各Queueの承認済みscope**に残る。今回の設計だけでOS実装Queueは開始しない。

## 役割と並列数

- メイン（Q1）は唯一の共有計画/同期キャッシュの書き手、Queue ID割当者、依存/ファイル所有の調整者、成果のレビュー・マージ担当。サブエージェントへの起動時Queue、後続Queue、通常/urgentラップアップを指示する。
- サブエージェントは全員 `gpt-6.1-sol`、reasoning effort `high`。希望並列数 `N_target=8`。実並列数は `min(N_target, 実行環境の子エージェント枠, 利用可能な枠, 承認済みで依存を満たす非競合Queue数)` とする。2026-10-02のこの実行環境はメインを含む4枠なので子は同時最大3。枠数を超えるための孫エージェントは起動しない。利用制限を観測できなければ推測で枠を増減しない。
- 過去のP1〜P7は履歴として保持。新しい実装担当は次のP8からIDを割り当て、同じ担当の再開世代を別に記録する。読取専用の短時間設計調査者は実装担当枠/Queueに数えない。可能ならWS担当を固定し、WS完了後は別WSへ再配属できる。
- 2026-10-02のユーザー指定により、前回の3人割当案のP10枠はWS074ブラウザ専任として予約する。p172取込後もAcid3のpixel完全一致、Interop 2025の対象WPTへ継続する。通常のWS切替候補にせず、ブラウザQueueが空ならmainの次の明示指示を待つ。予約は起動・実装Queueの承認を意味しない。
- 同日の追加指示で3枠を[P8=バグ修正、P9=Keilandデスクトップ高度化、P10=ブラウザ](registry.md)に固定する。P8はBug Boardの未解決ticketをmainが優先/前提を見て1件ずつhandling WSの有限Queueへ選ぶ。tracking/scheduled/実機待ちを無条件に着手可能とは扱わない。P9は[desktop outlook](desktop-outlook.md)の機能・検証を担当し、見つけたバグを再現条件/証拠とともにmainへ返す。mainが既存ticketと照合してBug Boardへ記録し、P8へ次のQueue候補として渡す。P9はバグを自分の高度化Queueへ混ぜない。P8がP9所有sourceを直す時はmainがpathの編集順とQEMU/実機を調整し、統合/検証したSHAをP9へ知らせる。P10はp172→p100→File System Access→OPFS→Interop 2025 100%→Test262を候補順とし、p101 CSS2を保持する。どの予約も実装Queue承認ではない。
- サブエージェントは担当WS/Phaseのsourceと当該WS計画、自分のworktree内buildのみ書く。担当worktreeのWS記録は統合前の草稿であり、canonicalな共有plan/cacheではない。mainは統合前に変更記録をoutboxへ準備し、統合後にQueue/WS/Phase/Project投影を更新する。共有Board、Master、Guardrail、history、同期cacheはメインが書く。bug ticketは明示された範囲のみ。HAL API・toolchain・WS095人間作業等の既存制限は保持する。

## Queue と割当

- [Queue Board](../queue.md)は全担当の索引と全体Outlook。各担当の詳細Queue laneは `plan/agents/P<番号>/queue.md` に作る（未配属なら作らない）。`qNNN`は全体で一意、履歴は従来の`plan/history/queue-qNNN.md`に置く。1担当にactive Queueは最大1、1Queueは原則1Phase/attempt。他の担当は独立したactive Queueを持てる。mainはQueue Boardに担当・dispatch順・承認元・scope hash/版・依存・状態・merge checkpointを投影する。担当laneにはmerge request ID、提出SHA、統合SHA、ACK/未統合理由を順に残し、chatだけを台帳にしない。
- 起動指示には担当ID、モデル/effort、WS、worktree/branch、すぐ実行するQueueのID/承認scope/Phase/時限、読み込む規則、所有path、必要な前提成果と検証、予約済みの次Queue、連絡方法を入れる。後続Queueはmainから順序付きで追加できる。Queue未承認、scope変更、未検証依存、競合source、キャンセルがあれば開始せずmainへ返す。
- 予定済みの次Queueはmainの明示的な投入であり、同じ担当が現在Queue終了後も継続できる。ただし次Queue自体の正確な実装承認を再確認する。依存Queueの成果を使う場合はmainによる統合commitと必要な検証/clearanceを確認してから開始する。独立Queueなら前のcommitの統合待ち中も継続可能。Queueが空なら勝手に次Phaseを選ばない。
- mainは依存グラフを先に見る。次にfocused goal/WS優先度、WS affinity、利用枠・回帰資源を使って割り当てる。同じPhase attemptを二重割当しない。重なるsource、QEMU/共有build、toolchain等は衝突を調整し、片方を待たせる。既存の優先順位はこの設計だけで変更しない。
- 承認は従来どおりuserの具体指示または既存のexact-scope承認を引用する。mainのdispatchは承認済みQueueの担当/順番を指示するもので、新しい製品scopeや例外を自動承認しない。Phase clearanceとQueue item clearanceは成果/試験の証拠で別々に判定する。

## 頻繁な統合

1. 各担当は独立worktree/`codex/` branch（例`codex/p08-ws099`）を使う。既存のprunableな`.claude/worktrees`を消したり再利用したりしない。新規worktreeはQueue割当時にmainが作り、絶対pathを起動指示で渡す。担当はmain checkoutや所有外ファイルを編集しない。build directory、QEMU port/socketは担当ごとに隔離し、共有build/toolchainを書かない。
2. Phase完了を待たず、意味がまとまり、検証範囲を説明でき、コミット可能になった時点で担当pathだけを `git commit -m WIP -- <path>...` する。未完了のPhaseをclearedに見せない。mainへmerge request ID、担当/Queue/attempt、base SHAと先頭/末尾commit SHA、branch、対象path、行ったcheck/未実施check、現在のPhase状態、残りと次の安全な地点を送る。
3. mainは依頼の順序/重複を照合し、sourceと証拠をレビューする。mainの清潔なcheckoutで提出時点のSHAを固定し、`git merge --no-ff --no-commit <SHA>`で統合する。競合はmainが解決して必要な有限回帰を行い、成功後に`git commit -m WIP`とする。試験失敗時は統合を確定せず原因と再開条件を残す。既にmainにある無関係な変更や古いworktreeをreset/pruneしない。統合commit/検証/保留を台帳とQueue/Phaseへ記録し、担当にACKを返す。commitしただけではPhase clearanceでも他WSの依存解除でもない。
4. agent branchは固定で、ACK後も同じWSを継続できる。mainが競合解決でbranchとの差を作ったら担当へ伝え、次の依存作業前にmainの結果を取り込ませる。新たなmerge requestは前回統合済みSHAを明記し、同じcommitを二重適用しない。pushはしない。

## ラップアップと障害回復

- **通常**: mainから依頼が届いたら、新しいQueueの開始を止める。現在の小さな作業を安全なコミット可能地点まで進め、担当pathのWIP commit/必要なcheck/証拠/未達/再開点と未着手の予約Queueを返して自発的に終了する。Queue基準未達ならmainがattemptをunclearedとして保存し、Queueを証拠付きで終了して、再開時は新しい承認済みattemptを作る。commit済み部分成果は統合できるが、Phase全体のclearanceとは別。
- **urgent**: 新しい実装や長い検証を始めず、現在の差分をすぐ保存する。未追跡の担当fileは`git add -N`で差分へ含め、`git diff --binary HEAD -- <担当path>`をworktreeの`plan/wsNNN/temp/`へ保存する。base SHA、patch SHA-256、patch path、未追跡/ignored assetの追加場所、実行中process、未実施check、再開手順をmainへ送って自発的に終了する。重要なignored/binary assetは別archiveにして一覧/hashを返す。patchは未検証の救出物であり、clearanceや統合commitと扱わない。mainはworktreeを保持し、patch/assetを耐久場所へ回収する。差分を自動mergeしない。
- 強制終了/利用制限時はmainがworktree・branch・未commit差分・最後のmerge requestを調べ、重複適用を避けて回収する。止める必要がある時も、可能なら担当へurgent指示を送り自発的な保存を待つ。中断したattemptの結果と再開条件はmainがQueue/Phase/historyへ残す。急停止を成功扱いしない。
- 次の指示を事前に積むことで同じ担当の文脈を使い続けられる。コンテキスト残量/実行時間が限界ならチェックポイントを作ってmainに知らせ、残りは同じ担当の再開または新世代へ引き継ぐ。runtimeの同時枠やモデルが保持される以上の常駐は保証しない。

## 記録の最小項目

[Registry](registry.md)は担当ID/世代、model/effort、WS、worktree/branch、current Queue、ordered next Queues、状態、最終checkpoint、wrap-up指示、最終merge ACKを持つ。Queue laneは承認元/範囲/Phase/attempt/依存と結果を持つ。merge requestは上のSHA/検証/残件のほか`requested → integrated / rejected / conflict`を記録する。成果のGit commit、統合、Queue/Phase clearance、WS completionは別イベント。shared cache/outboxはmainだけが書き、GitHub publicationの保留とローカル実行成果を混同しない。

現時点: q576 finished、製品実装のactive Queue なし。この設計調査のための3つの読取専用サブエージェントは実装Queueを開始していない。

2026-10-02 最新実行指示: current user「では、N=3でしばらく実行を続けてください」。当面N_target=3、有限QueueをP8/P9/P10へ投入し、mainがcheckpointを継続レビューする。上記「未起動」は設計時点の履歴。

2026-10-02 / continuous-worker-user-direction: current user「サブエージェントは終了せずに、次々とqueueを送り込んで、長時間稼働させてください」。Queueを有限で完了/終了することとagent sessionを終了することを分離する。mainは同じruntime/WS/contextにordered next Queueを積む。Queue終端ではworkerがcheckpointと結果を返し、次の承認済みQueueがあれば依存/ACKを確認して継続する。未投入なら同sessionでmainの指示待ちに入り、自発的final/終了をしない。explicit通常/urgent wrap-up、利用限界、回復不能障害では従来の保存/回収を行う。有限timeboxと製品判断/Queue権限を緩和せず、指示待ちを新Queueの実行と記録しない。既読contextを毎Queue全文再投入せず、差分規則/新規scopeだけを補足する。
