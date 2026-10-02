# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Exercise the installed real native compositor on the passed-through Intel GPU."""
from pathlib import Path
import os,pwd,grp,select,subprocess,tempfile,shutil,time,json
stage=Path('/root/keiland-stage-final/opt/keiland');prefix=Path('/opt/keiland');endpoint=Path('/var/run/seatd.sock')
assert not prefix.exists();assert not endpoint.exists();assert Path('/dev/dri/card0').exists()
identity=None;daemon=None;server=None;r,w=os.pipe();clients=[]
user=pwd.getpwnam('nobody');video=grp.getgrnam('video').gr_gid
try:
 shutil.copytree(stage,prefix)
 with tempfile.TemporaryDirectory(prefix='ws109-intel-',dir='/tmp') as directory:
  os.chown(directory,user.pw_uid,user.pw_gid);os.chmod(directory,0o700)
  env=dict(os.environ);env.pop('SEATD_VTBOUND',None)
  env.update(HOME=directory,XDG_CONFIG_HOME=directory,XDG_DATA_HOME=directory,XDG_RUNTIME_DIR=directory,WAYLAND_DISPLAY='wayland-keiland',KEILAND_SEAT='seatd',KEILAND_DRM_DEVICE='/dev/dri/card0',VK_DRIVER_FILES='/usr/local/share/vulkan/icd.d/intel_icd.x86_64.json')
  env.pop('LD_LIBRARY_PATH',None)
  daemon=subprocess.Popen(['/usr/local/bin/seatd','-u','nobody','-n',str(w),'-l','error'],pass_fds=(w,),env=env,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  os.close(w);w=-1
  assert select.select([r],[],[],5)[0];assert os.read(r,1)==b'\n';identity=endpoint.stat().st_ino
  def unprivileged():
   os.setgroups([video]);os.setgid(user.pw_gid);os.setuid(user.pw_uid)
  socket=Path(directory,'wayland-keiland')
  with Path('/root/ws109-passthrough-evidence/compositor.txt').open('w') as output:
   server=subprocess.Popen([str(prefix/'bin/wayland'),'--socket='+str(socket),'--timeout=12','--log-frames','--desktop-client=none'],preexec_fn=unprivileged,env=env,cwd=directory,stdout=output,stderr=subprocess.STDOUT,text=True)
   limit=time.monotonic()+8
   while not socket.exists() and server.poll() is None and time.monotonic()<limit:time.sleep(0.05)
   if socket.exists():
    for args in [['wlshm','--frames=3','--token=ws109-intel-shm'],['/tmp/ws109-wsi-probe','--frames','3','--timeout','5']]:
     run=subprocess.run(([str(prefix/'bin'/args[0]),*args[1:]] if not args[0].startswith('/') else ['/usr/bin/truss','-o',directory+'/trace.txt',args[0],*args[1:]]),preexec_fn=unprivileged,env=env,cwd=directory,capture_output=True,text=True,timeout=6)
     clients.append({'command':args,'exit':run.returncode,'stdout':run.stdout,'stderr':run.stderr})
   server.wait(timeout=16)
  print(Path('/root/ws109-passthrough-evidence/compositor.txt').read_text(),flush=True)
  if Path(directory,'trace.txt').exists():
   trace=Path(directory,'trace.txt').read_text()
   print('NATIVE IOCTL TRACE\n'+'\n'.join(l for l in trace.splitlines() if 'ioctl(' in l),flush=True)
  print(json.dumps({'server_exit':server.returncode,'clients':clients,'remaining_socket':socket.exists()},indent=2),flush=True)
  assert server.returncode==0
  assert len(clients)==2 and all(c['exit']==0 for c in clients)
  assert 'wsi-probe-client: PASS' in clients[1]['stdout'], 'trace wrapper exit is not client success'
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
print('PASS actual native compositor Intel GPU/shm and Vulkan clients/private install cleanup',flush=True)
