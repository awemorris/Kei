# Keiland console launcher check

Run on Linux or native FreeBSD with Python 3:

```sh
python3 plan/tools/keiland-launcher/check.py userland/desktop/wayland/keiland-desktop.in
```

Uses only owned temporary paths and a fake executable at a generated custom prefix. Checks actual sh behavior for console runtime permissions, existing session environment, quoted argv/prefix paths, display/socket, rejection before seat startup, exec PID/signal/exit status. Does not acquire GPU/VT or test authentication/GDM login. Source must be the current launcher template; no production test switch is used.
