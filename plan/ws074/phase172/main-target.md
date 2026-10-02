# q579 main target/guest evidence（進行中）

Target/image source: main72dabcba303d5debee29efcef3a08118115f082d（import95835ef85を含む）。
専用config: build/main-n3-browser-guest/config.mk = criteria.mk + libbrowser/browser。
make -j16 ZEDBSD_CONFIG=上記 BUILD=build/main-n3-browser-guest/image、guestextrafiles、既存canonicalNoct/hostartifact/stampを-oで維持しdisk-image。

Build exit0、projectwarning0（1976lines）、check-amd64-native-image OK。toolchain変更なし、共有packagebuild他agentと並列無し。Noct同一canonical artifact保持、Noct自体の再build/受け入れは対象外。
Target libbrowser DT_NEEDED=libvulkan/libtruetype/libjpeg-compat/libpng-compat/libz-compat/libgif-compat/libc、Wayland無し。SONAME libbrowser.so、definedglobalfunctionexports39。

boot-test（fresh Venus config、GPU無しframebuffer）exit0/login prompt PNG。main目視済み、[PNG](main-evidence/boot-login.png)。passthrough demo補助bootのQMPtimeout2回はp014/main-image-preflightに別記し混同しない。
Venus専用runtime build/main-n3-browser-guest/runtime、zdesktop-guest.sh start/wait240でguest:ready。loopbackSSHのみ。
first.html/blocks.htmlをguestの/usr/share/browser-testsへput、service stop greeter後に既存browser-p014.shでshell/GPUCPU比較/scroll/titlebar/closeを実行中。console/seriallogは判定に使わない。

p172全Phaseは全文manual/最終source検証と全disposition照合までin-progress。target/bootだけでclearanceしない。

Native guest browser-p014 exit0/status0: READY/frame、first/blocks各GPUCPU agree、End224/wheel179/Home0、CtrlQ、titlebar移動+100,+60/close、compositorERROR0。mainがfirst/moved画像を目視。独立guestをstopし所有資源のみ終了。[testlog](main-evidence/native-p014.log)、[app/compositor対象log](main-evidence/native-app.log)、[first PNG](main-evidence/native-first.png)。
