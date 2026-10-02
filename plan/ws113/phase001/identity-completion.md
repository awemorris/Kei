# WS113 p001: ID transportとpresent完了の比較

Date/validity: 2026-10-02、q586-i01、source baseline0e68854ac。本文の一次URLを同日に取得。latestページは可変なのでp003開始時に宣言pin/VUID/dependencyを再確認する。調査結果であり追加API実装の許可ではない。
Parent: [契約](contracts.md)、[能力](source-audit.md)。

## 1. session内識別と保存mapping

session識別は既存native(device_id,display_id)→instance-lifetime VkDisplayKHR→compositor output_tokenで足りる。再接続時の同port mapping、generation失効、mode handle更新を検証する。保存には別のconnector keyを要する。**2026-10-02 main技術採択は下記A2（local PCI segment:BDF+connector kind+物理DDI port）で、GPU標準UUIDを必須にしない。**

[VkPhysicalDeviceIDProperties](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceIDProperties.html)のdeviceUUIDはinstance/process/API/driver version/rebootを跨いで同deviceに不変である規約を持つ。ただし電源off中のhardware構成変更で変わり得て、serialized device識別への依存を避ける注意がある。driverUUIDはbuild識別、pipelineCacheUUIDはcache互換性であり、保存output keyにはしない。deviceLUIDはこの実装ではinvalid。いずれもconnectorやEDID identityを供給しない。

### 1.1 actual sourceでのUUID取得能力

- public `vulkan_external.h`にproperties2KHR/IDProperties ABI、libvulkan `external-properties.c:39`にID pNext処理がある。
- `vulkan_physical_identity():320`はnative opcode148にIDPropertiesを連結してserialize/decodeする。driverUUIDだけguest namespaceをxorし、deviceUUIDは返値のまま。query失敗はwrapperで全0にする。
- baseline i915 `render/instance.c:83`のopcode switchは0–8/11/12/19/20/155のみ。148は無く、dispatch builtin未port拒否はENOTSUP/reader poison。独立実機queryは未実行。
- 従って「標準GPU IDが既にあるからpersistは完了」とは言えない。GPU UUIDを採用する場合はp002/p003でnativequeryの実能力と同一GPUを2process/rebootで識別する証拠を要求する。query失敗0UUIDをvalid keyにしない。platform identityをcompositorが直接GPU ioctlで取得する案は境界違反。

### 1.2 比較

| 案 | compositorが使う値 | 長所 / 残条件 |
| --- | --- | --- |
| A: standard GPU UUID + implementation port key | properties2KHR IDのdeviceUUIDと、displayNameの認識済み`zedbsd-port-v1:…`等の短いconnector key | 私有Vulkan API追加無し。native name[64]にport部分だけなら収めやすい。GPU query未実装を補う必要。標準displayNameの一意/永続規約を新たに主張せず、このimplementation内のpolicyとして検証 |
| A2: full platform/port key in displayName（main採択） | version付きlocal PCI segment:BDF+connector kind+物理DDI key。session VkPhysicalDeviceとの対応も保持 | UUID runtime追加を必須にしない。同一machine/同一PCI portに保存範囲を限定し、再起動時もmode/capability再validate。hardware/PCI配置変更・別machine移植後の恒久identityは保証しない。64byte内を検査 |
| B: typed private Vulkan identity extension（比較のみ、main不採用） | 明示key/label/persistence scope/capability | EDID表示名とconnector keyを分離できる。新ABIとarchitecture所有/他OS扱いを要するため未採択。standard extensionを偽装しない |
| C: session only | VkDisplayKHR/output_tokenで実行中だけmapping、保存は保留 | 実行中hotplugには対応可能。restart/reconnect設定保存の要件を満たさないのでscope判断なしに最終採用不可 |

A/A2では同portへ別monitorを差してもport keyは同じ。これはconnector-based layout保存の意味でありmonitor追従配置ではない。同modelで同EDID名/同serial欠落の2台を別portとして区別する。monitorを別portへ移すとそのportのlayoutを使う。EDID identityを無検証でkeyに混ぜない。

`displayName`はinstance寿命中不変で、通常human-readable EDID名である。[VkDisplayPropertiesKHR](https://docs.vulkan.org/refpages/latest/refpages/source/VkDisplayPropertiesKHR.html)のcompatibilityに注意する。key+可変monitor名を同stringへ詰めてhotplugで更新する案は採らない。UI labelは短いconnector名を基本にし、EDID詳細表示が必要なら別に取得可能な標準/採択APIがあることを先に確認する。keyはprefix/version/文字集合/長さをvalidate、一意性collision時は保存・自動復元を拒否して現配置を維持する。

GPU交換/移動/構成変更でUUID/keyが変化した場合はunknown outputとして扱い、旧entryを保持して新fallbackへ置く。値の一致だけで異なるdriver/未検証schemeへ古い設定を投影しない。OS別backendのlibvulkanがschemeを提供しない場合、Settings snapshotはpersistable=falseとし、今回対象外のLinux/FreeBSD単一表示を壊さない。

## 2. standard present completionと厳格移動の限界

### 2.1 一次規約

| 一次source（確認2026-10-02） | 保証 / 制約 |
| --- | --- |
| [VK_KHR_present_wait](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_present_wait.html) rev1 | swapchain/present_id依存。presentがuserへ見えることを待つAPIだが、正確な表示時刻関係は要求しない。out-of-orderの更新にも注意 |
| [VK_KHR_present_id](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_present_id.html) rev1 | swapchainと(properties2KHRまたはcore1.1)依存。現在properties2KHRは存在するのでcoreversionを上げる必要は無い。feature query/enable処理は追加必要 |
| [VkPresentIdKHR](https://docs.vulkan.org/refpages/latest/refpages/source/VkPresentIdKHR.html) | presentInfoのpNext、swapchain毎に非0IDが前の非0より大きい。0はID無し。更新時刻の精密保証無し |
| [vkWaitForPresentKHR](https://docs.vulkan.org/refpages/latest/refpages/source/vkWaitForPresentKHR.html) | enabled feature、非retired swapchain、ID以上かtimeoutを待つ。OUT_OF_DATE時は「表示された可能性」でSUCCESSも許す。MAILBOX置換/同時thread/atomic内部状態の規約を守る |
| [VkDisplayEventTypeEXT](https://docs.vulkan.org/refpages/latest/refpages/source/VkDisplayEventTypeEXT.html) | FIRST_PIXEL_OUTは次refreshのfirst pixelがdisplay engineから出る時点。特定presentIDの表示完了や全旧pixels消去には結び付いていない |

display engineのlatch/start scanout、全行の新frame走査完了、LCD側の表示/残像は別である。**標準present_waitがあるだけで旧window全pixels消去や2head無重複を保証しない**。同headのmode/image原子操作も2headの同時latchではない。

### 2.2 sourceの不足

現在`vkQueueWaitIdle()`はWSI workerのnative requestとrenderproducerをdrainするがfront scanoutを除外（wsi-swapchain.c2792）。`drv_i915_present_display_present()`はFIFOflipをarmして戻る。次PRESENTまたはGPU_DISPLAY_WAITでlatchを確認する。i915 WAITはtimeout_nsを読まず最大100ms flip-event waitへ進み、NO_VSYNC/capture buildはphysical waitをしない。flip_stuck状態でも既存flip_waitの早期returnに注意する必要がある。既存WAITの存在だけからtimeout=0やstrict完了能力を推定しない。

したがってcandidate標準実装は、source/native completion契約を整えてp003でpresent_waitへ結線する必要がある。追加common HAL APIをここで採択しない。driver-owned GPU UAPIの変更が必要ならp002設計と所有者へ差分を提示する。

### 2.3 実装候補（D-ATOMIC採択後のみ）

1. present job受理時にcaller-owned ID配列をcopyし、swapchain毎のID単調性/featureをvalidateする。workerでIDと実native sequence/generationを対応づける。enqueueだけでcompleted IDを進めない。
2. direct-display FIFOでnative sequenceの**実latch**を観測後にcompleted IDをatomic publishする。finite timeout/0timeoutはnonblocking observationと専用completion wakeで実装し、mutexを持ってnativeblockしない。superseded/out-of-orderの場合はsource eraseを追い越すwindow有りframeが後から出ないことを独立に保証する。
3. compositorはtransfer epoch開始時に旧outputへのwindow有りframe生成を止める。既存accepted仕事をorderedに退役→窓無しsource sceneへIDを付けてpresent→確認→target sceneをsubmit。後続source sceneもwindow無しなのでID以上の完了が返っても内容保証を保つ。
4. `VK_ERROR_OUT_OF_DATE_KHR`/surface-lost/退役raceのとき、標準の曖昧なSUCCESSだけでsource eraseを証明しない。接続/generationを再照合し、物理出力が停止したことをdriver契約で確認できるか、target出現を保留してdegraded/parkへ移る。target失敗でsourceへ戻すときも対称のcompletion制約を守る。
5. latch後に旧frameの下部がまだ走査中かもしれない厳格解釈には、driverのpipe frame/scanline境界で**全sourceframe走査完了**を待つ強い実装保証を要する。100ms固定sleep/refresh推定は保証にしない。能力未確認でfeatureを広告しない。LCDのoptical残像までの無重複はVulkan/scanout contractでは約束できない。

通常候補は「描画ownerをpointer境界で原子的に変更、各frameは1owner、old/new scanout遷移を規定」である。厳格候補は「sourceの窓無しframeが必要な境界まで完了してからtargetに出す」ので短い不表示時間が生じる。物理全head同時切替/不表示無しを必須とするなら追加下層能力が必要で、現在のsourceと標準だけでは成立しない。**この要求解釈と不表示期間の許容はmainがreview可能な製品判断として残す。**

## 3. main技術裁量へ渡す通常提案

全てをuser必須選択にしない。以下は既存二択/単一window条件を満たす通常提案で、mainが技術設計として採用・変更して記録できる。ユーザーの明示意図と衝突する場合だけ製品判断として返す。

| 論点 | 通常提案 / main裁量の範囲 | userへ返す条件 |
| --- | --- | --- |
| D-BOOT | 保存mode/layout優先、初回全extended、既存internalをanchor。旧`auto`は全connectedを意味するdesktop policyへ一般化 | 明示`hdmi/edp`が他outputを隠す旧overrideとのcompatibilityを維持/廃止する範囲変更 |
| D-LAYOUT | edge snap、非重複、辺で連結、signedorigin。配置draft中のみdrag、Applyで実状態更新 | 任意gap/重なりlayoutが明示要求された場合 |
| D-REC | output配置だけ保存復元、退避windowは現ownerのまま。title/pointerのアクセスを維持 | reconnectでwindowも自動戻しをユーザーが要求した場合 |
| D-AUTH | active desktop sessionの同UIDを取得して変更許可、greeter/nonactive拒否。OS module getpeereid、identity失敗時拒否 | Settings専用capability/同UID他client拒否という新security policyが必要な場合 |
| D-PORT | 最初のfixtureは既往成功のeDP+HDMIに提案、Type-C/DP/MST欠落を能力表と後続残事項へ明記 | userの「外部」がType-C等を今回必須としている場合、又は既存WS受け入れから必要connectorを外す場合 |

D-IDはstandard implementation policy/私有APIに関わるarchitecture選択としてmainへ材料を提供する。D-ATOMICは要求解釈/physical gateとしてuserまたは既存の委譲authorityを確認する。current physical fixture可用性は製品選択ではなくp008 readinessの外部前提として未確認を保持する。

### 3.1 採択記録（2026-10-02）

mainのdelegated technical decision messageを受領: 初回全connected extended+internal anchor、辺で連結/非重複edge snap、退避窓の自動奪回無し、active session同UID peer検査/Settings限定secret無し、初回実fixture eDP+HDMI。contracts/WSと影響Phaseへ投影する。D-PORTは未移植portの成功/実装省略を承認した意味に拡張しない。

D-IDはA2としてnative display.name→標準displayNameでlocal port keyを渡すmain技術採択。私有Vulkan拡張は不採用、GPU UUIDqueryは別能力のまま必須実装に増やさない。旧boot hdmi/edpは初期preferred anchorを保持し、全connected inventoryを隠すdisable指定に転用しないmain採択。D-ATOMICはmainがuserへ質問中、回答前に採択しない。

## 4. A2の具体的schemaとsource対応（main技術採択）

- wire/displayName案: `zedbsd-port-v1:pci:0000:00:02.0:edp:A`（37byte、NUL込み38byte、Python ASCII lengthで照合）。segment4hex、bus2hex、device2hex、function1hex、kindは固定ASCII `edp`/`hdmi`等、portはphysical DDIのstable token。hex大小文字/先頭zeroをcanonicalにする。全体63byte以下、終端NUL必須、format/parseを同revisionで定義する。
- native source: `i915_device.pci`（i915.h54）、`drv_pci_device_address()`（公開pci.h304/pci.c701、既にi915/pci.c775で使用）でsegment/bus/device/functionを得る。HPD encoder.port（hotplug.c3137）はVBTの物理DDI portで、hpd.numやHDMI-A-<n>生成順を使わない。driver.queryのname[64]→libvulkan immutable name cache→standard displayNameの既存経路を使う。
- display_idもport固定mappingにし、世界のnum_encoders列挙順のid値をpersistent identityへ昇格しない。Vulkan globalplane indexはknown slotの固定順を維持する。MST branch等、version1で表現/実装していないconnectorは勝手にsuffixを作って完成扱いにしない。
- compositorは認識済みversion/PCI fields/kind/port/range/length/collisionを検査し、port keyをsession output_tokenへmapping。unsupported/unknown/duplicate schemeはpersistable=false、保存自動復元を拒否してworking配置へfallbackし通知する。既知schemaでもmode/capability/全memberを通常transactionで再validate。
- Settingsのlabelはcompositor/libkeiland snapshot内のkind/portから「内蔵ディスプレイ（eDP A）」「外部ディスプレイ（HDMI B）」等を作る。displayName内へEDID名を併記してhotplugで上書きしない。同一instance handleのimmutable string寿命を保持する。
- 保存keyはlocal machine/同PCI portの意味。GPU・PCI配置変更でkeyが変わればnew connectorとしてfallback、旧entryは保持。別machineへのfile移植で同BDFが一致し得るためmachine-global hardware serialの保証はしない。同種GPU交換で同portが同keyになる場合も、port-based mappingの範囲としてmode/capabilityを再検査し、old modeを強制しない。
- invalid saved file/unknown schema/stale dimensions/unsupported modeは部分適用せず、現working状態と失敗理由を公開。記録の例byte長はp002実装前にsizeof/ABIfixtureで照合する。

採択源: 2026-10-02 mainから本agentへのD-ID A2/旧boot anchor技術決定message。製品source変更は未実施。標準deviceUUID query不足と最新仕様の注意は上記のread-only能力記録として残す。
