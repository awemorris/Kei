<!-- awesome-plan project=zedbsd record=q579 -->

# q579: origin/browser2をlibbrowser配置へ統合

Status: finished
Attempt: q579-i01 / uncleared
Phase: [ws074-p172](../ws074/phase172/phase.md)
Approval: current user / 2026-10-02「N=3でしばらく実行」
Executor: P10 generation1/2、canonical writer Q1

固定`origin/browser2` tipの差分をWS107後のbrowser/libbrowser所有へ対応させ、source、test、runner、証拠を統合した。plain/ASan build、component83、native DOM23、golden81、regression98、ABI/include/ELF、Acid2 exact、main target boot/native p014等の証拠を保存した。Acid3はscore100でもpixel37.04%で、後続p100の条件は未達。

[Final checkpoint](../ws074/phase172/import/checkpoint05/README.md)時点で対象209のmanual reviewは8完了、C/header残141、その他残60。P10 final `db6a5b336`をmain `2f37ca98e`へ統合した。whole-Phaseはunclearedで、後続browser gateは閉じたまま。owned build/test/QEMU processなし。残reviewを新しい有限Queueへ選定して再開する。
