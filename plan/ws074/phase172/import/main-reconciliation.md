# q579 branch計画/bugの意味的な照合（main所有）

origin/browser2 e53ef03b80113aec959deb67f828cba21d68d4beの原本356件は同directoryのbrowser2-evidenceへ原byte/hash/出典を保持した。`.source.txt`は過去証拠でありcanonical Queue/Phase/bug状態を所有しない。

- branch p100 reportedclearedはAcid3score100/100だがpixel37.04%を診断に留めた当時条件。現行p100のpixel完全一致・fail0は未達、現行plannedを保持。
- p102〜p171はbranchの実装/試験履歴の原IDを維持。import sourceを現行plain/ASanで再試験し対応能力を照合する。過去attempt/closeを現行Queueへ再生せず、semantic-indexはhistorical reportedstatusとcurrentverifiedoutcomeを分ける。今後のbrowser作業はp172 wholePhase gateの後だけ。
- branch q508〜q591にmain既存qNNNとの衝突がある。出典をbrowser2:qNNNと識別して原本へリンクし、現行historyを上書きしない。
- branch BUG126/127/128/130はVM realm/GC/selector/mutation recordの過去resolved原本。main同番号の異なるbugを上書きせずbrowser2:BUGNNNで保存、対応import回帰の検証記録をworkerが完成させる。
- branch BUG131/132はviewport/dataURLの過去resolved原本としてbrowser2 namespaceで保持（IDを勝手に変更しない）。後続で同原IDを割り当てない。
- branch BUG129 unknown/trackingのUTF16byteoverflowはmainBUG129 Fontsと衝突。[BUG133](../../../bugs/BUG-133.md)に新canonical tracking destinationを作り、原ID/原本をreciprocal保存。原sourceはimport差分外、修理Queue未選定。import成功を修理/未再現証明に使わない。

code source93/test116はworkerがactual mapping・currentbuild/API/回帰/全文manualを検証する。archive保存だけではwholePhaseclearしない。GitHub publication保留。
