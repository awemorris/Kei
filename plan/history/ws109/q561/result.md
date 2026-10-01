# q561 / native application libc adapters partial result

NativePTY/attribute prerequisites cleared, wholep002/F2 remains uncleared until app/nativeGUI integration.
Shared application C source unchanged. Native private build pty.h selects base libutil declarations;
sys/xattr.h exposes only actualexisting filemanager calls, native FreeBSD implementation in
freebsd-compat/freebsd/xattr-freebsd.c linked into private libkeiland-compat.a. No publicKeilandABI,
broadOSmacro, Linux implementation import or false-success missingfeature. Native get/file/fd/link
object identity, user./system. namespace mapping, nativeerrnos retained. Unknown Linuxsecurity/trusted
namespaces and nonzero atomicflags refused ENOTSUP rather than falsely emulated with races.
Native extattr can truncate a value; adapter reads one guardbyte into ownedstorage and refusesERANGE
before callerpublication. Native length-prefixed list snapshots validated for complete entries/
embeddedNULs/overflow; growth filling guardbyte→EAGAIN, complete NUL-separated prefixedoutput only.
Deniedsystem namespace may be omitted whileuser enumeration survives; actualfilesystem/syscallfailures
are propagated. Temporarysnapshots freed on one commonforwardcleanup, borrowedfds neverclosed.

[Native selected libraries](native-compat-build.txt), actualFreeBSD15.1-p4/baseClang19.1.7/gmake4.4.1,
`gmake -j16 -f userland/desktop/keiland-freebsd.mk libraries KEILAND_FREEBSD_BUILD=build/native-ui CC=cc`
warning0, realnativeadapter/archive/PDFdependency rebuild. Private nativeheaders ordered before
objects/headeraudits and excluded from install-publicheaders. Digestimplementations/licenseunchanged.

[Actual UFS/PTY contract probes](native-compat-test.txt), linked actualnativearchive/base libutil,
`cc -O2 -g -std=gnu17 -Wall -Wextra -Werror -Ibuild/native-ui/include`. RealownedUFSfiles/destination/
symlink: actualapplication tagvalue and independentextattr_* storagequery agree; rootsetnative
systemnamespace+binaryuser values, translatedsize/list, shortvalue/listERANGE leavescallerbytesunchanged,
size-onlyread/emptyvalue-vs-missingENOATTR, nonzero flags/namesunsupported, EBADFfd/list, fd-based
copy bothnamespaces includingembeddedNUL, separatelinkvalue/nativeget_link-vs-followedtarget,
linkenumeration onlyownnames, removal/nativeabsence. Independent real unprivilegednogroups/gid/uid65534
systemenumerationEPERM while accessibleuserlist succeeds; ownfiles cleaned. Actualbase forkpty creates
childsession/controllingterminal (tcgetsid==getsid), canonicalinput and childreply through realkernel
PTY, finitepoll/ownedchildwait/masterclose; native -lutil is real provider. No fakebackend.

Full Cmanualreview of nativeadapter/probes/header declarations covers static/publicorder/nativecall
error/flag/name/byteextent/fd/allocation/cleanup lifetime, semantic paragraphs and successpaths.
clang-format19.1.7 applied preserving required argument/comments/compounddecision layout; stylecheck
nativeadapter6/test17 goto candidates are all singleforwardexits to one sharedcleanup, explicitly
allowed by fullC standard (not an exception). Other rules0; PTYprobe0. Nativeprobes repeated after
final formatting. Pythonbytecompile/diffcheckPASS. Kernel/list concurrentmutation guard is code-reviewed,
not a claimed race stress test. Native filesystempermissions verified only ownedUFSfixture, not
arbitrarymountedfilesystems. Appfullbuild/GUI/filesoperation finalintegration and fullWS/threeOS/
physical gates retained. No make check/push/remotepublication/hostmutation/sharedcleanup/toolchainchange.
Local records/outbox pending publication.
