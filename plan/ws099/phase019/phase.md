<!-- awesome-plan project=zedbsd record=ws099-p019 -->

# ws099-p019: 白樺・湖の背景を共通ソースと3 OSの成果物に収録する

Parent: [WS099](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: Agent Aの後続ID予約待ち
Owner: B2 / same GPT-6.1 Sol High context
Purpose / goal: テストだけのgit外資産を回収し、zedBSD/Linux/FreeBSD共通で利用できる背景としてソースとrelease dataに含める。
Investigation bound: 実装Queue投入時にexact source/criteria・最大3時間を確定。抽象版探索は有限で、見つからなければ探索場所/限界を記録する。

## Authority / prerequisites

2026-10-02 current user:「B2にお伝えください。Keilandデスクトップ開発の初期からテストで使っている、白樺と湖をぼかしたデスクトップ背景が、リリース成果物に含まれていないようです。zedBSD, Linux, FreeBSDに共通で利用できる背景として、ソースツリーに格納してください。また、この画像を直線的に抽象的にした背景画像もあったはずですが、それもみつかったらソースツリーにいれておいてください。」

[WS035 p061](../../ws035/phase061/phase.md)の当時git外保存方針を、この既存画像のsource/release収録について置換する。WS035当時のclearance/画像利用の履歴は保持し、受け入れを無効化しない。WS035の後継WS099へ追加のasset goalとして分離する。B2 q588のsource review結果を安全に保存後に実行し、q588へasset差分を混ぜない。

## Current evidence / pending design

mainが旧v2-soft-b.pngを目視確認。既存wallpaper.ppm（1280x800、SHA2563616eb2147caa717dbd53b5e151283f8942fbd2b1b2aa5cf54f47cad6639b687）とv2-soft-b.png（SHA2566ed9573292dc0004401d434e18d32eede24d9acda72e079566033a245ea6f4e6）のRGB pixels一致をB2が確認。署名LEEKING26と2026-09-26 user提供/ぼかし指示を旧recordから回復。既存bytes/provenanceを保持し、未根拠の新ライセンスを付与しない。

直線的な抽象版は未発見。既存assetを探し、画像生成/編集はscopeに含めない。sourceの共通wallpapers領域とzedBSD/nativeLinux/nativeFreeBSDのasset生成/install/package membershipを照合し、候補path/recipe/重複/依存をexact Queueへ確定する。

## Intended criteria / standards / resume

既存背景の同一bytesを共通sourceへ格納、provenance/hash/dimensionsを保存し、3 OSのstaged data/install/package経路で収録を確認。見つかった抽象版も同様、見つからない場合は条件付き指示の探索結果/再開triggerを記録。asset-only targetsの隔離実行とmanifest/hash/readbackを優先し、toolchain・共有build・外部systemへのdeployは行わない。必要なsource/recipe検査はexact Queueで確定、全体make checkは禁止。

[Guardrail](../../guardrail.md)、[automation](../../standards/automation.md)、各native packaging資料とsource配備規則を適用。Cを編集する場合は全文Cが必要だが、今回の意図は既存画像/asset recipes/data metadata。WS099全体conformanceは最後に保持し、asset収録だけでWSをcompleteにしない。

2026-10-02 / user-common-wallpapers-20261002: userの既存背景source/3 OS収録指示から新Phaseを計画。旧p061とWS099 summaryに関連eventを保存、Agent Aが共有Queue/registryと必要なpackaging projectionを所有。未実装。
