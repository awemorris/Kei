#!/bin/bash
set -euo pipefail
export GUEST_DIR="$PWD/build/ws105-p011/guest" GUEST_RUN="$PWD/build/ws105-p011/run" SSH_PORT=2228
G=plan/tools/keiland-linux/guest.sh
O=$PWD/build/ws105-p011
sh "$G" click 23 17
sh "$G" ssh 'sleep .5'
sh "$G" click 712 276
sh "$G" ssh 'sleep 2; ps -o user,pid,args -C terminal -C wayland' > "$O/gdm-terminal.log"
sh "$G" click 500 430
sh "$G" type 'echo keiland-user-ok'
sh "$G" key ret
sh "$G" ssh 'sleep 1'
sh "$G" screenshot "$O/gdm-terminal.png"
sh "$G" ssh 'pgrep -u kei -x wayland' > "$O/gdm-pid-before.log"
for method in switch chvt; do
 if test "$method" = switch; then
  sh "$G" ssh 'busctl call org.freedesktop.login1 /org/freedesktop/login1/seat/seat0 org.freedesktop.login1.Seat SwitchTo u 3; sleep 2; ps -o user,pid,args -C wayland' > "$O/$method-away.log"
 else
  sh "$G" ssh 'chvt 3; sleep 2; ps -o user,pid,args -C wayland' > "$O/$method-away.log"
 fi
 sh "$G" screenshot "$O/$method-away.png"
 if test "$method" = switch; then
  sh "$G" ssh 'busctl call org.freedesktop.login1 /org/freedesktop/login1/seat/seat0 org.freedesktop.login1.Seat SwitchTo u 2; sleep 2; ps -o user,pid,args -C wayland' > "$O/$method-back.log"
 else
  sh "$G" ssh 'chvt 2; sleep 2; ps -o user,pid,args -C wayland' > "$O/$method-back.log"
 fi
 sh "$G" click 550 450
 sh "$G" type "echo $method-ok"
 sh "$G" key ret
 sh "$G" ssh 'sleep .5'
 sh "$G" screenshot "$O/$method-back.png"
 sh "$G" ssh 'journalctl -b --no-pager -t /usr/libexec/gdm-wayland-session -n 120' > "$O/$method-events.log"
done
sh "$G" ssh 'pgrep -u kei -x wayland' > "$O/gdm-pid-after.log"
python3 - "$O" <<'PY'
from pathlib import Path
import re,sys
r=Path(sys.argv[1]);assert (r/'gdm-pid-before.log').read_text()==(r/'gdm-pid-after.log').read_text()
s=(r/'chvt-events.log').read_text();pause=re.findall(r'PauseDevice.*',s);resume=re.findall(r'ResumeDevice.*',s)
print('VT pause',len(pause),'resume',len(resume))
assert len(pause)>=10 and len(resume)>=10,(pause,resume)
print('gdm-VT: PASS same compositor PID / SwitchTo+chvt / 5 leases x2; input screenshots retained')
PY
