# Architecture

Architecture documents give the external design of each subsystem: its
purpose, its parts and their boundaries, and the reasons for them. They state
the target design; the implementation follows ([rules](../style.md)).

## Current architecture

- [Kernel, HAL and driver boundaries](kernel-and-hal.md): ownership, storage,
  public interfaces and implementation limits.
- [Keiland desktop environment](keiland.md) (target design): purpose, layers,
  the stable libkeiland API, internal Wayland extensions, the
  operating-system backend, touch input and the titlebar.

## Planned architecture

- [μITRON-compatible real-time domain](muitron-rt-domain.md): confirmed
  user-mode resident-ELF and explicit-MMIO direction, compatibility rationale,
  POSIX service bridge, and the unresolved decisions that currently block
  WS015.
