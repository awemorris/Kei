# WS113 p001: 出力・通知・設定・窓所属の契約案

Revision: 2026-10-02 / q586-i01 / A3 / source baseline `0e68854ac`。
Status: 設計調査。製品sourceの変更・実機実行は無し。
Authority: [WS全文方針](../../standards/ws113-display.md)とcurrent userの二択・pointer境界切替・zedBSD i915受け入れ。下記の通常技術設計を後続Phaseの入力とし、**未決表の選択を採択済みとして扱わない**。
Source evidence: [能力/source照合](source-audit.md)、[ID/完了保証の詳細比較](identity-completion.md)、[fixtureと検証](fixtures.md)。

## 1. 所有と境界

| 層 | 所有する情報/操作 | 上位へ公開するもの |
| --- | --- | --- |
| i915 / GPU core | connectorの検出、mode、generation、plane lease、実scanout完了、IRQからのイベントsequence | 既存GPU_DISPLAY_QUERY/MODE/CLAIM/PRESENT/WAIT/EVENTS。変更が必要なら所有境界とHAL事前承認を別途守る |
| libvulkan | GPU nodeの独立open、display/mode handleの寿命、native ACK、event fence、surface/swapchainの喪失、present完了 | 標準VK_KHR_display/WSIと実装・検証済み標準拡張。compositorへnative fd/lease/ioctlを漏らさない |
| compositor | 接続output集合、論理配置、全拡張/全mirror、窓の単一所属、設定適用と保存 | 専用Wayland管理拡張のsnapshot/通知/transaction。通常wl_output情報も同じ状態から投影 |
| public libkeiland | registry/version negotiation、protocol object寿命、snapshotの所有、結果/callback | Settings向け公開C API。未対応serverはENOTSUP相当。UI threadの既存Wayland dispatchでcallback |
| Settings | snapshotの表示、配置draft、Apply、結果と保存失敗の表示 | libkeiland公開APIだけを利用。GPU deviceと私有compositor内部symbolには触れない |

既存`keyboard-inset.c`の公開wrapper/registry交渉を参照する。独自protocolのXML、generated header、dispatch interface、公開API provenanceはp005で一緒に照合する。実装場所の追加/OS module境界はGuardrailに従う。

## 2. 識別子と世代

| 値 | 意味 / 寿命 | 保存 |
| --- | --- | --- |
| native device_id | 登録中のGPU identity。`gpu_handle_allocate()`の値で再起動/再登録の恒久identityではない | 不可 |
| native display_id | GPU登録寿命内の物理connector identity。ordinalや接続順から生成しない。切断/同じport再接続で同じ値 | 不可（device_idと組でも再起動保証無し） |
| output generation | 接続・支持mode/capability/lease契約が失効するepoch。通常frame、支持mode内の選択、純power設定、単なる列挙、別outputの変化では増やさない | 不可 |
| topology sequence | device全体の表示inventory変更。1から始まり、ACK対象でありoutput generationとは別 | 不可 |
| VkDisplayKHR | instance寿命のhandle。切断しても破棄/別connectorへの再利用をしない。mode handleはgenerationに結ぶ | 不可 |
| compositor output_token | session内で重複/再利用しない64bit token。native keyへの内部対応を保持 | 不可 |
| topology_serial / config_serial | 完全snapshotと適用状態を区別するcompositorの単調64bit値 | 不可 |
| persistent connector key | main採択A2のlocal PCI segment:BDF+kind+DDI port。native name→standard displayNameのimplementation policy | schema/一意性/mode/capability検証後のみ可 |

切断時は当該generationを失効させ、再接続では新generation。stale mode/lease/surfaceは旧outputへ副作用を起こさない。上限に達したsequence/generation/tokenをwrapして再利用せず、terminal errorを返す。いずれも固定1の現在のi915実装とは異なる後続目標。

`VkDisplayPropertiesKHR.displayName`はNULLも許される名称で、通常EDID由来、instance中不変のUTF-8 string。[一次仕様](https://docs.vulkan.org/refpages/latest/refpages/source/VkDisplayPropertiesKHR.html)には一意・再起動後永続・同一monitor識別の保証が無い。EDID名の一致、VkDisplayKHR値、ordinal、`/dev/gpuN`名を保存用IDにしない。

D-IDは2026-10-02 main技術採択A2: local machineのPCI segment:BDF+connector kind+物理DDI portをversion付きkeyにし、native name[64]→標準displayNameの既存経路で伝達する。例`zedbsd-port-v1:pci:0000:00:02.0:edp:A`。同一machine/同PCI portに保存範囲を限定、hardware/PCI配置変更やconfig別machine移植後の恒久GPU identityを保証しない。ordinal/conn_nameは使わない。unknown/invalid/collisionは復元拒否、mode/capability再validateを必須とする。人向けlabelはkind/portからsnapshot/Settingsで表現し、同handleのEDID名上書きをしない。[source/grammar/比較詳細](identity-completion.md)を参照。私有Vulkan identity拡張はmain不採用。GPU標準UUIDはi915 nativequery未実装の別能力として残し、この採択へ必須追加しない。

## 3. i915 HPDとnative ACK

HPD IRQの到着そのものではinventoryを変更しない。既存HPD workerで接続検出・mode情報を確定し、connectorとgenerationを短いlock内でpublishした後、device topology sequenceを進める。GPU契約どおりIRQ-safe events callbackはsleep/allocate/clear/hardware commandをしない。`poll_notify()`はpublish lockを解放してから行う。切断は当該lease/presentを失効させ、新しいsubmitを拒否し、保持中bufferとscanoutが安全に退役するまでreferenceを保持。

`GPU_DISPLAY_EVENTS` QUERYは非破壊、ACKはそのopenで観測済みのsequence以下のみ。dup/SCM_RIGHTSは同じopenのACKを共有し、別openは独立。copyout成功後にだけ観測/ACKを進める既存core契約を維持。SをACKする間にS+1がpublishされてもPOLLPRIは残る。IRQの数やplug/unplugの全履歴を要求せず、coalesced通知から最新inventoryを再構成する。

複数output化は単一`rd`/lease/planeからconnectorごとの状態へ分ける。pipe/PLL/plane/bandwidth割当を全接続集合でvalidateし、他outputが使用中の資源を奪わない。2台を列挙しただけでは2台同時claim/present可能と主張しない。現在のmode validateはnative timingへのscaleだけで、retiming/複数refresh選択は未実装。対応modeだけを列挙し、要求だけを受けた偽成功を返さない。

native inventoryは既知の物理connector slotを切断時も保持し、CONNECTED flagだけ変える案。KHR_display側で接続displayだけを列挙する。global Vulkan plane indexはphysical device全体のindexなので、接続済みlistを詰め直して別planeへ再解釈しない。現wsi.cはoutput毎plane_countの連結なので、固定slot/plane mappingとmode/surface生成前後のcoherent inventoryをp003で監査する。native count0とconnected count0を混同しない。

## 4. Vulkan device event fenceと独立consumer

[VK_EXT_display_control](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_display_control.html)はdevice revision 1で、依存はinstance `VK_EXT_display_surface_counter`とdevice `VK_KHR_swapchain`。4 entry（device event、display event、power、counter）がある。hotplugだけを実装して拡張全体を広告しない。surface counter側は`vkGetPhysicalDeviceSurfaceCapabilities2EXT`を含める。counter bitは実際に提供できるもののみ（0も可能）。power/first-pixel-outも能力と結果をsource/実機で検証する。

現在GPU display UAPIにpower/next-first-pixel/vblankcounter操作は無い。[native capability結線案](native-contract.md)を後続review入力とし、lease-owned WAITをidle display eventへ代用しない。p002/p003で標準entry全体に必要なnative能力の差分設計を提示し、共有/HAL APIの所有・事前承認を守る。新APIを本設計で採択した扱いにしない。

[vkRegisterDeviceEventEXT](https://docs.vulkan.org/refpages/latest/refpages/source/vkRegisterDeviceEventEXT.html)は新しいVkFenceを返す。DISPLAY_HOTPLUGはplug/unplug時の再列挙の契機であり、native seqやACKをapplicationへ公開するAPIではない。[イベント定義](https://docs.vulkan.org/refpages/latest/refpages/source/VkDeviceEventTypeEXT.html)を適用する。

### 4.1 ライブラリ内monitor

1. device monitorはlease用fdと別のnative **open**を所有する。dupでACK独立性を作らない。lease/swapchain無し、0接続、idleでもPOLLPRI/device lossを監視する。現在の`display_progress()`だけではこれを満たさない。
2. open直後のsequence 1はinventory取得が必要というbaselineで、plugを捏造しない。QUERY S0→全connector inventory/各generation→QUERY S1と照合し、同一sequenceの完全snapshotをpublishした後だけACK S1。同数connector交換もcount比較だけで見逃さない。
3. 各event fenceは登録時にQUERYしたcursorを持つ。monitorのcursorと別に、pending fenceごとに`cursor < observed_sequence`を評価し、同じ通知を全対象fenceへlatchする。Aのfenceをobserve/destroy/resetしてもBのfenceを消費しない。別VkDevice/別processは独立openで確認する。
4. 登録時のQUERYとpending listの公開をmonitor処理とserializeし、間に起きたsequenceを落とさない。ioctl・allocator callback・thread joinを短いregistry lock中に行わない。workerはapplication callbackを呼ばず、library内payload/conditionのみをpublishする。
5. eventは一度latchするとsignaled状態を保つ。clientはsignal後に**新fenceをregisterしてからinventoryを再列挙**し、旧fenceをdestroyする。signalから再登録までの変化は再列挙で回収し、その後の変化は新fenceで回収。snapshotが競合したらdispatchごとの有限retry（例3回）後に次turnへ回し、spinしない。

上記one-shot/re-registerはこのclientと実装の安全な運用契約である。参照した一次ページはresetによる再登録を定義しておらず、明示的な「one-shot」という規範文も今回見つけていない。resetを再登録APIとして説明しない。

### 4.2 fence寿命とcore syncへの影響

専用payload kindとしてpending/cursor/latchedとmonitor referenceを持ち、通常render fenceのnative wire IDやproducer completionに代入しない。`vkGetFenceStatus`、wait-all/any/timeout=0/有限/無限、destroy、device teardown、external fence import/reset復帰を一体で監査する。既存wait loopのrenderer/external-fdだけではdisplay通知を待てない。destroyはpending listから外し、monitorの借用referenceが退役してからallocatorで解放し、device teardownでmonitor停止/join後にfdをclose。

[vkResetFences](https://docs.vulkan.org/refpages/latest/refpages/source/vkResetFences.html)はunsignal操作であり、既にunsignaledなら効果無し。したがってpending eventのresetで監視を取消してはならない。latched後resetはunsignalし、過去のeventを再playしない（以後のhotplugを捕捉するcompositorは新登録を使う）。core fenceのhost external synchronizationとtemporary importの復帰を保持する。signaled状態をqueue submitへ再利用する場合など、coreで合法な操作は新payloadとの切替を監査し、独自禁止を追加しない。

[vkDestroyFence](https://docs.vulkan.org/refpages/latest/refpages/source/vkDestroyFence.html)の未完queue使用禁止・allocator一致・host external synchronizationを保持する。未発火display登録はlibraryが安全に取消す設計とし、「plugするまでdestroy不可」とは書かない。切断は対象surfaceのSURFACE_LOST/OUT_OF_DATEとして扱い、正常な別outputをdevice-lostに巻き込まない。GPU自体の喪失は別のterminal device error。display-event fenceが切断中に未発火ならcore waitへSURFACE_LOST等の許されないerrorを追加せずpendingのまま、同handle再接続後の次実refreshで発火できるようgeneration/cursorを安全にrebaseする。切断自体をfirst-pixelとしてsignalしない。

### 4.3 header/広告/完全性

p003は維持header、API-PROVENANCE、api-commands.tsv、dispatch/gating、instance/device extension bits、struct layout、正しいpNextとallocatorを同時に更新・検証する。既存宣言入力はVulkan-Headers 1.3.269に対応するがcore広告は1.0のまま。declared=enabled=runtime-supportedを混同しない。未enable/device unsupported時のprocaddrとextension enumeration、LP64/ILP32のABI、既存render/external sync regressionを検証する。shared toolchainを変更/buildしない。

## 5. 0/1/2出力と適用の状態

| 接続状態 | compositor契約 |
| --- | --- |
| 0 | device/watchとWayland serverを維持し、presentしない。窓/設定を保持し、pointerをpark、Settingsにはno-output snapshot。初期count0でserver全体をINITIALIZATION_FAILEDにしない |
| 1 | 1outputで描く。選んだextended/mirror preferenceを保持し、見た目が同じでも二択を勝手に変更しない |
| 2以上 | 全接続outputを選択modeに参加させる。資源不足/unsupportedを成功扱いで無視しない。既存working outputを維持し、degradedと原因を通知する |

output内部状態はdetected→validated→claimed→active、切断/失敗でretiring→gone。render schedulingを停止してから古いsurface/swapchain/leaseを退役させる。再接続は新generationでvalidate/claimし、保存配置の適用可否を再検査する。再接続しただけで退避済み窓を奪い戻さない（main通常技術採択D-REC）。0台→最初の1台でparkした窓を一つのownerへ復帰。

設定transactionはexpected topology_serial/config_serialと全接続output_token+generationを含む。modeはEXTENDED/MIRRORのみ。接続集合の欠落/重複、stale token、座標overflow、不支持modeは副作用前に拒否する。compositorが唯一のwriterで、並行applyはbusyまたはstaleを返す。

stageでmode/資源/配置をvalidateし必要allocationを準備、topologyを再照合、apply、実present確認、snapshot publishの順。native全headの同時commit保証は現在無いため、apply失敗は旧状態へのrollbackを試みる。rollback失敗時は実際のactive/degraded状態を新snapshotとして公開する。全体成功を捏造しない。topology変化はtransactionをstale扱いにし、残る出力を保持して再構成する。

## 6. mirrorとextendedの座標

### mirror

1論理desktopの全内容を各outputのnative supported modeへGPUでaspect-fitする。各outputで独立swapchain/画像extentを持ち、共通解像度やdriver scaler/retimingを必須にしない。黒いletterbox/pillarboxをopaqueに塗り、切り捨てcropをしない。source W×Hに対しscale=min(outputW/W, outputH/H)、整数viewportは範囲内へ丸め、正の寸法を検査する。四隅markerと文字が全outputで見えることを実機で確認する。

logical desktopのanchor/sizeは保存anchor、無ければmain通常技術採択D-BOOT（初回extended、internal anchor）で選ぶ。anchor切断で新anchorへ1回reconfigureし、残る窓とpointerをclamp。全displayは同じ論理contentとcursorを表示するが、refresh/flipは独立であり同時vblankは保証しない。touch/absolute inputはoutput viewportの逆変換を使い、black bar入力は境界へclampする。

[VK_KHR_display_swapchain](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_display_swapchain.html)はsrc/dst rectangleとshared swapchainを提供するが、異寸法の全head同時latch保証ではない。compositor自体のGPU transformを基本案とし、共有imageが必要な場合は標準の同一format/extent等の条件を別途満たす。

### extended

logical座標はsigned global origin、各outputはhalf-open `[x,x+w) × [y,y+h)`、scale=1を初期範囲とする。負のoriginを許し、寸法/加算はint64で検査してprotocol int32へ収める。outputlocal=global-outputorigin、client local座標はcontentrelative。DPI設定の追加はこのWSで決めていない。

配置draftは矩形が重ならず、辺の正の長さで連結する形へsnapする（main通常技術採択D-LAYOUT）。cornerだけの接触と隙間を横断可能な隣接として扱わない。例えば1920×1080@(0,0)、1920×1280@(1920,0)ではx=1919は左、x=1920/y<1080は右、y=1100の水平移動は共有edgeがなく左境界へclamp。drag configurationのvalidateもこの非重複/辺連結を強制する。

input.cの単一width/height clampをoutput集合の境界に置換し、相対motionの線分が通過するshared edgeを順に処理する。大きいdeltaで飛び越す場合も対象を一意に決め、空白領域へのteleportをしない。absolute inputの属する物理outputが判明しない場合はnative input契約を調査し、2台touch対応を仮定しない。

## 7. 窓全体の所属・drag・退避

top-level rootにoutput_tokenとownership_epochを持たせ、subsurface、popup、transient、decoration、shadowはrootのownerを継承する。extendedのrender list/damage/cache/scissorはowner以外のoutputへtreeを一切追加しない。rootのboundsが隣画面へはみ出してもownerのviewport内でclipする。fullscreen/maximizedのtarget sizeもownerから決める。

drag開始でglobal pointerとroot positionのgrab offsetを保存する。motionでpointerがshared edgeを越えた時、motion dispatch内でtree全体のownerと位置を一度に更新し、旧/新outputをdamageし、wl_surface enter/leaveと必要configureを整合させてからbutton処理へ進む。新local root位置はglobalPointer-grabOffset-newOrigin。targetが小さい時はtitle/grab pointへのアクセスを保ち、client resize未完了でも旧bufferを単一output内にclipし、隣へ部分描画しない。drag中の高速往復はepochで古いrender workを識別する。

disconnect中は当該ownerへの新描画を先に止める。残るpreferred output、無ければstable keyの一定順でroot treeを退避し、title/grab pointを範囲内へclamp、popupのparent関係を保つ。残るoutput無しならtreeをparkし、client buffer referenceを安全に保持。単なる接続復帰は保存layoutを戻すがwindowの退避先を勝手に戻さない（main通常技術採択D-REC）。切断中dragはgrabをcancelまたは新owner上で再baseし、stale pointer/sourceを次buttonへ渡さない。

### 7.1 logical ownershipと物理scanoutの差

[vkQueuePresentKHR](https://docs.vulkan.org/refpages/latest/refpages/source/vkQueuePresentKHR.html)のenqueue成功は表示完了ではない。既存`vkQueueWaitIdle`も`present_drain()`のaccepted job完了まででfront scanoutを除外する。i915 PRESENTはflip armで戻り、WAITで最新latchを待つ。したがってlogical ownerの原子更新だけでは、旧headが前frameを走査する間に新headへ窓が出る重複を排除できない。

D-ATOMICの厳格な単一物理出力案は、旧ownerの窓無しsceneをsubmit→**そのpresentの表示完了を確認**→新ownerで窓をsubmitの順にする。旧ownerの既にqueuedな窓有りsceneも先行順序をdrainし、新しいepochのsceneで置換する。瞬間的な不表示期間を伴う。render fence、FIRST_PIXEL_OUT単独、queue idleを消去frameの証明へ代用しない。

completion候補は標準[VK_KHR_present_wait](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_present_wait.html)と[VK_KHR_present_id](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_present_id.html)。この追加は未採択。features/依存、presentID→native sequence mapping、実latch、切断時のout-of-dateと成功の意味をp003で設計する。[vkWaitForPresentKHR](https://docs.vulkan.org/refpages/latest/refpages/source/vkWaitForPresentKHR.html)は表示時刻との精密関係を要求せず、OUT_OF_DATEでも表示された可能性でSUCCESSを許す。latch/start scanoutは全旧pixels消去と別で、このAPIの存在だけでstrict物理無重複を主張しない。[追加保証/失敗/timeoutの詳細](identity-completion.md#2-standard-present-completionと厳格移動の限界)を適用する。両headのrefresh境界を同時にするなら別の下層commit能力が必要で、現在確認できていない。

## 8. 専用Wayland管理拡張 / public libkeiland

以下はoperation/ownership契約案で、symbol名とwire signatureはp005で確定する。

| operation/event | payloadと検査 |
| --- | --- |
| manager bind/get_snapshot | negotiated version。begin(topology_serial, config_serial, mode, health)、output(token,generation,persistent keyの可否,label,geometry,mode/refresh,active)、done。doneまで部分snapshotをUIへ渡さない |
| changed | 新snapshotを同じbatch形式で送る。切断/外部apply/復旧を同じ権威状態から通知し、in-flight Apply結果と区別 |
| create_configuration | expected serialsとrequest_id。各outputは現snapshotのtoken/genを参照し、mode+全memberとextended座標をdraftへ記録 |
| apply | stale/invalid/unsupported/unauthorized/busy/backend_failed/rollback_failedを分ける。実状態snapshotと対応serialを返す。fatal protocol errorは構造破損に限定し、通常設定失敗でclientを切断しない |
| result | applied と saved を別に返す。disk失敗なら適用状態を表示し「保存できなかった」と通知。request_id重複/後着snapshotも照合できる |
| destroy | draft破棄は副作用無し。client切断で処理中configuration referenceを退役し、partial wire stateを保存しない |

64bit値はprotocolでhi/lo uint32等の固定表現を決め、native pointer/lease/GPU fdを送らない。string/count/座標/mode数をboundedにし、checked allocation/overflowを行う。libkeilandはcallback snapshotの寿命を明記し、UIがApply中のdraftをcloneして保持できるAPIを提供する。現在の固定1 wl_output registryからoutputごとのglobalにし、registry remove/addとgeometry/mode/scale/doneを一致させる。

権限案はactive desktop sessionのUIDとUnix socket peer credential一致。現在のacceptはその検査無し。公開`getpeereid()`とunix-socket SO_PEERCREDが実装済みなので、OS-specific backendで取得し、application名による信頼にしない。greeter/seat非active/credential取得失敗では管理applyを拒否する。照会と変更の権限を分ける。main通常技術採択D-AUTHとしてactive session同UIDの別clientも変更を許可し、Settings限定secret認証を追加しない。既存desktop-role tokenを許可根拠へ転用しない。

Settingsは通知で現在値を更新し、ユーザーdragはdraftだけを変更する。表示mode二択、拡張時のoutputカードedge snap、Apply/取り消し、stale時の再読込、unsupported時の具体原因を提供する。drag毎のhardware modesetを避け、Apply後にresult/snapshotで確定する。mirror時は配置dragを無効化し、native output別のmode/scale状態を案内する。

## 9. 保存と復元

compositorがdisplay設定の保存を所有し、既存desktop.confとは別のversion付きdisplays.conf相当へ書く。libkeiland preferencesのtmp→flush→rename手順を参照し、所有module/permission/fsync必要性をp005で決める。GUIがdriver状態と別に設定fileを書かない。

保存内容はmode、採択済みpersistent connector keyごとのextended origin、mirror anchor。generation/token/leaseは保存しない。切断したentryは保持し、temporary unplugで既定配置へ上書きしない。起動時に不在portを除いた有効subsetを検査し、初出力/新connectorにはdeterministic fallbackを適用、無効/旧version/一意性無しなら現working配置を保持して理由を通知する。保存データの復元も通常transactionと同じvalidateを通す。

## 10. 未決と通常提案の選択材料

| ID | 未決内容 / 選択肢 | 事実と影響 / 待つ後続 |
| --- | --- | --- |
| D-ID | main技術採択A2: local PCI segment:BDF+kind+physical DDI port keyをnative name→standard displayNameへ | 同machine/同PCI portに限定。mode/capability再validate、scheme拒否。私有API不採用、UUID能力は別。p002/p003/p005 |
| D-ATOMIC | logical ownerの同時更新を受入解釈 / 物理重複無しで短い不表示期間を許容しsource completionを追加 / 物理2head同時latchを追加要求 | 現標準/driverには同時latch保証無し。p003/p004/p007/p008 |
| D-BOOT | main通常技術採択: 保存優先、初回全接続extended+internal anchor。旧hdmi/edpは初期preferred anchor | 全connected inventoryを隠すdisable指定に転用しない。p002/p004/p005 |
| D-PORT | main通常技術採択: 最初の受け入れfixtureはeDP+HDMI | 全接続要件から他portを削除した判断ではない。Type-C/DP/MSTの未移植/後続不足を明記し成功を主張しない。現fixture不明。p002/p008 |
| D-LAYOUT | main通常技術採択: edge snap、非重複、辺で連結 | signed origin/half-open/shared edgeを検証。p004/p006/p007 |
| D-REC | main通常技術採択: output layoutだけ復元、退避窓は現ownerに保持 | 0台park→1台復帰を含む。p007/p008 |
| D-AUTH | main通常技術採択: active session同UID変更許可、nonactive/greeter拒否、Settings限定secret無し | OS-specific credential検査を実装。既存tokenを転用しない。p005 |

Authority: 2026-10-02 mainから本agentへの通常技術設計採択message（D-BOOT/LAYOUT/REC/AUTH/PORT）。D-ID A2と旧boot preferred anchorもmainが採択、私有Vulkan拡張は不採用。D-ATOMICはuser回答待ちで未採択。mainへ材料を送付済み。[裁量境界](identity-completion.md#3-main技術裁量へ渡す通常提案)を参照。全7件をuser必須選択にはしない。material architecture/要求解釈/受け入れ範囲は既存authorityと照合し、main技術裁量の通常採択はsourceを記録する。未決を受け入れ条件の緩和で埋めない。

## 一次仕様の確認時点とvalidity

この文書の全Vulkan linkはKhronos Vulkan Documentation Projectの該当一次ページを**2026-10-02**にopenして確認した。`latest`は可変URLで、調査時のextension revision（EXT control/counter 1、KHR display 23、display_swapchain 10、present_id/wait 1）を記録。後続p003で維持宣言のpin（現在Vulkan-Headers 1.3.269）とVUID/dependencyを再照合する。最新仕様を既存1.0実装の能力証明とは扱わない。one-shot運用、stable key案、physical transitionの選択は仕様からの推論/設計として明示した。
