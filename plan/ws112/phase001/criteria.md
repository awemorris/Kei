# q585-i01 / p001 criteriaと実証範囲

Status: 2026-10-02 08:11 UTC、q585-i01/p001 uncleared。D1返答待ち。Phase正本は[phase.md](phase.md)。
Scope: 07:12〜08:12 UTCの契約調査。実sourceと一次資料から設計/後続commandを具体化し、実環境取得/buildをこのattemptで行わない。

| p001基準 | evidence / 2026-10-02判定 |
| --- | --- |
| 5 target / CPU / OS版・公式input pins | [survey](survey.md)に全5URL/hash/版/CPU、Debian/Ubuntu既存pin維持、RPi真正rootfs、Fedora Generic、Arch version directory/repo日付。metadata照合PASS、image本体未取得 |
| RPi build-only/native環境を選択 | [native-environments](../native-environments.md)の外側既存Debian13 QEMU＋内RPi arm64 rootfs/QEMU-user/native compiler。mainのdelegated判断でD2選択済み、compiler/ABI/provenance/isolation検証command定義。実成立はp003 |
| 共通manifest/payload/launcher/source | [survey](survey.md#共通production-payload)と[contract](../package-contract.md)に全path/mode/owner/種類/除外/config/session/license、native source allowlist/outer gzip修正と全OS source hash契約。実source照合PASS |
| 個別dependency / CPU / version / format | contractのdpkg-deb/rpmbuild/makepkg、native DB/NEEDED＋absolute dlopen/private Vulkan、config保持、source識別version、native query/独立展開を定義。公式package index/.info確認、実ELF/native guest版照合は後続package Phase |
| build環境・成果物/CI/release契約とcommand | native-environments/contractにguest手順、5OS20file集合、sidecar schema/source/checksum/OS/CPU照合、CI全件needs/既存imagezip保持と有限否定試験を定義。CI runtime/FreeBSD packageを追加しない |
| QEMU boot方針・調査上限/時間見積 | Debian/Ubuntu既存SSH/QMP方式、RPi外側Debian方式。Fedora/Archの同方式適用はD1 user返答待ち。既存WS108のTCG成功/8GiB/CI45minとelapsed未保存、新方式の時間未実測・後続工程別elapsed/finite90min条件を記録 |
| 後続に未解決人間判断を持ち込まない | **D1未決のため未達**。mainがuser返答/判断元と共有Guardrailへの反映を保存するまでp001をclearしない。D2/D3/D4は上記の技術選択/後続検証契約として具体化 |

## Commands / results / limits

読取: `git status --short`（開始/小checkpointでclean）、`git branch --show-current`（codex/a2-packages）、root Makefile/release Python/native mk/CIとcompleted WS105/108/111、Guardrail/package full/automation/approved Phase全文。
web: official download/package/QEMU/kernel/PRoot一次URLを閲覧、観測日は各contract。取得不可/403/404とversion URLの差をsurveyへ保存。
bounded metadata: Python3 urllib、timeout10〜20s、body上限200000/500000 bytes、最大並列6（最後の追加は3）、image body GET無し。RPi.info/catalog、公式SUMS/SUMS.sig/public keys等をignored tempへ保存。
signature: isolated GnuPG2.4.7 keyring、Ubuntu/Fedora/Archの公式fingerprintと`VALIDSIG`一致/exit0、版付きArch SUMSも実検証。RPi publickey fingerprint/packet issuer一致だけ、full image署名未検証。Debian現行unsigned cloud入力を署名済みとしない。
tools: Python3.13.5/git2.47.3/curl8.14.1/GnuPG2.4.7/QEMU10.0.11を読取。guest/compiler版は未観測、公開index/.info版と区別。
manual: 非C近傍Markdown形式、scope/受け入れ/依存順/IDsと変更Phaseごとのorigin/foreign/WS events、公式URLの支持範囲、取得metadata・署名と未取得imageの区別をreview。
local links: 全WS112 tracked Markdownの相対path存在をPythonで確認（最終148件/0error）。remote URLの将来保持/リンクfragmentの全自動検証は保証しない。
`git diff --check`: 各小checkpoint PASS。git commitsはWIP、全差分plan/ws112内のみ、mainへA2-001〜004 MRとSHA/path/checks/remainingを提出。

Skipped: OS image本体取得/rootfs展開、guest boot、production/CI source編集、host install/build、native package生成/導入・runtime、GPU/GUI/実機、push/remote Actions/release/Issues。
未実行が設計調査scopeと一致し、実装/後続Phaseの合格を主張しない。shared Master/Queue/Guardrail/standards/history/registryは読取、canonical projections/outboxはmain所有。
Resume: D1のexact判断元/適用方法をmainが保存後にp001基準を再評価。終端時未解消ならq585-i01とp001はuncleared、次Queueはmain承認を待つ。[q591候補](../phase002/queue-candidate.md)は準備のみ。
