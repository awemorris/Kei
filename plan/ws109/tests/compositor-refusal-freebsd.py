# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Check the installed real compositor's absent-primary startup without replacing its collaborators."""
from pathlib import Path
import os,select,subprocess,sys,tempfile
stage=Path(sys.argv[1]).resolve()
endpoint=Path('/var/run/seatd.sock')
assert not endpoint.exists(), 'existing endpoint belongs to another owner'
assert not Path('/dev/dri/card0').exists(), 'this refusal fixture requires absent DRM'
with tempfile.TemporaryDirectory(prefix='ws109-compositor-',dir='/tmp') as directory:
 os.chmod(directory,0o755)
 libraries=Path(directory,'lib');libraries.mkdir(mode=0o755)
 # The unprivileged child cannot traverse a root-owned stage directory.
 for library in (stage/'lib').glob('*.so*'):
  destination=libraries/library.name
  destination.write_bytes(library.read_bytes());destination.chmod(0o755)
 program=Path(directory,'wayland');program.write_bytes((stage/'bin/wayland').read_bytes());program.chmod(0o755)
 r,w=os.pipe();daemon=None;identity=None
 try:
  env=os.environ.copy();env['SEATD_VTBOUND']='0';env['LD_LIBRARY_PATH']=str(libraries)
  daemon=subprocess.Popen(['/usr/local/bin/seatd','-u','nobody','-n',str(w),'-l','error'],pass_fds=(w,),env=env,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  os.close(w);w=-1
  assert select.select([r],[],[],5)[0];assert os.read(r,1)==b'\n'
  identity=endpoint.stat().st_ino
  def unprivileged():
   os.setgroups([]);os.setgid(65534);os.setuid(65534)
  socket=Path(directory,'owned-wayland');env['KEILAND_DRM_DEVICE']='/dev/dri/card0';env['KEILAND_SEAT']='seatd'
  run=subprocess.run([str(program),'--socket='+str(socket),'--timeout=1','--desktop-client=none'],preexec_fn=unprivileged,env=env,cwd='/tmp',capture_output=True,text=True,timeout=10)
  print(run.stdout,run.stderr,flush=True)
  assert run.returncode==1
  assert 'ZWL OS unavailable errno=2' in run.stdout
  assert 'ZWL COMPOSE unavailable' not in run.stdout
  assert 'error=2 cleanup_failed=0' in run.stdout
  assert not socket.exists()
  # Verify the daemon survived the rejected compositor's cleanup.
  assert daemon.poll() is None
  print('PASS real native compositor absent primary refusal/cleanup; authority remains live',flush=True)
 finally:
  os.close(r)
  if w>=0:os.close(w)
  if daemon:
   daemon.terminate()
   try:out=daemon.communicate(timeout=5)[1]
   except subprocess.TimeoutExpired:daemon.kill();out=daemon.communicate(timeout=5)[1]
   print('owned seatd exit',daemon.returncode,flush=True)
  if endpoint.exists():
   assert endpoint.stat().st_ino==identity
   endpoint.unlink()
