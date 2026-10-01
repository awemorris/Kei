# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Own a native guest seat daemon and restore all captured console properties."""
from pathlib import Path
import fcntl,json,os,select,struct,subprocess,termios,time
endpoint=Path('/var/run/seatd.sock')
assert not endpoint.exists(), 'existing endpoint is not owned'
abi=json.loads(subprocess.check_output(['/tmp/ws109-console-abi'],text=True))
console=os.open('/dev/ttyv0',os.O_RDWR|os.O_NOCTTY)
def inquiry(request,size=4):return fcntl.ioctl(console,abi[request],bytes(size))
original={'active':struct.unpack('i',inquiry('get_active'))[0],
          'vt':inquiry('get_vt_mode',abi['vt_size']),
          'drawing':struct.unpack('i',inquiry('get_drawing'))[0],
          'keyboard':struct.unpack('i',inquiry('get_keyboard'))[0],
          'termios':termios.tcgetattr(console)}
assert original['active']==1, 'fixture requires its prepared VT1'
modes=[Path('/dev/input/event'+str(i)).stat().st_mode for i in range(6)]
def unprivileged():
 os.setgroups([]);os.setgid(65534);os.setuid(65534)
def line(process,wanted):
 assert select.select([process.stdout],[],[],7)[0], 'client notification timeout: '+wanted
 got=process.stdout.readline().strip();print('client:',got,flush=True)
 if not got:
  print('client failure status:',process.wait(timeout=2),'stderr:',process.stderr.read(),flush=True)
 assert got==wanted, (got,wanted)
def scenario(mode):
 daemon=None;probe=None;identity=None;r,w=os.pipe()
 try:
  env=os.environ.copy();env['LD_LIBRARY_PATH']='/tmp/ws109-seat-lib'
  env.pop('SEATD_VTBOUND',None)
  if mode=='refusal':env['SEATD_VTBOUND']='0'
  daemon=subprocess.Popen(['/usr/local/bin/seatd','-u','nobody','-n',str(w),'-l','error'],pass_fds=(w,),env=env,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  os.close(w);w=-1
  assert select.select([r],[],[],5)[0];assert os.read(r,1)==b'\n'
  identity=endpoint.stat().st_ino
  probe=subprocess.Popen(['/tmp/ws109-seat-backend',mode],preexec_fn=unprivileged,env=env,cwd='/tmp',stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,bufsize=1)
  if mode=='lifetime':
   line(probe,'READY_PAUSE')
   fcntl.ioctl(console,abi['activate'],2)
   line(probe,'PAUSED_RETIRED')
   fcntl.ioctl(console,abi['activate'],1)
   line(probe,'READY_DISCONNECT')
   daemon.terminate();daemon.wait(timeout=5)
   probe.stdin.write('d');probe.stdin.flush()
  out,err=probe.communicate(timeout=10);print(out,err,flush=True)
  assert probe.returncode==0, f'{mode} status={probe.returncode}'
 finally:
  os.close(r)
  if w>=0:os.close(w)
  if probe and probe.poll() is None:
   probe.terminate()
   try:probe.wait(timeout=3)
   except subprocess.TimeoutExpired:probe.kill();probe.wait(timeout=3)
  if daemon:
   if daemon.poll() is None:daemon.terminate()
   try:out=daemon.communicate(timeout=5)[1]
   except subprocess.TimeoutExpired:daemon.kill();out=daemon.communicate(timeout=5)[1]
   print('daemon exit',daemon.returncode,'diagnostics:',out,flush=True)
  if endpoint.exists():
   assert endpoint.stat().st_ino==identity, 'foreign endpoint preserved'
   endpoint.unlink()
try:
 scenario('refusal');scenario('lifetime')
finally:
 # The external daemon may choose text/keyboard defaults; restore the independently captured native state.
 fcntl.ioctl(console,abi['activate'],original['active'])
 fcntl.ioctl(console,abi['set_vt_mode'],original['vt'])
 fcntl.ioctl(console,abi['set_keyboard'],original['keyboard'])
 fcntl.ioctl(console,abi['set_drawing'],original['drawing'])
 termios.tcsetattr(console,termios.TCSAFLUSH,original['termios'])
 print('VT before',original['vt'].hex(),'after',inquiry('get_vt_mode',abi['vt_size']).hex(),flush=True)
 assert inquiry('get_vt_mode',abi['vt_size'])[0:2]==original['vt'][0:2]
 assert struct.unpack('i',inquiry('get_active'))[0]==original['active']
 assert struct.unpack('i',inquiry('get_keyboard'))[0]==original['keyboard']
 assert struct.unpack('i',inquiry('get_drawing'))[0]==original['drawing']
 assert termios.tcgetattr(console)==original['termios']
 assert modes==[Path('/dev/input/event'+str(i)).stat().st_mode for i in range(6)]
 os.close(console)
 print('PASS original native VT/mode/keyboard/drawing/termios/input permissions restored',flush=True)
