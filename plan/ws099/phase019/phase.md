<!-- awesome-plan project=zedbsd record=ws099-p019 -->

# ws099-p019: 白樺・湖の背景を共通ソースと3 OSの成果物に収録する

Parent: [WS099](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: q593予約（Agent A、未実行。最新user wrap-up指示により後続投入を停止し、再開待ち）
Owner: B2 / same GPT-6.1 Sol High context
Purpose / goal: テストだけのgit外資産を回収し、zedBSD/Linux/FreeBSD共通で利用できる背景としてソースとrelease dataに含める。ぼやけた湖を初回起動のdefaultにし、収録済み背景の選択と保存済み設定を維持する。
Investigation bound: 実装Queue投入時にexact source/criteria・最大3時間を確定。抽象版探索は有限で、見つからなければ探索場所/限界を記録する。

## Authority / prerequisites

2026-10-02 current user:「B2にお伝えください。Keilandデスクトップ開発の初期からテストで使っている、白樺と湖をぼかしたデスクトップ背景が、リリース成果物に含まれていないようです。zedBSD, Linux, FreeBSDに共通で利用できる背景として、ソースツリーに格納してください。また、この画像を直線的に抽象的にした背景画像もあったはずですが、それもみつかったらソースツリーにいれておいてください。」

2026-10-02 current user追加決定:「B2にお伝えください。旧画像2枚のうち、湖の画像（ぼやけたもの）をデフォルトの背景で起動するようにしてください。すでに収録されている画像はデスクトップ背景切り替えで利用できるように維持します。」3 OS共通の未設定時の起動defaultを湖の画像にする。保存済みの背景設定は既存の優先関係を維持し、収録済みのAurora/Dawn/Lagoon/Meadow/Twilight等を背景切り替えから引き続き選べることを検証する。B2は同じセッションで受領済み。

[WS035 p061](../../ws035/phase061/phase.md)の当時git外保存方針を、この既存画像のsource/release収録について置換する。WS035当時のclearance/画像利用の履歴は保持し、受け入れを無効化しない。WS035の後継WS099へ追加のasset goalとして分離する。B2 q588のsource review結果を安全に保存後に実行し、q588へasset差分を混ぜない。

## Current evidence / pending design

mainが旧v2-soft-b.pngを目視確認。既存wallpaper.ppm（1280x800、SHA2563616eb2147caa717dbd53b5e151283f8942fbd2b1b2aa5cf54f47cad6639b687）とv2-soft-b.png（SHA2566ed9573292dc0004401d434e18d32eede24d9acda72e079566033a245ea6f4e6）のRGB pixels一致をB2が確認。署名LEEKING26と2026-09-26 user提供/ぼかし指示を旧recordから回復。既存bytes/provenanceを保持し、未根拠の新ライセンスを付与しない。

直線的な抽象版は未発見。既存assetを探し、画像生成/編集はscopeに含めない。sourceの共通wallpapers領域とzedBSD/nativeLinux/nativeFreeBSDのasset生成/install/package membershipを照合し、候補path/recipe/重複/依存をexact Queueへ確定する。

## Intended criteria / standards / resume

既存背景の同一bytesを共通sourceへ格納、provenance/hash/dimensionsを保存し、3 OSのstaged data/install/package経路で収録を確認。未設定時の起動defaultがぼやけた湖の同一画像になること、既存背景の選択肢が維持されること、保存済み設定が優先されることを確認する。見つかった抽象版も同様、見つからない場合は条件付き指示の探索結果/再開triggerを記録。asset-only targetsの隔離実行とmanifest/hash/readbackを優先し、toolchain・共有build・外部systemへのdeployは行わない。必要なsource/recipe検査はexact Queueで確定、全体make checkは禁止。

[Guardrail](../../guardrail.md)、[automation](../../standards/automation.md)、各native packaging資料とsource配備規則を適用。Cを編集する場合は全文Cが必要だが、今回の意図は既存画像/asset recipes/data metadata。WS099全体conformanceは最後に保持し、asset収録だけでWSをcompleteにしない。

2026-10-02 / user-common-wallpapers-20261002: userの既存背景source/3 OS収録指示から新Phaseを計画。旧p061とWS099 summaryに関連eventを保存、Agent Aが共有Queue/registryと必要なpackaging projectionを所有。未実装。

2026-10-02 / user-lake-default-20261002: 実装前の追加指示により起動defaultと既存背景選択維持を受け入れ条件へ追加。q593のexact snapshotへ含める。q588のsource conformance scopeは変更しない。WS099 summaryへ投影し、共有記録の投影はAgent Aへのhandoff対象。

2026-10-02 / user-b1-b2-wrap-up-20261002: userがB1/B2へ現在のPhase後のラップアップ/終了を指示。B2の現在の実行はq588であり、q593/p019はまだ開始していないため後続投入を停止する。背景収録/defaultの決定と調査成果を保持し、Phaseはplanning/normalのまま。取り消しや実装完了を意味しない。

2026-10-02 / b2-wallpaper-resume-saved: B2最終提出111b864aをB main9323725bへ統合。[再開資料](../../ws094/phase007/q593-resume.md)に既存PNG/PPMの同一bytes/hash/provenance、native/zedBSDの最小recipe候補、保存設定優先/既存選択肢維持、抽象版の未発見と探索限界を保存。source画像収録・default変更は未実施。q593は予約のみで未着手を維持する。
