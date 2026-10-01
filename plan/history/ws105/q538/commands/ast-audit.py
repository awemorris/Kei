import json, subprocess, pathlib
root=pathlib.Path('.')
files=[p for p in pathlib.Path('build/ws105-p011/new-scope.txt').read_text().splitlines() if p.endswith('.c')]+['plan/tools/keiland-linux/dbus-wire.c','plan/tools/keiland-linux/seat-fd.c']
findings=[]
def walk(n):
 yield n
 for c in n.get('inner',[]): yield from walk(c)
def real_call(n):
 if n.get('kind')!='CallExpr':return False
 return not any(x.get('referencedDecl',{}).get('name')=='__errno_location' for x in walk(n))
def off(n):
 return n.get('range',{}).get('begin',{}).get('offset',0)
for f in files:
 cmd=['clang','-D_GNU_SOURCE','-DKEILAND_VULKAN_BACKEND_PATHS="/usr/lib/x86_64-linux-gnu/libvulkan.so.1"','-std=gnu17','-I.','-Iuserland/desktop/keiland','-Ibuild/keiland-linux/include','-Ibuild/keiland-linux/gen/libvulkan-compat','-Ibuild/keiland-linux/test/gen','-Xclang','-ast-dump=json','-fsyntax-only',f]
 r=subprocess.run(cmd,capture_output=True,text=True)
 if r.returncode:
  findings.append((f,0,'COMPILE',r.stderr[-1500:]));continue
 ast=json.loads(r.stdout);src=pathlib.Path(f).read_text();lines=src.splitlines()
 for fn in ast.get('inner',[]):
  if fn.get('kind')!='FunctionDecl' or fn.get('loc',{}).get('includedFrom'):continue
  if fn.get('loc',{}).get('file',f)!=f:continue
  body=next((x for x in fn.get('inner',[]) if x.get('kind')=='CompoundStmt'),None)
  if not body: continue
  leading=set();executable=False
  for x in body.get('inner',[]):
   if x.get('kind')=='DeclStmt' and not executable: leading.add(x['id'])
   else:executable=True
  for n in walk(body):
   kind=n.get('kind');line=src.count('\n',0,off(n))+1
   if kind=='DeclStmt' and n['id'] not in leading:
    findings.append((f,line,'LATE_DECL',lines[line-1]))
   if kind=='VarDecl' and any(real_call(x) for x in walk(n)):
    findings.append((f,line,'INIT_CALL',lines[line-1]))
   cond=[]
   if kind in ('IfStmt','WhileStmt','SwitchStmt'):cond=n.get('inner',[])[:1]
   if kind=='DoStmt':cond=n.get('inner',[])[1:2]
   if kind=='ForStmt':cond=n.get('inner',[])[2:3]
   if any(real_call(x) for c in cond for x in walk(c)):
    findings.append((f,line,'COND_CALL',lines[line-1]))
   if kind=='ReturnStmt' and any(real_call(x) for x in walk(n)):
    findings.append((f,line,'RETURN_CALL',lines[line-1]))
 print('audited',f,flush=True)
pathlib.Path('build/ws105-p011/ast-findings.json').write_text(json.dumps(findings,indent=2))
for x in findings:print(*x,sep=':')
print('findings',len(findings))
