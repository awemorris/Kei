import pathlib,subprocess,json
files=[p for p in pathlib.Path('build/ws105-p011/new-scope.txt').read_text().splitlines() if p.endswith('.c')]+['plan/tools/keiland-linux/dbus-wire.c','plan/tools/keiland-linux/seat-fd.c']
findings=[]
for f in files:
 r=subprocess.run(['clang','-D_GNU_SOURCE','-DKEILAND_VULKAN_BACKEND_PATHS="/usr/lib/x86_64-linux-gnu/libvulkan.so.1"','-std=gnu17','-I.','-Iuserland/desktop/keiland','-Ibuild/keiland-linux/include','-Ibuild/keiland-linux/gen/libvulkan-compat','-Ibuild/keiland-linux/test/gen','-Xclang','-ast-dump=json','-fsyntax-only',f],capture_output=True,text=True)
 if r.returncode:findings.append([f,0,'COMPILE',r.stderr[-900:]]);continue
 ast=json.loads(r.stdout);src=pathlib.Path(f).read_text();lines=src.splitlines()
 def walk(n,parent=None):
  yield n,parent
  for x in n.get('inner',[]):yield from walk(x,n)
 for fn in ast.get('inner',[]):
  if fn.get('kind')!='FunctionDecl' or fn.get('loc',{}).get('includedFrom') or fn.get('loc',{}).get('file',f)!=f:continue
  body=next((n for n in fn.get('inner',[]) if n.get('kind')=='CompoundStmt'),None)
  if not body:continue
  ret=fn['type']['qualType'].split(' (')[0];last=body.get('inner',[])[-1]
  if ret=='void' and last.get('kind') not in ['ReturnStmt','ForStmt','WhileStmt','DoStmt','IfStmt']:
   findings.append([f,src.count('\n',0,last['range']['begin'].get('offset',0))+1,'VOID_END',fn['name']])
  if last.get('kind')=='ReturnStmt':
   off=last['range']['begin'].get('offset',0);line=src.count('\n',0,off)+1
   if ret=='VkResult' and lines[line-1].strip() not in ['return VK_SUCCESS;']:
    findings.append([f,line,'RESULT_END',fn['name']+' '+lines[line-1]])
  for n,parent in walk(body):
   off=n.get('range',{}).get('begin',{}).get('offset',0);line=src.count('\n',0,off)+1;kind=n.get('kind')
   if kind=='CompoundStmt' and parent and parent.get('kind')=='CompoundStmt':findings.append([f,line,'SCOPE_BLOCK',fn['name']])
   if kind=='BinaryOperator' and n.get('opcode') in ['==','!=','<','<=','>','>=','&&','||'] and parent and parent.get('kind') in ['ReturnStmt','BinaryOperator','VarDecl']:
    if parent.get('kind')=='BinaryOperator' and parent.get('opcode') not in ['=']:continue
    if 'errno' not in lines[line-1]:findings.append([f,line,'BOOL_VALUE',lines[line-1]])
 print('audit',f,flush=True)
pathlib.Path('build/ws105-p011/ast-structure.json').write_text(json.dumps(findings,indent=2))
for row in findings:print(*row,sep=':')
