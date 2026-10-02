# WS114 q580 baseline reproduction / handoff

Saved owned runtime, stopped:

- P9 worktree `/home/awe/zedBSD-worktrees/p9`, source branch `codex/p9`.
- Image `/home/awe/zedBSD-worktrees/p9/build/p9-q580/resume/gtk-baseline.qcow2` (SHA256 in evidence/q580/resume-image.log); backing raw image `/home/awe/zedBSD-claude1/build/keiland-linux/guest/guest.img`, read only.
- Kernel/initrd/key assets `/home/awe/zedBSD-worktrees/p9/build/p9-q580/guest/`. Keys are local ignored files, never committed/shared through Git. Preserve the entire ignored runtime for authorized B handoff.
- Old guest-control at `build/p9-q580/guest-control`; it uses root SSH127.0.0.1:2249 and old run directory. Its **start** makes a fresh raw-base overlay, losing saved package additions. Do not use start to resume the saved snapshot.
- Prior QEMU251216 and guest compositor2793/probe5950/demo6106 are stopped. No owner is running. Evidence/stop.log verifies PID/port absence.

Only after main assigns the B runtime and remaining attempt, create another disposable overlay from the saved snapshot and launch manually with the same devices. For example from the assigned checkout:

```sh
q580_assets=/home/awe/zedBSD-worktrees/p9/build/p9-q580/guest
q580_saved=/home/awe/zedBSD-worktrees/p9/build/p9-q580/resume/gtk-baseline.qcow2
q580_run="$PWD/build/ws114-resume"
mkdir -p "$q580_run"
timeout 20 qemu-img create -f qcow2 -F qcow2 -b "$q580_saved" "$q580_run/overlay.qcow2"
timeout 30 qemu-system-x86_64 -machine q35,vmport=off -accel kvm -cpu host -m 4G -smp 4 -display none \
 -kernel "$q580_assets/vmlinuz" -initrd "$q580_assets/initrd.img" -append 'root=/dev/vda rw console=ttyS0 quiet' \
 -drive "file=$q580_run/overlay.qcow2,format=qcow2,if=virtio" -device virtio-vga,id=video0 \
 -device virtio-keyboard-pci,display=video0 -device virtio-tablet-pci,display=video0 \
 -audiodev none,id=snd0 -device intel-hda -device hda-duplex,audiodev=snd0 \
 -netdev user,id=n0,hostfwd=tcp:127.0.0.1:2249-:22 -device virtio-net-pci,netdev=n0 \
 -qmp "unix:$q580_run/qmp.sock,server=on,wait=off" -pidfile "$q580_run/qemu.pid" -daemonize
export GUEST_DIR="$q580_assets" GUEST_RUN="$q580_run" SSH_PORT=2249
# Poll SSH with a bounded deadline; then inspect boot PNG. Do not read serial/console logs.
timeout 15 sh plan/tools/keiland-linux/guest.sh ssh 'cat /etc/os-release; dpkg --audit'
timeout 15 sh plan/tools/keiland-linux/guest.sh screenshot "$q580_run/boot.png"
timeout 15 sh plan/tools/keiland-linux/guest.sh put plan/ws114/tests/start-session.sh /tmp/q580-start-session.sh
timeout 15 sh plan/tools/keiland-linux/guest.sh put plan/ws114/tests/gtk4-baseline.py /tmp/gtk4-baseline.py
timeout 30 sh plan/tools/keiland-linux/guest.sh ssh 'sh /tmp/q580-start-session.sh'
```

The saved image already has the frozen Keiland deb, standard gtk-4-examples/libgtk-4-bin/libgtk-4-dev/python3-gi/gir1.2-gtk-4.0/dbus-x11/wayland-utils and later gsettings-desktop-schemas. Version receipts are evidence/environment.log and final-state.log. start-session.sh creates a fresh root session bus, XDG_RUNTIME_DIR=/run/p9-gtk, WAYLAND_DISPLAY=wayland-q580, GDK_BACKEND=wayland, XDG_SESSION_TYPE=wayland, XDG_CURRENT_DESKTOP=Keiland. Root/direct is the measured baseline, not a user GDM/session certification.

Recorded guest invocations (with `. /tmp/q580-session.env` before each):

```sh
nohup env WAYLAND_DEBUG=client GDK_DEBUG=opengl GSK_DEBUG=renderer gtk4-demo --run=clipboard > /tmp/q580-demo.stdout 2> /tmp/q580-demo.stderr < /dev/null &
nohup env WAYLAND_DEBUG=client python3 /tmp/gtk4-baseline.py > /tmp/q580-probe.stdout 2> /tmp/q580-probe.stderr < /dev/null &
nohup timeout 240 dbus-monitor --address "$DBUS_SESSION_BUS_ADDRESS" > /tmp/q580-dbus.log 2>&1 < /dev/null &
```

QMP actual actions at initial1280×800 image, with settling time before screenshots:

1. Probe source click460,321 → Ctrl+A → Ctrl+C; target click480,369 → Ctrl+V. Same-client text result and source.send observed; no cross-client claim.
2. Menu click333,232; after popup visible choose Second action325,311.
3. Dialog click350,419; close640,476. transient parent/modal API and Wayland set_parent observed.
4. After standard schema installation, FileDialog click450,420; create `/tmp/q580-marker.txt`; Ctrl+L → type path → Enter. Actual callback returned that path.
5. Maximize click563,422, then automatic restore after4s. Fullscreen click647,422, then automatic restore after4s. Screenshot after1s.
6. Pointer drag999,682→1160,748 was an ineffective resize attempt: text selected, no xdg_toplevel.resize request. Continue only after selecting correct edges in new PNG; do not reuse coordinates blindly.
7. Second gtk4-demo clipboard launch reached setup only; interrupted before cross-client paste. No Vulkan/Cairo forced renderer or wheel/touch/IME/scale-change claim.

Original FileDialog crash before schema installation and no-portal/a11y errors are retained. Renderer is GskGLRenderer/EGL3.2/llvmpipe with wl_shm. Do not substitute a new package/current source unnoticed. Safe shutdown: close owned apps, terminate owned compositor, collect its exit receipt/console PNG, preserve or clone the owned overlay before existing guest.sh stop deletes its configured `overlay.qcow2`. Verify QEMU process and listener absence. Do not stop unrelated guests.

## q581 remaining measurements / 2026-10-02

B1 isolated runtime `/home/awe/zedBSD-worktrees/b1/build/b1-q581`, frozen q580 snapshot backing and same SSH2249/devices. QEMU283995 is normally stopped and its overlay preserved; do not start another guest before main's resource allocation. Use the earlier manual launch procedure with the new run path/overlay, then bounded SSH polling up to180s. Ctrl+Alt+F1 and QMP PNG established boot before SSH became available. First immediate wayland-info raced startup; wait for socket readiness for that utility. Actual GTK registry trace provided the complete globals.

The owned Python probe gained PID/scroll/focus instrumentation, tooltip, configurable application IDs/title/text for independent clients. These controls belong only to this test helper. `q581-control.py` preserves the actual low-level SSH/QMP action methods and records UTC JSONL. Import it from a Python script to call `ssh`, `shot`, `click`, `key`, `drag` after owner/resource checks. Its default paths deliberately match this saved runtime; override/inspect paths before a different assignment.

The exact run is `evidence/q581/actions.jsonl`, with all app/Wayland/D-Bus logs and named images. Confirm coordinates from a fresh1280×800 screenshot. Successful move680,232→780,282. Restarted successful resize997,680→1097,740 (inner988,668 was ineffective). Restarted fresh client wheel-down5 at500,540, tooltip hover334,232. Cross-client copy A465,326 Ctrl+A/C; distinct application Btarget500,423 Ctrl+V. `GSK_RENDERER=cairo` and `vulkan` each mapped/input-tested once with standard distro stack, CPU llvmpipe/wl_shm.

Prior move/resize grabs may leave GTK pointer state active; fresh clients were used for independent observations. This is retained as a baseline finding, not a repair. Logs and attempted-resize images stay classified in `phase001/q581-result.md`. To resume a future source-verification guest, retain frozen baseline as comparison and install only the explicitly authorized package/source; never replace the baseline provenance silently.
