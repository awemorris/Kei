<!-- awesome-plan project=zedbsd record=ws095-p012 -->

# ws095-p012: 補いの辞書の拡張と活用の種類の注釈

Status: cleared（q593-i01、2026-10-02、P4 の実行。受け入れの証拠は下の「結果」。merge と Queue の記録は Q1）
Disposition: normal
Parent: [WS095](../ws.md)
Queue: q593 / q593-i01（承認: 2026-10-02 ユーザー「作業を開始しましょう。」、IME はユーザー「人間の作業を完了したので、あなたが担当します。」。時限 4h）
Prerequisites: p003・p004（cleared）
Investigation bound: timebox 4h

## 範囲

- `userland/desktop/ime/dict/SKK-JISYO.kei`（書き下ろし、zlib、D3）を千語へ広げ、候補に活用の種類の注釈（SKK の `;…`）を足す。`ja-dict.c`・`ja-segment.c` が注釈を読み、送り仮名の活用の候補の順を改善。
- held-out の文 100 以上を書き下ろし（`plan/ws095/tests/`）、拡張の前後で `measure.sh` の正解率を測る。

## 受け入れ

host の engine の試験が通り、held-out の正解率が拡張の前より下がらない（前後の数を記録）。既存の 100 文の正解率も下がらない。

## 所有 path

`userland/desktop/ime/`（dict・ja-dict.c・ja-segment.c）、`plan/ws095/tests/`

## 未決の判断

なし（D3 の答え 2026-09-29 夜に従う）

## Standards / evidence

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[design.md](../design.md) を変更前に読む。新しい C は全文規約。zedBSD の起動は `plan/tools/boot-test.sh` だけ、guest の操作は serial/SSH、画面は PNG で目視しユーザーに見せる。serial/console log を判定に使わない。共有 `build/` と toolchain は読取専用。全体の `make check` は禁止。QEMU の証拠と実機の証拠を分け、やっていない確認は未実施と書く。具体コマンド/結果は実行時に記録する（今は計画のみ）。

2026-10-02 / ws095-beta1-plan-20261002: 計画担当が作成。

## 結果（q593-i01、2026-10-02、P4、worktree `/home/awe/zedBSD-worktrees/p4`・branch `agent/p4`、base 901037f9f）

### 行ったこと

1. **held-out の文を先に固定**: `plan/ws095/tests/ja-heldout.tsv`（A、125 文、書き下ろし）を辞書を広げる前に書き、拡張の前の数だけを測って
   commit 1c049e599（SHA-256 `c9c2085eb09856e093a5c0ef5bd14e1e3b36b8de90a94e7349fd4985d12555df`）。`measure.sh` に `ENGINE`・`SENTENCES` を足した。
2. **注釈を読む engine**（`ja.h`・`ja-dict.c`・`ja-inflect.c`・`ja-segment.c`）: 候補の注釈がちょうど `五段`・`一段`・`形容詞` なら活用の種類として読み
   （`ja_dict_next_conjugated()`）、その種類の語尾だけで語の終わりを調べ（`ja_inflect_conjugated_ends()`）、候補は自分の活用で語が終わる時だけ並べる。
   注釈の無い候補（X の全て）は従来と同じ。X だけ・旧 337 見出しで 100 文・A の数が変わらないことを確かめた（37・92、64）。
3. **補いの辞書を広げた**: `userland/desktop/ime/dict/SKK-JISYO.kei` を 337 → **1,478 見出し**（okuri-nasi 1,054、okuri-ari 424 = 動詞 260 行・
   形容詞 82 語の i/k の 164 行）。D3 の答えの分類（代名詞・疑問・挨拶・時・数と助数詞・家族と人・体・家と物・食べ物・場所と地名・天気と自然・
   学校と仕事・な の語・移動・暮らし・かなの語と副詞・外来語の日常 約 100 と IT 約 100）で書き下ろし、他の辞書から写していない。動詞・形容詞は
   変わらない仮名を候補に含め注釈を付ける形（`たべr /食べ;一段/`、`かえr /帰;五段/返;五段/変え;一段/…/`）に旧の行も書き直した。かなを先に出す語・
   同じ読みの順は p003 の答えの考え方のまま。同じ読みの動詞の衝突（読む／呼ぶ、行く／生きる、買う／勝つ、売る／打つ、暑かった／扱った）は
   多い方だけを入れ、助詞と同じ 1 字の語幹の動詞（似る・貼る・減る）は入れなかった（どちらも 100 文で見つけ、A は見ていない）。
4. **分割の費用**（`ja-segment.c`）: 文節の数の次に「日常の語（利用者・内蔵・する・来る・かなの動詞・補いの辞書）と助詞の字数の和」を比べる段を足した
   （100 文の 3 つの分割の誤りから。A は見ていない）。ここまでを commit 060cb5f68 で固定し、A を 1 回だけ測った。
5. **A を測った後**: A の誤りに する だけの文節（しますか → 知ますか、する → 刷る、しました → 島した）と 来る の誤った形（きる → 来る、ころ → 来ろ）が
   多かった（どちらも p012 の前からの誤り）。規則を直す前に **held-out B** `ja-heldout2.tsv`（110 文）を書いて commit a4a66d020
   （SHA-256 `bc05cbe7be66b20405febbcd4fbf3600b091fd6757bc2ea0135d09ba9d79757e`）で固定し、B を測ってから、する・来る の語幹が取る語尾を実際の
   ものに絞り（`ja-inflect.c`）、する だけの文節をかなの動詞と同じ出所の順に置いた（`ja-segment.c`）。commit df0428870・0752b4384。
6. host の試験を足した（`host-engine.c`: 注釈の読み、活用の種類ごとの語尾、注釈付きの補いの辞書での変換、日常の語の段、する・来る の形。
   150 → 203。日常の語の段の試験は段の無い engine で 1 件、する・来る の試験は 060cb5f68 の engine で 9 件 FAIL することを確かめた）。
   design.md §7.1・§7.3・§7.8・§9.4 を更新。

### 計測（host、REmacs の SKK-JISYO.X SHA-256 `73819384…1ab9`、第一候補を並べた文全体の一致、`measure.sh`）

| 状態 | 100 文（既存） | held-out A（125） | held-out B（110） |
| --- | --- | --- | --- |
| 拡張の前（p003 の終わり、base 901037f9f） | 92 | **64** | 47 |
| 辞書・注釈・日常の語の段（060cb5f68、A に対して盲検） | 97 | **102** | 75 |
| ＋する・来る の規則（最終、0752b4384） | **97** | 109（規則は A を見て直した） | **81**（規則に対して盲検） |

- 受け入れ: 100 文 92 → 97（下がらない）、held-out A 64 → 102（盲検の時点）・最終 109、B 47 → 81。3 つの集合のどれでも、どの段の間でも、
  前の段で正しかった文が誤りに変わったものは無い（文ごとの比較）。拡張の前の数は base 901037f9f の source から build した engine で測り直して同じ。
- 寄与（同じ engine での比較）: 旧辞書＋新 engine は 100 文 95・A 65、新辞書＋日常の語の段なしは 100 文 94・A 100。差の大半は辞書の拡張。
- X だけ（補いの辞書なし、出荷の構成ではない）: 100 文 37 → 36、A 34 → 37、B 29 → 34。100 文の 1 文（見にいきたい → 見にい｜来たい）は 来る を
  日常の語に数えたことによる X だけの時の退行。
- 速さ: 335 文の変換で user 時間 0.08 秒（-O2、host）。

### 実行したコマンド（worktree の root、時間の上限付き）

- `timeout 600 sh plan/ws095/tests/host-engine.sh build/p4-ime/host-engine` → 200 passed, 0 failed（X なし）。
  `build/p4-ime/host-engine plan/ws095/tests/ja-test.dict build/p4-ime/SKK-JISYO.X` → **203 passed, 0 failed**（ASan・UBSan、clang -std=c11 -Werror -Wdeclaration-after-statement）。
- `ENGINE=build/p4-ime/host-engine [SENTENCES=plan/ws095/tests/ja-heldout{,2}.tsv] sh plan/ws095/tests/measure.sh build/p4-ime/SKK-JISYO.X userland/desktop/ime/dict/SKK-JISYO.kei`
  （X は main の `build/sources/remacs/dict/SKK-JISYO.X` を SHA-256 を確かめて `build/p4-ime/` に複写）。
- engine の全 source（ja-*.c・output.c・engine-direct.c）を zedBSD の target（`build/llvm/bin/clang --target=x86_64-unknown-zedbsd`、amd64 の
  user の CFLAGS `-Os -Wall -Wextra -Werror`、sysroot は main の `build/amd64/sysroot` を読むだけ）と gcc（`-std=gnu17 -Wall -Wextra -Werror`、
  Linux の build の flag）で compile → warning 0。`git diff --check` 通過。

### 未実施

- zedBSD の image の build と `plan/tools/boot-test.sh`: 既定の image は keiland-ime・ime-dict-ja を選ばない（package の既定 n）ので、既定の image の
  内容はこの Phase で変わらない。IME の入った image（`plan/ws095/tests/config-amd64-ime.mk`）の build と guest での変換の確認はしていない
  （辞書の file の path と形式は不変、engine は host で確かめた）。
- keiland-ime の program 全体の link（main.c 等は不変）、Linux・FreeBSD の keiland の build。実機の確認。

### 制限・残り

- **語の数**: 1,478 見出し（動詞の 1 行に一段と五段が並ぶ行、形容詞の i/k の 2 行を数えると語は約 1,400）で、ユーザーの「千語まで」を 4 割ほど
  超えた。削るかどうかはユーザーの判断（削る時は分類の単位で、100 文と新しい held-out で測り直す）。
- held-out の文と辞書は同じ書き手（この担当）が書いた。A は拡張の前に固定し、語の選択に A・B の誤りを使っていないが、書き手の語彙が重なる偏りは除けない。
  する・来る の規則は A の誤りを見て直したので、その分の A の数（102 → 109）は盲検でない。B はその規則に対して盲検だが、A を見た後に書いた。
- 残る誤りの種類は design.md §9.4（読みで決まらない語、助詞の字数の段が日常の語の字を助詞に取る分割、短い語・かなの語の不足、可能形＋そう、数）。
  B を見た後なので、直すなら新しい held-out を書いてから別の Phase。

### 判定

受け入れ（host の engine の試験が通る、held-out の正解率が拡張の前より下がらず前後の数を記録、既存の 100 文も下がらない）を満たすので cleared。
