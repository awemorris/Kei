# WS113 p001: native capabilityと標準EXT entryの結線案

Date: 2026-10-02 / q586-i01 / baseline0e68854ac。
Scope: 文書設計のみ。[契約](contracts.md)/[完了保証](identity-completion.md)/[fixture](fixtures.md)のp002/p003への補足。
Status: semantic operation/ownershipのreview入力。下記は製品API実装、exact ioctl番号、common callback ABI、新HAL APIの承認ではない。後続の有限Queue選定前にmainが実差分と所有/必要チェックを照合する。material製品選択を新たに追加していない。

## 1. 現在のnative面

`gpu-display.h`はQUERY/MODE/CLAIM/RELEASE/PRESENT/WAIT/EVENTSを持つ。IRQ-safe eventsはsequenceのみ。power操作、lease無しnext-first-pixel event、実vblank counterは無い。既存WAITはleaseのpresent完了なので、新refreshのfirst-pixel eventとして再利用しない。

| 標準entry | native semantic入力 / 出力 | 既存能力 / p002側必要出力 |
| --- | --- | --- |
| vkRegisterDeviceEventEXT | device、登録cursor→次HPD plug/unplug後のsequence | EVENTS/QUERYとpoll有り。i915の固定1を実HPD inventory publicationへ変える |
| vkRegisterDisplayEventEXT | native display_id+generation、登録時refresh cursor→次refresh first-pixel boundary | lease無しconnector-specific timing observation不足。render fence/virtualframeを代用しない |
| vkDisplayPowerControlEXT | native display_id+generation、ON/OFF/SUSPEND→実状態の成功/失敗 | power op不足。i915 output/worker/modeset/backlightの所有を保持した操作が必要 |
| vkGetSwapchainCounterEXT | surfaceで支持/作成時enableしたcounter、display/gen、swapchain初回present以降→vblank値 | 実counter query不足。supportedSurfaceCounters=0は合法なcapability案だが偽counterを返さない |
| vkGetPhysicalDeviceSurfaceCapabilities2EXT | surface→既存surface caps+supported counter bits | libvulkan wrapper/ABIを追加。virtual/non-display/未能力にはVBLANK bitを付けない |

sourceのpower不足やtiming不足を一般UNKNOWNの恒常stubで埋め、full EXT対応と書かない。p003のadvertisementはentry/validity/実能力/合法なfailure pathのreview後。

## 2. timing operationの契約案

新native query/waitが必要なら、version/size、display_id、generation、operation、cursor/timeout、出力capability/counter/stateを持つ、driver-owned display operationとして局所化する。exact symbol/field layout/番号はp002選定の差分で確認する。

- QUERY: 副作用無し。connectorのpower/refresh状態と実能力、単調64bit refresh/first-pixel observationを返す。CONNECTEDとPOWER/ACTIVEを区別。capability0は未対応を意味し、値0を「対応clock開始値」と取り違えない。
- WAIT_NEXT_REFRESH: 登録時cursorを超える実first-pixel eventをwait。lease/active swapchainは要求しないが、pipeが停止中はeventを捏造しない。caller有限timeout/0timeoutを守る。再接続gen変更はESTALE、切断は対象ローカルerror、GPUofflineはENODEV。
- QUERY_VBLANK: active surfaceで支持する場合だけ実counterを返す。driverの有限hardware counterは64bitへ拡張し、wrapで後退させず、generation変化を併記。kernel tick数/nominal refresh/successfulpresent数を実vblankへ置換しない。
- IRQ: 既存pipeの実first-pixel/vblank境界を確認後、短いlockでcounter/eventをpublishしてwaiterをwake。wait/register/cancel/closeとのraceをreferenceで処理し、callback内でallocate/sleep/ioctl相当をしない。monitor threadがapplication callbackを呼ばない。
- shutdown: pending waitはdevice teardown/cancelでwakeしてjoin可能にする。無限ioctlをholdしたままlibrary mutex/device destroyを待たせない。exact native取消methodまたは有限blocking+明示shutdown wakeをsource差分で確認する。

vblank IRQの時点とfirst pixelがengineから出る時点は同義と仮定しない。pipe状態・scanline/hardware docsとi915 workerを照合し、単なるvertical blank開始をFIRST_PIXEL_OUT成功へ変換しない。capability/未確認理由をH09/H10へ記録する。

## 3. power operationの契約案

[VkDisplayPowerStateEXT](https://docs.vulkan.org/refpages/latest/refpages/source/VkDisplayPowerStateEXT.html)を2026-10-02確認。ONは点灯、OFFはpower down、SUSPENDは低消費電力でOFFと同じ状態でもよい。ここで高速resume保証を追加しない。

- native powerはdisplay_id+generationの選択、input enum/reserved0/authorityを副作用前に検査。write権限無し、foreign exclusive leaseとの衝突、unplug/旧generationを拒否する。
- output owner mutexでworker/modesetをserializeするがHPD registry/IRQ lock中に長いpower処理をしない。操作中HPDを失わず、完了後にgeneration/connectionを再確認する。
- OFF/SUSPENDはnew presentを停止し、pending flip/scanout bufferを安全にretireした上でoutputを止める。kernelがまだ読むbufferをfreeしない。ONは接続/mode/plane/capabilityを検証してresumeし、失敗は実状態を返す。
- power stateをplug/unplugと誤通知しない。CONNECTEDは物理検出のまま。通常の選択mode/power変更だけでdevice hotplug sequenceを進めない。generationは支持mode/接続/lease契約が失効する場合だけ進め、純粋な設定変更とは区別する。
- first-pixel fenceはOFF中にfake signalしない。resume後の次実refreshを待つ。counterの停止/再開/epochは能力契約を明示し、以前の値を若返らせない。

この操作をSettingsのON/OFF選択肢へ追加しない。製品のdisplay構成は全connectedのextended/mirror二択のままで、標準extension全entryの実装責務として扱う。

## 4. counterのvalidityとswapchain所有

[VkSwapchainCounterCreateInfoEXT](https://docs.vulkan.org/refpages/latest/refpages/source/VkSwapchainCounterCreateInfoEXT.html)は作成時にsurfaceが支持するbitだけをenableする。[VkSurfaceCounterFlagBitsEXT](https://docs.vulkan.org/refpages/latest/refpages/source/VkSurfaceCounterFlagBitsEXT.html)のVBLANKはdisplayの実vertical blank毎に増えるcounter。[vkGetSwapchainCounterEXT](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetSwapchainCounterEXT.html)は少なくとも1 presentがengineで処理されたswapchainに対するqueryで、out-of-dateで値不可ならOUT_OF_DATEを許す（全て2026-10-02確認）。

libvulkanはswapchainのenabled bit/first processed present/generationを保持し、native counterを同outputへ対応づける。enqueue時点でcounter-activeにしない。nativeclock未能力/virtual captureならsurface capabilityを0にし、fake60Hz counterを作らない。client requestが不支持bitを指定した場合は標準validityを守る。

## 5. failure domain / ownership

| native結果 | libraryでの扱い / 証拠 |
| --- | --- |
| EAGAIN | observation未達。core fence statusならNOT_READY、timeout=0のwaitならTIMEOUT等、各Vulkan commandの許容resultに合わせる |
| ETIMEDOUT | 有限wait expiry。event登録自体を失敗/消費させず、fence waitはTIMEOUTを返す |
| ESTALE / connector disappearance | 対象mode/surface/swapchainの失効。commandごとのSURFACE_LOST/OUT_OF_DATE/UNKNOWN等をvalid return listに合わせる。device全体のfatal stateに直結しない |
| ENODEV / GPUoffline | render deviceもofflineか確認し、device lossのterminal domain。正常peer outputの局所切断と分離 |
| EACCES/EPERM/EBUSY | power authority/exclusive owner競合。成功にせず、標準entry許容のUNKNOWN等へ変換し内部診断を保持 |
| ENOMEM | command許容のHOST/DEVICE_MEMORY errorへ所有allocator/nativeallocationを区別して変換 |
| EOVERFLOW / impossible framing | epoch/counter再利用を拒否。native identity再構築/terminal output/device判断を記録し、wrapした成功を返さない |

未発火display-eventがunplugでnative ESTALEになっても、core fence waitへSURFACE_LOST等を新設しない。pending fenceを取消すのはhost destroy/teardownであり、同handleの新generationへrebaseして次実refreshを待つ。device eventは実plug/unplugでsignalする。registerDevice/displayEventのreturn listとcore fence waitのreturn listは異なる。libvulkanの共通display_error()を全新entryへ無検査で当てはめない。各entryのpNext、allocator、同physical parent、enable gatingを確認する。

per-open timing observationとtopology ACKは別にする。native ACKを次refreshのsignalとして使わず、同deviceの別fence/consumerを独立に保つ。per-output bufferとleaseは最後のGPUsample/scanout referenceをretireするまでpinする。

## 6. 後続選定への具体的入力

p002: 上記power/timing/capabilityを既存i915 display workerとGPU coreのどこが所有するか、source差分候補・単位・timeout/wake・close/cancelを有限設計してから実装する。新共有APIは該当owner/main reviewを受け、HAL API変更なら事前承認。GPU標準UUID opcode148未実装は別能力で、D-ID A2に不要なのでp002必須実装へ増やさない。

p003: standard declarations/pinned header/dispatch/procaddrを揃え、native仕様が実際に成立するentryだけ広告し、独立clientでH04–H10を検証する。D-ATOMICが強いsource消去保証を要求した場合、その追加native completionはこのfirst-pixel/vblank queryと別gateにする。

現時点ではIRQ first-pixel、power resume、実vblank counter、physical all-frame消去の実能力は未検証。この不足は後続Phaseの設計/実機gateとして保持し、p001の読取証拠から実装済みと主張しない。
