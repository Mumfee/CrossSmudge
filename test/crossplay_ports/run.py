#!/usr/bin/env python3
"""Reproducible stock-ABI Lua tests. Native API adapter, never a HAL claim."""
import hashlib,json,pathlib,shutil,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
HERE=pathlib.Path(__file__).resolve().parent
OUT=pathlib.Path('/root/github/.host-data/crosssmudge/crossplay-ports')
STOCK='bae8322b7933fc46007682d2b2d13db7c157f729'
GAMES=['minesweeper','connectfour','knucklebones','calculator','notes','yahtzee']
# Ensure compiler configuration AND entire vendored VM match delivered release.
changed=subprocess.check_output(['git','diff',STOCK,'--','lib/Lua'],cwd=ROOT,text=True)
assert not changed,'Vendored Lua differs from target firmware'
subprocess.run(['cc','-O2','-I'+str(ROOT/'lib/Lua/src'),str(HERE/'runner.c'),
 *map(str,sorted((ROOT/'lib/Lua/src').glob('*.c'))),'-lm','-o','/tmp/crossplay-lua-runner'],check=True)
results=[]
def execute(app,test,cap=76800,w=480,h=800,extra=None):
 with tempfile.TemporaryDirectory(prefix='cp-verify-') as temp:
  p=pathlib.Path(temp)
  shutil.copytree(ROOT/'apps'/('cp-'+app),p,dirs_exist_ok=True)
  if extra:
   (p/'test.lua').write_text(extra);script='test.lua'
  else:script=str(HERE/test)
  r=subprocess.run(['/tmp/crossplay-lua-runner',script,str(cap),str(w),str(h)],cwd=p,text=True,capture_output=True,timeout=120)
  lines=r.stdout.strip().splitlines();metrics=json.loads(lines[-1]) if lines else {}
  row={'app':'cp-'+app,'test':test,'cap':cap,'width':w,'height':h,'returncode':r.returncode,'metrics':metrics,'stdout':r.stdout,'stderr':r.stderr}
  results.append(row)
  if r.returncode:print(json.dumps(row),flush=True)
  return r
for app in GAMES:
 execute(app,app+'.lua')
 # Compile every module using the exact firmware Lua parser, under the budget.
 files=sorted(p.name for p in (ROOT/'apps'/('cp-'+app)).glob('*.lua'))
 execute(app,'syntax',extra='\n'.join("assert(loadfile(%s));collectgarbage('collect')"%json.dumps(f) for f in files))
 script=app+'_integration.lua' if app in ['calculator','notes','yahtzee'] else 'integration.lua'
 for cap in [76800,2097152]:
  for w,h in [(480,800),(800,480)]:execute(app,script,cap,w,h)
 # Repeated truly independent VM launches, including full compile/start/close.
 for i in range(12):execute(app,'fresh-launch-'+str(i),extra="dofile('main.lua');on_init();on_draw();on_exit()")
execute('minesweeper','journal.lua')
execute('notes','notes_capacity.lua')
execute('notes','notes_capacity.lua',76800,800,480)
OUT.mkdir(parents=True,exist_ok=True)
(OUT/'runtime-results.json').write_text(json.dumps(results,indent=2)+'\n')
print('Native adapter cases:',len(results),'passed:',sum(r['returncode']==0 for r in results))
assert all(r['returncode']==0 and r['metrics'].get('after_close')==0 for r in results)
