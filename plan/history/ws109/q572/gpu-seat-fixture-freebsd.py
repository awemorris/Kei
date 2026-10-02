# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Exercise the installed real native compositor on the passed-through Intel GPU."""
from pathlib import Path
import os,pwd,grp,select,subprocess,tempfile,shutil,time,json,fcntl,struct,termios
stage=Path('/root/keiland-stage-final/opt/keiland');prefix=Path('/opt/keiland');endpoint=Path('/var/run/seatd.sock')
assert not prefix.exists();assert not endpoint.exists();assert Path('/dev/dri/card0').exists()
abi=json.loads(subprocess.check_output(['/tmp/ws109-console-abi'],text=True))
console=os.open('/dev/ttyv0',os.O_RDWR|os.O_NOCTTY)
def inquiry(request,size=4):return fcntl.ioctl(console,abi[request],bytes(size))
original={'active':struct.unpack('i',inquiry('get_active'))[0],'vt':inquiry('get_vt_mode',abi['vt_size']),'drawing':struct.unpack('i',inquiry('get_drawing'))[0],'keyboard':struct.unpack('i',inquiry('get_keyboard'))[0],'termios':termios.tcgetattr(console)}
assert original['active']==1
modes=[Path('/dev/input/event'+str(i)).stat().st_mode for i in range(6)]
def active(target):
 limit=time.monotonic()+5
 while time.monotonic()<limit:
  if struct.unpack('i',inquiry('get_active'))[0]==target:return
  time.sleep(.05)
 raise AssertionError('VT did not reach '+str(target))
def descriptors(label):
 report=subprocess.check_output(['/usr/bin/procstat','-f',str(server.pid)],text=True)
 print(label+' FD SNAPSHOT\n'+report,flush=True)
 return report.count('/dev/input/event')
identity=None;daemon=None;server=None;r,w=os.pipe();clients=[]
user=pwd.getpwnam('nobody');video=grp.getgrnam('video').gr_gid
try:
 shutil.copytree(stage,prefix)
 with tempfile.TemporaryDirectory(prefix='ws109-intel-',dir='/tmp') as directory:
  os.chown(directory,user.pw_uid,user.pw_gid);os.chmod(directory,0o700)
  env=dict(os.environ);env.pop('SEATD_VTBOUND',None)
  env.update(HOME=directory,XDG_CONFIG_HOME=directory,XDG_DATA_HOME=directory,XDG_RUNTIME_DIR=directory,WAYLAND_DISPLAY='wayland-keiland',KEILAND_SEAT='seatd',KEILAND_DRM_DEVICE='/dev/dri/card0',VK_DRIVER_FILES='/usr/local/share/vulkan/icd.d/intel_icd.x86_64.json')
  env.pop('LD_LIBRARY_PATH',None)
  daemon=subprocess.Popen(['/usr/local/bin/seatd','-u','nobody','-n',str(w),'-l','debug'],pass_fds=(w,),env=env,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  os.close(w);w=-1
  assert select.select([r],[],[],5)[0];assert os.read(r,1)==b'\n';identity=endpoint.stat().st_ino
  print('VT after seatd start',inquiry('get_vt_mode',abi['vt_size']).hex(),flush=True)
  def unprivileged():
   os.setgroups([video]);os.setgid(user.pw_gid);os.setuid(user.pw_uid)
  socket=Path(directory,'wayland-keiland')
  with Path('/root/ws109-passthrough-evidence/compositor.txt').open('w') as output:
   server=subprocess.Popen([str(prefix/'bin/wayland'),'--socket='+str(socket),'--timeout=30','--log-frames','--desktop-client=none'],preexec_fn=unprivileged,env=env,cwd=directory,stdout=output,stderr=subprocess.STDOUT,text=True)
   limit=time.monotonic()+8
   while not socket.exists() and server.poll() is None and time.monotonic()<limit:time.sleep(0.05)
   if socket.exists():
    args=[str(prefix/'bin/wltest'),'--frames=90','--delay-ms=200','--windowed','--size=320x240','--token=ws109-lifecycle']
    client=subprocess.Popen(args,preexec_fn=unprivileged,env=env,cwd=directory,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    try:
     time.sleep(2)
     assert client.poll() is None, 'GPU client failed before VT test'
     print('VT after GPU start',inquiry('get_vt_mode',abi['vt_size']).hex(),flush=True)
     print('SEATD FILES',subprocess.check_output(['/usr/bin/procstat','-f',str(daemon.pid)],text=True),flush=True)
     before=descriptors('ACTIVE')
     assert before>0
     fcntl.ioctl(console,abi['activate'],2);active(2);time.sleep(.5)
     limit=time.monotonic()+6
     while time.monotonic()<limit:
      report=subprocess.check_output(['/usr/bin/procstat','-f',str(server.pid)],text=True)
      if '/dev/input/event' not in report:break
      time.sleep(.1)
     paused=descriptors('PAUSED');assert paused==0, 'input descriptors not retired'
     fcntl.ioctl(console,abi['activate'],1);active(1);time.sleep(1)
     limit=time.monotonic()+6
     while time.monotonic()<limit:
      report=subprocess.check_output(['/usr/bin/procstat','-f',str(server.pid)],text=True)
      if '/dev/input/event' in report:break
      time.sleep(.1)
     resumed=descriptors('RESUMED');assert resumed>0
     Path('/tmp/ws109-lifecycle-ready').write_text('ready')
     out,err=client.communicate(timeout=19)
     clients.append({'command':args,'exit':client.returncode,'stdout':out,'stderr':err})
    finally:
     if client.poll() is None:client.terminate();client.wait(timeout=3)
   server.wait(timeout=34)
  print(Path('/root/ws109-passthrough-evidence/compositor.txt').read_text(),flush=True)
  if Path(directory,'trace.txt').exists():
   trace=Path(directory,'trace.txt').read_text()
   print('NATIVE IOCTL TRACE\n'+'\n'.join(l for l in trace.splitlines() if 'ioctl(' in l),flush=True)
  print(json.dumps({'server_exit':server.returncode,'clients':clients,'remaining_socket':socket.exists()},indent=2),flush=True)
  assert server.returncode==0
  assert len(clients)==1 and all(c['exit']==0 for c in clients)
  report=Path('/root/ws109-passthrough-evidence/compositor.txt').read_text()
  import re
  exitline=re.search(r'ZWL EXIT frames=(\d+) error=0 cleanup_failed=0.*input_events=(\d+) seat_events=(\d+)',report)
  assert exitline and int(exitline[1])>10 and int(exitline[2])>0 and int(exitline[3])>=2, 'GPU resume/input/seat output missing'
  assert not socket.exists()
finally:
 if w>=0:os.close(w)
 os.close(r)
 if server and server.poll() is None:
  server.terminate()
  try:server.wait(timeout=5)
  except subprocess.TimeoutExpired:server.kill();server.wait(timeout=5)
 if daemon:
  daemon.terminate()
  try:diagnostics=daemon.communicate(timeout=5)[1]
  except subprocess.TimeoutExpired:daemon.kill();diagnostics=daemon.communicate(timeout=5)[1]
  print('owned seatd cleanup',daemon.returncode,diagnostics,flush=True)
 if endpoint.exists():
  assert endpoint.stat().st_ino==identity;endpoint.unlink()
 if prefix.exists():shutil.rmtree(prefix)
 fcntl.ioctl(console,abi['activate'],original['active'])
 fcntl.ioctl(console,abi['set_vt_mode'],original['vt'])
 fcntl.ioctl(console,abi['set_keyboard'],original['keyboard'])
 fcntl.ioctl(console,abi['set_drawing'],original['drawing'])
 termios.tcsetattr(console,termios.TCSAFLUSH,original['termios'])
 assert struct.unpack('i',inquiry('get_active'))[0]==original['active']
 assert inquiry('get_vt_mode',abi['vt_size'])[:2]==original['vt'][:2]
 assert struct.unpack('i',inquiry('get_keyboard'))[0]==original['keyboard']
 assert struct.unpack('i',inquiry('get_drawing'))[0]==original['drawing']
 assert termios.tcgetattr(console)==original['termios']
 assert modes==[Path('/dev/input/event'+str(i)).stat().st_mode for i in range(6)]
 os.close(console)
 Path('/tmp/ws109-lifecycle-ready').unlink(missing_ok=True)
 print('PASS native VT/mode/keyboard/drawing/termios/input permissions restored',flush=True)
print('PASS actual native Intel compositor/Vulkan window, VT pause/resume/input and private install cleanup',flush=True)
