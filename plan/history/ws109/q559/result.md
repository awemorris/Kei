# q559 / native seat authority partial result

Native OS/seat modules and real authorization/lifetime subset cleared. Fullnativecompositor integration
and actual GPU/positivefence/display/physicalVT/input acceptance remain F3/F2/p005. No duplicate renderer,
Linux console ioctl or successful native stub. Native default KEILAND_SEAT=seatd, other explicit
providers refused; supported LIBSEAT_BACKEND=seatd forces verified MITclient instead of noop/basu.
Exact absolute primary path/default /dev/dri/card0 fixed before Vulkan inquiry; real daemon primary
requests/refusals and native Vulkan AcquireDrmDisplay/ReleaseDisplay ownership interface. Native seatd
owns native VT behavior. One fd and one protocoldeviceID per lease; bounded activation, nonblocking/
CLOEXEC, borrowed callback storage retained to finalclose, repeated partialcleanup safe.

Disable: publishpause→quiesceframe→retireoutput→commoninputnotifications and fd/lease retirement→
partialprobe/primaryretirement→daemonACK. Enable: reopen requested real primary beforeunpause,
windowed0/dirty1/rescan0, never reuse old input fds. Daemonloss marksfailed and ordinary cleanup closes
local files even when protocolreturn fails. Finalclose drains no callback that can reopen resources.

[Native build](native-seat-build.txt): FreeBSD15.1-p4/baseClang19.1.7 actual privateclient/nativeheaders,
`cc -O2 -g -std=gnu17 -Wall -Wextra -Werror -fPIC -I. -Iuserland/desktop/keiland
-Ibuild/native-ui/include -I/usr/local/include`, both OS/seat objects warning0. Currentnativepackage
still libraries only; compositor integration comes next, this objectproof is not a linked GUI service.

[Native tests](native-seat-test.txt): actual FreeBSDportsseatd daemon/nativekernel input, client
uid/gid65534/nogroups. Real absentprimary→ENOENT, no primaryfd, displayacquire rejects before nonexistent
Vulkaninstance; repeatedpartialcleanup/pollcount0. Real default VT-bound daemon, independent controller
VT_ACTIVATE1→2→1 delivered actualdisable/enablecallbacks. Renderer/commonnotification collaborators
only record callorder (no GPU/frame/protocolnotification acceptance); their recordedquiesce/outputclose
precede actual attached input fd/servicelease close. Independent fcntl proves EBADF for attached and
unattachedprobe handles before pause finishes. Reactivation sets normaldiscovery/outputflags, fresh
realmouse fd/nativeEVIOCGNAME with NONBLOCK/CLOEXEC PASS. Actualdaemon terminated while a new real
inputlease remains; actualpoll/dispatch loss setsfailed, localfileclosed/EBADF even serviceunreachable,
finalbufferedcallbacks cannot unpause. Ownprocess/socketcleaned, native VTactive/mode/keyboard/drawing/
termios/inputpermissions restored by independent native ioctl readback. QMP loginPNG also verified.
QEMU native VT notifications are separate from physicalGPU/VT validation, not a physical claim.

Initial test exposed SIGBUS from storing listener on connect's stack: libseat retains the pointer.
NativeLLDB/core backtrace showed libseat dispatch calling a stale address (zwl_os_poll_done).
Fixed durable file-scope listener, added finalclose callbackguard, sameactual test nowPASS. Initial
controller incorrectly compared inactive VT_AUTO signal fields; kernel keeps those unused fields.
Restoration verifies effective mode/waitv and every active console property; it does not claim to
clear ignored stale signal numbers. Controller restores properties on everyfailure as well.

FullC manualreview all newownC covers declaration/function/staticorder, semanticcomments/successpaths,
flags/callback/deviceIDs/partialfdcleanup/error/lifetime. clang-format19.1.7 applied retaining mandatory
argument/comment/three-clause layout, style-check0; Pythonbytecompile/diffcheckPASS. Tests repeated after
finalsource formatting. No make check/push/remotepublication/hostinput/display/toolchain mutation.
OtherOSsource unchanged; finalallWS/threeOSbuild/boot gates retained. Local outbox pendingpublication.
