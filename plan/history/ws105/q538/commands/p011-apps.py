from pathlib import Path
import subprocess,time,re
out=Path.cwd()/'build/ws105-p011';g=['sh','plan/tools/keiland-linux/guest.sh']
def run(*args):
 p=subprocess.run(g+list(args),text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=140)
 if p.returncode:raise RuntimeError((args,p.returncode,p.stdout))
 return p.stdout
def shot(name):run('screenshot',str(out/(name+'.png')))
def close_latest(name):
 s=run('ssh','grep "ZWL MAP" /tmp/wayland.log | tail -1; grep "ZWL IMPORT client" /tmp/wayland.log | tail -3')
 (out/(name+'-geometry.log')).write_text(s)
 m=re.search(r'ZWL MAP client=(\d+).* x=(-?\d+) y=(-?\d+)',s);assert m,s
 client,x,y=map(int,m.groups());width=int(re.findall(r'ZWL IMPORT client='+str(client)+r'.* width=(\d+)',s)[-1])
 run('click',str(x+width-26),str(y-30));time.sleep(2)
 s=run('ssh','pidof wayland; grep "ZWL HOME ended" /tmp/wayland.log | tail -1')
 (out/(name+'-closed.log')).write_text(s)
 assert 'status=0' in s,(name,s)
for name,x,y in [('files',280,274),('terminal',712,274),('settings',568,274),('notes',424,274),('textedit',280,426),('imageview',1000,274),('pdfviewer',856,274),('kuidemo',568,426),('mview',424,426)]:
 run('click','23','17');time.sleep(.6);run('click',str(x),str(y));time.sleep(10)
 s=run('ssh','pidof '+name+' && pidof wayland');(out/(name+'-live.log')).write_text(s);shot(name)
 if name=='terminal':run('click','500','430');run('type','echo keiland-final-ok');run('key','ret');time.sleep(1);shot('terminal-echo')
 if name=='textedit':
  run('click','500','400');run('type','keiland-final-text');time.sleep(1);shot('textedit-typed')
  run('key','ctrl','a');run('key','backspace');run('key','alt','spc');time.sleep(1)
  run('key','shift','k');run('type','anji');run('key','spc');time.sleep(1);shot('ime-conversion');run('key','ret');time.sleep(1);shot('ime-committed');run('key','alt','spc')
 close_latest(name);print(name+': Home live10s / close status0',flush=True)
run('put','build/ws105-p011/linux-console.png','/tmp/p011-picture.png')
run('put','build/ws079-host/writer-plain.pdf','/tmp/p011-document.pdf')
for name,file in [('imageview','/tmp/p011-picture.png'),('pdfviewer','/tmp/p011-document.pdf')]:
 run('ssh',f'XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 nohup /opt/keiland/bin/{name} {file} > /tmp/p011-{name}.log 2>&1 &')
 time.sleep(6);shot(name+'-loaded');(out/(name+'-loaded.log')).write_text(run('ssh',f'cat /tmp/p011-{name}.log; pidof {name}'))
 s=run('ssh','grep "ZWL MAP" /tmp/wayland.log | tail -1; grep "ZWL IMPORT client" /tmp/wayland.log | tail -3')
 m=re.search(r'ZWL MAP client=(\d+).* x=(-?\d+) y=(-?\d+)',s);assert m,s
 client,x,y=map(int,m.groups());width=int(re.findall(r'ZWL IMPORT client='+str(client)+r'.* width=(\d+)',s)[-1]);run('click',str(x+width-26),str(y-30));time.sleep(2)
(out/'wayland-after-apps.log').write_text(run('ssh','cat /tmp/wayland.log'))
print('apps: PASS Home9 / Terminal echo / Text Editor / IME screenshots / image / PDF',flush=True)
