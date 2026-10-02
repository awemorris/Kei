# q570 actual native GPU/input/VT acceptance

Wholep003 cleared. q569 positivewindow/nativeDMAerror/ownership receipts retained unchanged;
q570 supplied missing actualGPU VT/lifecycle/input acceptance. Earlier fixture sampled async
notification tooearly (0.5secondpause,1secondresume), not a broken native seat callback. Native
seatdordinarydebuglog independently proves VT_PROCESS owner, disable/ACK/clientrelease and
reactivation; final boundedpoll checks actualkernel process fd snapshots until retirement and
reacquisition (6secondsmaximum each), no production simulation or semantic codechange.

[Actual positive lifecycle](actual-gpu-lifecycle.txt): FreeBSD15.1-p4/drm66/IntelMesa26.1.3/IrisXe,
standard Vulkan sharedrenderer and unprivileged proper wltestGPUwindow90frames/200msdelay.
Active6nativeevdevfds→paused0→resumed6, seatd primaryowner retired/reopened. Readonly Vulkan
inquiry/renderfds may remain; no claim that allDRM inquiryfiles disappear. Native driver revokes
master before asynchronousdisplayrestore, expected Permissiondenied restore logged, resumed
realoutputswapchain and93compositorframes prove actual recovery. wltest exits0, servererror0/
cleanup_failed0. QMPactualUSBtablet movement/key-a injected only after restored inputleases;
input_events4 and native device path support realkernelinput, no synthesizedprovider.
Compositorservice logs/daemonlogs are ordinarySSH application observations, not QEMUconsolelog.

Owned prefix/socket/runtime/client/server/seatd removed. Independentconsole captures restore
VTactive/mode/keyboard/drawing/termios and inputfilepermissions even on earlierfailedchecks.
Initialdiagnosis and shortresume falsefailure retained; final actualreadback proven, no bugtransfer.
Fullmanualreview newPythonfixture checks ownership/finally/pid/runtime/positiveclient outcomes and
boundedwait; bytecompile/diffcheckPASS. NativeCsource unchanged from q569 finalreview/build.
ActualGPU F3 now verified with q568offscreen/nativechain and q569mappedwindow/preciseerror/borrowfd
receipts. Existing shared Linuxrenderer/input/session implementation reused, no copy/kernelport.
BUG130 upstreamdriverzeroaccess remainstracking with validated KeilandCPUcompletion workaround.
p005 will review q569nativechanges and actualmainapps/finaldocs. F4closure next; physicalWiFi
explicitlywaived, Venus remainsunsupported/unexecuted, userselectedi915route accuratelynamed.
WIP/no push/publication; ownremoteQEMU remainsrunning; hostoriginalVFIObinding unchanged.
