# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Exercise native installed GPU applications with owned fixtures and positive outcomes."""
from pathlib import Path
import os,pwd,grp,select,subprocess,tempfile,shutil,time,json,zlib,struct,sys,fcntl,termios
stage=Path('/root/keiland-stage-final/opt/keiland');prefix=Path('/opt/keiland');endpoint=Path('/var/run/seatd.sock')
assert not prefix.exists() and not endpoint.exists()
abi=json.loads(subprocess.check_output(['/tmp/ws109-console-abi'],text=True))
console=os.open('/dev/ttyv0',os.O_RDWR|os.O_NOCTTY)
def inquiry(request,size=4):return fcntl.ioctl(console,abi[request],bytes(size))
original={'active':struct.unpack('i',inquiry('get_active'))[0],'vt':inquiry('get_vt_mode',abi['vt_size']),'drawing':struct.unpack('i',inquiry('get_drawing'))[0],'keyboard':struct.unpack('i',inquiry('get_keyboard'))[0],'termios':termios.tcgetattr(console)}
modes=[Path('/dev/input/event'+str(i)).stat().st_mode for i in range(6)]
identity=None;daemon=None;server=None;r,w=os.pipe();client=None
user=pwd.getpwnam('nobody');video=grp.getgrnam('video').gr_gid
results=[];evidence=Path('/root/ws109-passthrough-evidence');evidence.mkdir(exist_ok=True)
def unprivileged():
 os.setgroups([video]);os.setgid(user.pw_gid);os.setuid(user.pw_uid)
def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
try:
 shutil.copytree(stage,prefix)
 with tempfile.TemporaryDirectory(prefix='ws109-native-apps-',dir='/tmp') as directory:
  runtime=Path(directory);os.chown(runtime,user.pw_uid,user.pw_gid);os.chmod(runtime,0o700)
  env=dict(os.environ);env.pop('SEATD_VTBOUND',None);env.pop('LD_LIBRARY_PATH',None)
  env.update(HOME=directory,XDG_CONFIG_HOME=directory,XDG_DATA_HOME=directory,XDG_RUNTIME_DIR=directory,WAYLAND_DISPLAY='wayland-keiland',KEILAND_SEAT='seatd',KEILAND_DRM_DEVICE='/dev/dri/card0',VK_DRIVER_FILES='/usr/local/share/vulkan/icd.d/intel_icd.x86_64.json')
  text=runtime/'sample.txt';text.write_text('FreeBSD native text\nKeiland GPU sample\n')
  image=runtime/'sample.png';image.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',32,32,8,2,0,0,0))+chunk(b'IDAT',zlib.compress((b'\0'+b'\x40\x80\xd0'*32)*32))+chunk(b'IEND',b''))
  pdf=runtime/'sample.pdf';objects=[b'<< /Type /Catalog /Pages 2 0 R >>',b'<< /Type /Pages /Kids [3 0 R] /Count 1 >>',b'<< /Type /Page /Parent 2 0 R /MediaBox [0 0 300 300] /Resources << >> /Contents 4 0 R >>',b'<< /Length 27 >>\nstream\n0 0 1 rg 30 30 100 100 re f\nendstream']
  body=b'%PDF-1.4\n';offsets=[0]
  for i,obj in enumerate(objects,1):offsets.append(len(body));body+=str(i).encode()+b' 0 obj\n'+obj+b'\nendobj\n'
  xref=len(body);body+=b'xref\n0 5\n0000000000 65535 f \n'+b''.join(f'{o:010d} 00000 n \n'.encode() for o in offsets[1:])+f'trailer\n<< /Size 5 /Root 1 0 R >>\nstartxref\n{xref}\n%%EOF\n'.encode();pdf.write_bytes(body)
  for path in [text,image,pdf]:os.chown(path,user.pw_uid,user.pw_gid)
  daemon=subprocess.Popen(['/usr/local/bin/seatd','-u','nobody','-n',str(w),'-l','error'],pass_fds=(w,),env=env,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,text=True)
  os.close(w);w=-1;assert select.select([r],[],[],5)[0] and os.read(r,1)==b'\n';identity=endpoint.stat().st_ino
  socket=runtime/'wayland-keiland';serverlog=evidence/'main-apps-compositor.txt'
  with serverlog.open('w') as output:
   server=subprocess.Popen([str(prefix/'bin/wayland'),'--socket='+str(socket),'--timeout=90','--log-frames','--desktop-client=none','--desktop-token=ws109-native'],preexec_fn=unprivileged,env=env,cwd=directory,stdout=output,stderr=subprocess.STDOUT,text=True)
   deadline=time.monotonic()+10
   while not socket.exists() and server.poll() is None and time.monotonic()<deadline:time.sleep(.05)
   assert socket.exists()
   cases=[('terminal',['--timeout-s=12','--command=read answer; printf "%s" "$answer" > '+directory+'/typed.txt; sleep 1'],['ZTERM START','ZTERM DONE']),('files',['--timeout-s=5',directory],['ZFILES READY','ZFILES DONE']),('settings',['--timeout-s=5','sound'],['ZSETTINGS READY','ZSETTINGS DONE']),('notes',['--timeout-s=5',str(pdf)],['NOTES START','NOTES EXIT']),('textedit',['--timeout-s=5',str(text)],['TEXTEDIT READY','TEXTEDIT DONE']),('imageview',['--timeout-s=5',str(image)],['IMAGEVIEW READY','IMAGEVIEW DONE']),('pdfviewer',['--timeout-s=5',str(pdf)],['PDFVIEWER READY','PDFVIEWER DONE']),('files',['--desktop','--timeout-s=5',directory],['ZFILES READY','ZFILES DONE'])]
   if '--only-desktop' in sys.argv:cases=cases[-1:]
   for name,args,positive in cases:
    before=serverlog.read_text().count('ZWL MAP ')
    clientlog=evidence/(name+('-desktop' if '--desktop' in args else '')+'.txt')
    with clientlog.open('w') as clientout:
     clientenv=dict(env)
     if '--desktop' in args:clientenv['KEILAND_DESKTOP_TOKEN']='ws109-native'
     client=subprocess.Popen([str(prefix/'bin'/name),*args],preexec_fn=unprivileged,env=clientenv,cwd=directory,stdout=clientout,stderr=subprocess.STDOUT,text=True)
     if name=='terminal':
      deadline=time.monotonic()+8
      while client.poll() is None and time.monotonic()<deadline:
       if serverlog.read_text().count('ZWL MAP ')>before:break
       time.sleep(.05)
      assert serverlog.read_text().count('ZWL MAP ')>before
      Path('/tmp/ws109-app-terminal-ready').write_text(directory)
     client.wait(timeout=20)
    report=clientlog.read_text();print(name,client.returncode,report,flush=True)
    assert client.returncode==0 and all(marker in report for marker in positive), name
    assert 'FAILED' not in report,name
    if '--desktop' not in args:assert serverlog.read_text().count('ZWL MAP ')>before,name+' unmapped'
    if name=='terminal':
     assert (runtime/'typed.txt').read_text()=='freebsd', 'actual kernel/Wayland/PTY input not delivered'
     Path('/tmp/ws109-app-terminal-ready').unlink()
    results.append({'app':name,'args':args,'exit':client.returncode,'positive':positive})
   server.terminate();server.wait(timeout=10)
  report=serverlog.read_text();print(report,flush=True)
  assert server.returncode==0 and 'error=0 cleanup_failed=0' in report and not socket.exists()
  assert report.count('ZWL IMPORT ')>=3*len(cases)
  assert 'ZWL DESKTOP' in report, 'native App Home role not observed'
  print(json.dumps(results,indent=2),flush=True)
finally:
 if client and client.poll() is None:client.terminate();client.wait(timeout=5)
 if server and server.poll() is None:server.terminate();server.wait(timeout=10)
 if w>=0:os.close(w)
 os.close(r)
 if daemon:
  daemon.terminate();diagnostics=daemon.communicate(timeout=5)[1];print('owned seatd cleanup',daemon.returncode,diagnostics,flush=True)
 if endpoint.exists():assert endpoint.stat().st_ino==identity;endpoint.unlink()
 if prefix.exists():shutil.rmtree(prefix)
 Path('/tmp/ws109-app-terminal-ready').unlink(missing_ok=True)
 # The daemon changes console defaults; restore independently captured effective state.
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
 print('PASS native console/input permission restoration',flush=True)
if '--only-desktop' in sys.argv:print('PASS installed real nativeGPU App Home role/private install cleanup',flush=True)
else:print('PASS installed real nativeGPU mainapps/file opens/keyboard PTY/private install cleanup',flush=True)
