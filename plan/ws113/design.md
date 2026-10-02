# WS113: 能力照合・契約・後続検証

Status: incomplete / p001 in-progress / q586-i01。
調査: 2026-10-02、source baseline `0e68854ac`。文書だけを変更し、production source、driver/HAL API、hardware/SSH/host、toolchainを変更していない。

## p001成果と未決

- [能力表](phase001/source-audit.md): 20行。i915 HPD eventsはsequence1固定、選択outputはID1/generation1の0/1台、claim/presentは単一。通知UAPIのper-open QUERY/ACKは既存。拡張enum値だけはheader内に存在するが、EXT control/counterのstruct/prototype/entry/runtime広告は非検出。
- [契約](phase001/contracts.md): native→Vulkan→compositor→public libkeiland→Settingsの所有、ID/世代、per-fence cursor、idle/0台watch、0/1/2台状態、rollback、異解像度mirror、pointer境界でroot tree単一owner、protocol snapshot/transaction、保存。
- [IDとpresent完了比較](phase001/identity-completion.md): session keyと保存mappingを分離。GPU deviceUUID+displayNameのport key案/全key案/typed私有API案を比較。私有拡張は比較のみでmain不採用。standard present_waitの曖昧な切断成功・timingと実scanout消去の差を明記。
- [native capability](phase001/native-contract.md): EXT control全entryへ必要なsemantic operation/power/timing/counter/寿命/失敗domain。既存UAPIに無い能力を差分review入力とし、HAL/API変更承認とは分離。
- [fixture](phase001/fixtures.md): 歴史上の5330/eDP+HDMI成功をcontextに、H01–H10/D01–D10の有限検証案。現在の実fixtureは未確認/未利用。virtual capture/host contractを実i915合格へ代用しない。

D-IDはmain技術採択A2（local PCI segment:BDF+connector kind+物理DDI key→native name/standard displayName）、私有Vulkan拡張はmain不採用。GPU標準UUIDは別能力として残し必須追加しない。D-ATOMIC（logical ownerとstrict physical移動の解釈/不表示期間）はuser回答待ち。旧hdmi/edpは初期preferred anchorを保ち全connected inventoryを隠すdisableに転用しないmain採択。D-BOOT/LAYOUT/REC/AUTH/PORTの通常案は2026-10-02 main技術採択（初回全extended/internal anchor、edge snap/辺連結/非重複、退避窓奪回無し、active同UID変更、eDP+HDMI初回fixture）。材料未解決のままp001をclearしない。

## 後続へ渡す実出力

| Phase | 対応契約 / 実出力 / gate |
| --- | --- |
| p002 | connector slot/ID/generationとcoherent inventory、HPD worker→sequence publish/wake、複数pipe/plane/lease/scanout寿命。[EXT power/first-pixel等のnative能力結線案](phase001/native-contract.md)を差分設計。A2 keyは既存PCI APIで生成し、UUID query追加は必須にしない。追加APIを採択済としない |
| p003 | KHR display/plane/mode/surface、EXT control全4entry+surface counter依存、device event fence独立consumer/寿命/timeout、header ABI/procaddr enable広告。D-ATOMIC採択時だけpresent completion追加を規約限界付きで設計 |
| p004 | output table、0台live server/watch、独立swapchain、global/local座標、全extended/allmirror、GPU aspect-fit+opaque bars、transaction rollback/degradedとdynamic wl_output |
| p005 | 専用Wayland snapshot/batched done、expected topology/config serial、token/gen全member、apply/result、public wrapper ownership、active-session credential、保存と適用結果分離 |
| p006 | libkeiland-only Settings、mode二択、extended配置draft/edge snap/Apply/cancel、hotplug/stale/unsupported/persistence failureの表示 |
| p007 | root+popup/subsurface/transient/decorationsの単一owner、global pointer shared edgeとgrab offset、generation/epoch、退避/park、採択D-ATOMICのsource/target順 |
| p008 | actual zedBSD i915両outputの通知→設定→表示→窓移動→復帰。manifestとfinite H/D matrix。physical同時表示/消去証拠をlogical captureと区別 |
| p009 | 最終WS source全文のfull-standard review、該当narrow format/style/analysis/build/ABI/regression、D1–D5をPhase countから独立判定 |

各Phaseへの改訂とeventは保存済みrecordを参照。p002–p009はplanned/Queue noneであり、今回p001文書Queueから実装を開始しない。依存はp001→p002→p003→p004→{p005→p006,p007}→p008→p009。実装前に必要な実出力とauthoritative標準を確認する。

## 主な成立条件

- native ACKはopen description単位、Vulkan event fenceは独立cursor。lease fdのdupでdevice watcher独立性を作らない。native初回sequence1と実plugを区別。
- stable ID/generation/sequence/VkHandle/session token/保存keyを別に扱う。標準displayNameはunique/persistentでなくinstance中不変。実装policyを規範の保証へ置換しない。
- standard QueuePresent/QueueWaitIdle/render fence/FIRST_PIXEL_OUTだけで旧headの窓消去を証明しない。present_waitも一般仕様だけでは全旧pixels消去を保証しない。強い実装保証とactual i915証拠が必要。
- 両modeは全接続集合が対象。unsupported資源/connectorを黙って成功扱いにしない。mixed mode、別GPU、今回のLinux/FreeBSD複数画面受け入れは追加しない。
- Settings→public libkeiland→専用compositor拡張。compositor GPU UAPI直接ioctl禁止。GPU操作はVulkan経由。

## 標準 / context / sync

[全文方針](../standards/ws113-display.md)、[Guardrail](../guardrail.md)、[C全文](../coding-style.md)、[automation](../standards/automation.md)。[WS075](../ws075/ws.md)/[WS103](../ws103/ws.md)/[WS089](../ws089/ws.md)はread-only context。既往Phaseのclearance、他WS/standards/全体recordsは変更しない。
Khronos一次仕様のURL・確認日・revision・latest可変性は能力表/契約/比較に記録。GitHub公開とMaster/Queue/Agent lane/standardsの投影はmain所有のpending reconciliation。
