#!/usr/bin/env python3
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Recorded q587 control operations; use only in the assigned owned runtime."""
import os,sys,json,subprocess,time,importlib.util
from pathlib import Path
root=Path(__file__).resolve().parents[3]
os.environ.update(GUEST_DIR='/home/awe/zedBSD-worktrees/p9/build/p9-q580/guest',GUEST_RUN=str(root/'build/b1-q587/guest'),SSH_PORT='2249')
spec=importlib.util.spec_from_file_location('guest',root/'plan/tools/keiland-linux/guest.py');g=importlib.util.module_from_spec(spec);spec.loader.exec_module(g)
evidence=root/'plan/ws114/evidence/q587'
def record(action,**args):
 with (evidence/'actions.jsonl').open('a') as f:f.write(json.dumps({'utc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'action':action,**args})+'\n')
def ssh(cmd,name=None):
 record('ssh',command=cmd,evidence=name)
 p=g.ssh([cmd],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 if name:(evidence/name).write_text(p.stdout)
 return p.stdout

def shot(name):
 record('screenshot',name=name);g.screenshot(evidence/name)
def point(x,y):
 record('point',x=x,y=y);g.input_events([{'type':'abs','data':{'axis':'x','value':x*32767//1279}},{'type':'abs','data':{'axis':'y','value':y*32767//799}}])
def button(down,kind='left'):
 record('button',down=down,kind=kind);g.input_events([{'type':'btn','data':{'down':down,'button':kind}}])
def click(x,y):
 point(x,y);button(True);button(False)
def key(*keys):
 record('key',keys=keys);g.key(keys)
def drag(x,y,tx,ty):
 point(x,y);button(True);time.sleep(.2)
 for i in range(1,11):point(x+(tx-x)*i//10,y+(ty-y)*i//10);time.sleep(.05)
 button(False);time.sleep(.5)
if __name__=='__main__':
 deadline=time.monotonic()+60
 while not g.ready():
  if time.monotonic()>deadline:raise RuntimeError('SSH timeout')
  time.sleep(1)
 print(ssh('cat /etc/os-release; uname -r; dpkg --audit; dpkg-query -W libgtk-4-1 gtk-4-examples gsettings-desktop-schemas; sha256sum /opt/keiland/bin/wayland; ldd /opt/keiland/bin/wayland','environment.log'))
 shot('boot.png')
