# WS106 q540 partial 検証checkpoint

29 package＋直下13files、ime-probeはpending。build outputはbuild/ws106-p002/、source inventory/checkpointはWS106内。

| Command | Result |
| --- | --- |
| make list-user-programs / normalized old rows | 全ID/default/label/platform/dependency維持、移動29件のmenu/pathだけ変更 |
| make -j16 keiland-linux CC=gcc/clang KEILAND_LINUX_BUILD=build/ws106-linux-{compiler} | clean outputから各exit0、warning0 |
| make keiland-linux-install ... DESTDIR=build/ws106-{compiler}-stage | 各exit0 |
| elf-check.sh STAGE | ELF24各PASS |
| CC=gcc/clang KEILAND_LINUX_BUILD=... header-check.sh | source331各PASS |
| makefile-sync.sh | PASS、tests/も既存globが対象 |
| make -j16 ZEDBSD_USER_PROGRAMS=<既存+test IDs> <28 test binaries + POSIX-R1/R2/R2-REMAINING/SUSV4-XSI/SMP-STRESS/helper/dynamic-userland-check> | exit0、warning0、target専用compiler |
| make -j16 disk-image | exit0、project/external warning0 |
| moved file/mode/C preservation + normalized style | 164files/58C、style1272で既存との差無し |
| quoted header/source/data registration + installed model/texture | PASS、gpu-shareの既存private -Iは維持、data18entries |
| edited scripts syntax / git diff --check | PASS |

初回test target commandは未選択のgpu-admission-testの規則が無くexit2。既存configの選択IDに28appをcommand lineで足して再実行exit0。
構成ファイル自体は変更していない。初回include inspectionはgpu-shareのprivate -Iを考慮せず指摘、実build/宣言されたinclude先で確認して解決。
過去のログ/patch/ledgerへの自動locator変更はreviewで除外し、元hashと完全一致するよう復元。実際の差分は現役runner/source/buildのみ。
既存styleはユーザーが認めたWS106限定例外。new implementation無し、GPU/実機/機能回帰はpure relocationなので未実施。
最終boot/full acceptanceはp003で記録。ime-probe所有未確認のままwhole WSをcompletedにしない。

## q540 の終了checkpoint

29 package＋追加13filesの承認partial scopeはcleared。164filesのhash/mode/参照・registry保存、Linux GCC/Clang clean build/install warning0（ELF24/source331各）、zedBSD28app/POSIX/loader/image warning0、boot-test.sh login PNG確認。ime-probe2filesは非競合回答待ちで未変更。whole p002はuncleared、p003は未実行、WS106はincomplete。

既存menuconfig host fixtureは移動前と同じFonts分類欠落でFAIL、[BUG-129](../bugs/BUG-129.md)へtracking、今回未修正。
[ビルド記録とPNG](../history/ws106/q540/evidence/SHA256SUMS)をSHA256照合して保存。bootは移動済み範囲の中間確認であり、残りime-probe移動後の最終p003 bootは未実施。
source commits: 771469b0 / 313ec26f、shader-generatorの実path補完をこのcheckpointとcommit。並行するc38be46b README変更は人間の変更として保持、WS106のsource差分に含めない。
