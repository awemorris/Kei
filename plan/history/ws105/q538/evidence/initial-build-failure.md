# Initial clean build failure (q538)

Before the object-variable namespace fix, the first gcc clean build exited2. The retained tool observation included:

    /usr/bin/ld: cannot find build/keiland-linux/obj/userland/desktop/wayland/linux/os-linux.o: No such file or directory
    /usr/bin/ld: cannot find build/keiland-linux/obj/userland/desktop/wayland/linux/dbus-linux.o: No such file or directory
    /usr/bin/ld: cannot find build/keiland-linux/obj/userland/desktop/wayland/linux/gpu-linux.o: No such file or directory
    make[1]: *** [userland/desktop/libwayland/Makefile.linux:24: build/keiland-linux/lib/libwayland-client.so] Error 1

The first complete log was overwritten by the subsequent build, so this is a retained excerpt from the tool observation, not the original full log. Cause: library and program both used KEILAND_LINUX_OBJS_wayland, with lazy recipe expansion taking the program list. Fixed by library/program/static namespaces. The subsequent complete logs are preserved under first-pass, and the final pass is rebuilt after the standards edits.

The first host helper ended at a fixture compile with duplicate _GNU_SOURCE (-D plus vk-chain-test.c's own define), after host WSI / D-Bus tests passed. The helper flag was removed for this fixture; no production macro change was needed.
