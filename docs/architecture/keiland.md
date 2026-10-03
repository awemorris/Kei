# Keiland desktop environment

Status: design (2026-10-03). This is the target design: the document comes
first and the implementation follows it, so some of what it describes is not
built yet.

This document describes Keiland from the outside: what it is for, how it is
divided, which interfaces are stable, and what a user and an application see.
It does not describe internal implementation.

## Purpose

Keiland exists to build a new kind of desktop: one that joins a touch UI/UX
and the conventional desktop UI/UX into one environment. Commercial desktops
such as macOS and Windows have not achieved this. They either keep a
mouse-and-keyboard desktop with touch added on top, or switch to a separate
tablet mode. Keiland treats a finger, a pen, a mouse and a keyboard as equal
ways to use the same windows, menus and system controls.

Two design choices follow from this aim and run through the rest of the
document:

- **One desktop, not a set of cooperating programs.** Parts that other
  environments run as separate processes, such as the on-screen keyboard, are
  part of the compositor. The desktop is meant to feel like one thing.
- **Portability is a design goal, not an afterthought.** Keiland runs as the
  native desktop of zedBSD and also on Linux and FreeBSD. Everything that
  differs between operating systems is kept behind defined boundaries.

## Layers

| Layer | What it is | Who uses it | Stability |
| --- | --- | --- | --- |
| Applications | Programs with windows: Files, Settings, Terminal, Notes, the browser and others, plus X11 programs through the X server | The user | — |
| [libkeiland](../../userland/desktop/keiland/keiland.h) | The public C library of the desktop | Applications | **Stable public API** (`KEILAND_VERSION`) |
| Standard Wayland and Vulkan | Core Wayland, `xdg-shell` and other standard protocols; Vulkan for drawing | Applications | Upstream standards |
| Keiland's own Wayland extensions | `keiland_*_v1` protocols spoken between libkeiland and the compositor | libkeiland only | **Internal**; may change at any time |
| Keiland compositor | The Wayland compositor: windows, input, system bar, on-screen keyboard, input method host | — | — |
| [libkeiland-backend](../../userland/desktop/libkeiland-backend/keiland-backend.h) | Everything in the compositor that differs between operating systems | The compositor only | Internal, compile-time interface |
| Operating system | zedBSD, Linux or FreeBSD: kernel, GPU driver, network and sound services | The backend | — |

## Wayland compositor and protocols

Keiland is a Wayland compositor. It speaks the standard protocols an ordinary
Wayland client expects: core Wayland (`wl_compositor`, `wl_shm`, `wl_seat`,
`wl_data_device_manager`, subsurfaces), `xdg-shell` with popups and
positioners, server-side decoration (`zxdg_decoration_manager_v1`), cursor
shapes, primary selection, the tablet protocol (`zwp_tablet_manager_v2`),
`zwp_text_input_v3`, and the input method and virtual keyboard protocols for
the system's input method. An unmodified GTK, Qt or SDL client runs as a
normal Wayland window.

The parts of the UI/UX that only Keiland has are carried by its own
extensions, for example:

| Extension | What it carries |
| --- | --- |
| `keiland_titlebar_manager_v1` | The window's titlebar presentation: menu, controls (navigation, breadcrumb, search, view selector) or tabs |
| `xdg_menu_manager_v1` | The application's menus as a tree of items, drawn by the compositor in the titlebar or the system bar |
| `keiland_glass_manager_v1` | Which parts of a surface stand on the desktop's frosted glass, and whether the glass blurs what is under it |
| `keiland_desktop_manager_v1` | The desktop surface (the icons of `~/Desktop`) between the wallpaper and the windows |
| `keiland_edit_manager_v1` | Editing operations (copy, paste, select all) that the system UI can ask a window to perform |
| `keiland_keyboard_inset_manager_v1` | How much of a window the on-screen keyboard covers, so the window can keep the caret in view |
| `keiland_ime_status_manager_v1` | The input method's state (language, mode) for the system bar's indicator |

**These extensions are not an application interface.** An application never
binds them itself. libkeiland wraps every one of them, and the protocols are
an internal implementation detail between libkeiland and the compositor: a
protocol can be renamed, versioned or replaced in any release, as long as the
libkeiland API it serves keeps its meaning.

## libkeiland: the stable public API

[libkeiland](../../userland/desktop/keiland/keiland.h) is the one interface an
application uses to reach anything Keiland offers beyond standard Wayland and
Vulkan. Its API is public and stable. `KEILAND_VERSION` (21 at the time of
writing) increases when calls are added, and `keiland_version()` reports the
version of the library actually loaded, so a program can detect an older
library.

The API covers the UI and also settings of the whole system. Its families
include:

| Family | Examples |
| --- | --- |
| Window UI | System Menu (`keiland_menu_*`), titlebar presentation (`keiland_titlebar_*`, including sheets), context menus, glass panels, the desktop surface, window operations |
| Touch | The touch motion, the scroller and the gestures (`keiland_motion_*`, `keiland_scroller_*`, `keiland_gesture_*`), so that a finger feels the same in every program |
| Input | The keyboard inset of the on-screen keyboard, editing operations |
| System | The network, including Wi-Fi control, saved keys, links and DNS (`keiland_network_*`); the sound output's volume and mute (`keiland_audio_*`); the desktop's preferences; recent files |

A call is handled in one of two places, and the application does not need to
know which:

- **In libkeiland itself**, for example the touch motion and scroller
  calculations, or reading the desktop's preferences.
- **Bridged to the compositor** through the internal extensions, for example
  a menu the compositor draws, or a system setting the compositor owns.

How a system call is carried out differs per operating system (for example
how Wi-Fi is controlled on zedBSD, Linux and FreeBSD). That difference stays
below libkeiland; the call's meaning does not change.

## libkeiland-backend: the operating-system boundary

Existing Wayland compositors are tightly bound to Linux interfaces (DRM/KMS,
dma-buf, logind, udev). Porting one to another POSIX system usually means
forking it. Keiland instead puts every operating-system dependency of the
compositor into a backend, `libkeiland-backend`, with one interface
([keiland-backend.h](../../userland/desktop/libkeiland-backend/keiland-backend.h))
and one implementation per system:

- `libkeiland-backend-zedbsd`
- `libkeiland-backend-linux`
- `libkeiland-backend-freebsd`

The compositor calls only this interface; an area a system does not offer
answers `ENOTSUP`. Applications never see the backend.

GPU buffers show why the split matters. On Linux, buffers are shared through
dma-buf. On zedBSD, the GPU driver offers a much simpler buffer model, and the
compositor uses that directly. With both behind the backend, the same
compositor is a complete, maintainable desktop on Linux and on a non-Linux
POSIX system such as FreeBSD, not a Linux program with a compatibility layer.

The backend covers every area where the systems differ: the network, sound,
power, the seat and session, input devices, the display and the GPU buffers.

## Touch input, on-screen keyboard and input method

The on-screen keyboard is drawn and controlled by the compositor itself. A
swipe from a bottom corner opens it: the QWERTY panel along the bottom from
the bottom-left corner, the flick panel at the side from the bottom-right
corner. Desktops such as GNOME run the on-screen keyboard as a separate
process. Keiland deliberately does not: it chooses a desktop that feels like
one piece over process separation, so that the keyboard, the windows and the
gestures share one notion of where a finger is.

The input method is one program (`keiland-ime`) that the compositor starts
and supervises. It speaks the standard `zwp_input_method_v2` and virtual
keyboard protocols, and applications receive text through the standard
`zwp_text_input_v3`. Standard Wayland input methods are built around a
hardware keyboard. They make touch input hard to do well, in particular
predictive conversion while typing on the on-screen keyboard. Keiland
therefore extends its input method beyond the Wayland standard. The input
method talks to the compositor's on-screen keyboard through Keiland's own
extensions, so prediction and the keyboard work together. These extensions
are internal, like the others.

## Titlebar and system bar

The titlebar is Keiland's most visible feature.

- **Server-side decoration.** The compositor draws every window's titlebar.
  A client that asks for client-side decoration is honored, and a full-screen
  window has no titlebar.
- **One integrated bar.** The title, the application's menus, a search field
  and the application's buttons share the titlebar. The application gives
  their meaning through libkeiland. The compositor decides the layout, so
  menus and controls look and behave the same in every program. When room
  runs short, controls fold into a "..." popup in a defined order.
- **Floating and touchable.** The titlebar floats over the window as a
  rounded glass card, sized for a finger as well as a pointer.
- **Docking into the system bar.** When a window is maximized, its titlebar
  merges into the system bar at the top of the screen. The window's menus
  and controls appear in the system bar's application zone, so a small
  screen keeps its full height for the content.

The system bar also holds the system's own controls (network, sound, battery,
input method, virtual desktops). They are driven by the same compositor and
reached by applications such as Settings through libkeiland.

## Related documents

- [Kernel, HAL and driver boundaries](kernel-and-hal.md)
- [Keiland on Linux](../../userland/desktop/LINUX.md) and
  [on FreeBSD](../../userland/desktop/README.freebsd.md): build and launch
