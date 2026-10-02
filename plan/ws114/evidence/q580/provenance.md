# q580 baseline provenance and isolation

P9 generation2 / q580-i01; worktree base `25729c88a`; began 2026-10-02 06:30 UTC, max3h.

Runtime: WS105 raw Debian13 base `/home/awe/zedBSD-claude1/build/keiland-linux/guest/guest.img` read only as backing; P9 overlay `build/p9-q580/run/overlay.qcow2`, SSH `127.0.0.1:2249`, QMP `build/p9-q580/run/qmp.sock`. Key copy and kernel/initrd links are ignored local runtime assets; no credentials are committed. Boot SSH identity and QMP PNG both verified; serial/console logs are not used for acceptance.

Compositor: WS108 verified distro deb `keiland_0~git20261001.b0e1eaf972e1-1+debian13_amd64.deb`, source `b0e1eaf972e1`. Frozen package is installed in the new overlay. Later HEAD has FreeBSD/shared-wire/backend relocations and launcher changes; this baseline is not asserted to be identical to current HEAD. Protocol source table remains source evidence; runtime evidence identifies this exact package. No compositor/GTK source edits or build, shared stage/toolchain writes, other guest stop or host install.

GTK4 and diagnostic dependencies: Debian13 standard apt packages, install command/result in `package-install.log`; exact installed versions follow after installation. Dedicated apt bootstrap introduces session dependencies only inside disposable overlay. No new portal backend is selected or installed intentionally.

Primary references checked on 2026-10-02:

- [GTK Wayland backend](https://docs.gtk.org/gtk4/wayland.html): backend/socket/runtime selection.
- [GTK runtime diagnostics](https://docs.gtk.org/gtk4/running.html): GDK/GSK diagnostics and renderer selectors. Current online docs are library4.23.4; actual distro diagnostic help is recorded separately rather than assuming all online options exist.
- [Gtk.FileChooserNative](https://docs.gtk.org/gtk4/class.FileChooserNative.html): ordinary dialog and portal integration paths.
- [Gtk.FileDialog](https://docs.gtk.org/gtk4/class.FileDialog.html): asynchronous dialog API.
- [Portal system integration](https://flatpak.github.io/xdg-desktop-portal/docs/system-integration.html): D-Bus session/backend selection, distinct from compositor protocols.
- [Portal FileChooser](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.FileChooser.html) and [Settings](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Settings.html): interface contracts.
- [Upstream xdg-shell protocol XML](https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/main/stable/xdg-shell/xdg-shell.xml): web access was denied by Anubis. The packaged guest XML at `/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml` was read instead; its SHA256 is in `environment.log`. The XML is diagnostic reference and is not copied into the repository.

All measured/failed/skipped outcomes will remain distinct. User adoption decisions remain pending p002.

Initial apt SSH wrapper exceeded210s and returned124; guest dpkg continued and finished. Subsequent `dpkg --audit` was empty, installed package versions were queried, and `gtk-4-examples` and `gsettings-desktop-schemas` installation commands returned0. The first FileDialog attempt terminated for absent GSettings schemas; original trace retained. After the standard schema package, the ordinary chooser selected `/tmp/q580-marker.txt` successfully without a portal frontend/backend.
