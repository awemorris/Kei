# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
from pathlib import Path
import os,sys,time,subprocess,select
endpoint=Path('/var/run/seatd.sock')
assert not endpoint.exists(), 'existing seat endpoint is not owned'
mode=Path('/dev/input/event1').stat().st_mode
r,w=os.pipe();daemon=None;identity=None
try:
 env=os.environ.copy();env['SEATD_VTBOUND']='0'
 daemon=subprocess.Popen(['/usr/local/bin/seatd','-u','nobody','-n',str(w),'-l','error'],pass_fds=(w,),env=env,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
 os.close(w);w=-1
 assert select.select([r],[],[],5)[0], 'daemon readiness timeout'
 assert os.read(r,1)==b'\n', 'invalid readiness notification'
 identity=endpoint.stat().st_ino
 def unprivileged():
  os.setgroups([]);os.setgid(65534);os.setuid(65534)
 env['LIBSEAT_BACKEND']='seatd';env['LD_LIBRARY_PATH']='/tmp/ws109-seat-lib'
 probe=subprocess.run(['/tmp/ws109-seat-client'],preexec_fn=unprivileged,env=env,cwd='/tmp',stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=10)
 print('configuration SEATD_VTBOUND=0 LIBSEAT_BACKEND=seatd',flush=True)
 print(probe.stdout,flush=True)
 assert probe.returncode==0, f'probe status={probe.returncode}'
finally:
 os.close(r)
 if w>=0:os.close(w)
 if daemon:
  daemon.terminate()
  try:out=daemon.communicate(timeout=5)[1]
  except subprocess.TimeoutExpired:daemon.kill();out=daemon.communicate(timeout=5)[1]
  print('owned daemon exit',daemon.returncode,'diagnostics',out,flush=True)
 if endpoint.exists():
  assert identity==endpoint.stat().st_ino, 'foreign endpoint must be preserved'
  endpoint.unlink()
 assert Path('/dev/input/event1').stat().st_mode==mode
 print('owned socket cleaned; native input permissions unchanged',flush=True)
