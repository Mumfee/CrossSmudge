#!/usr/bin/env python3
import pathlib,subprocess,shutil,json
root=pathlib.Path(__file__).resolve().parents[2];source=pathlib.Path('/root/github/crossplay/src/apps_local')
subprocess.run(['c++','-std=c++17','-O2','-I'+str(source),str(pathlib.Path(__file__).with_name('yahtzee_review.cpp')),'-o','/tmp/crossplay-yahtzee-review'],check=True)
lines=subprocess.check_output(['/tmp/crossplay-yahtzee-review'],text=True).splitlines();header=lines[0];cases=[];current=[]
for line in lines[1:]:
 if line.startswith(('g=','h=')) and current:cases.append(current);current=[]
 current.append(line)
if current:cases.append(current)
p=pathlib.Path('/tmp/crossplay-yahtzee-review-stage');p.mkdir(exist_ok=True);shutil.copy(root/'apps/cp-yahtzee/core.lua',p/'core.lua')
results=[]
for i,case in enumerate(cases):
 (p/'test.lua').write_text(header+'\n'+'\n'.join(case)+'\n')
 r=subprocess.run(['/tmp/crossplay-lua-runner','test.lua','76800'],cwd=p,text=True,capture_output=True)
 row={'case':i,'returncode':r.returncode,'stdout':r.stdout,'stderr':r.stderr};results.append(row)
 if r.returncode:print(json.dumps(row),flush=True)
 if i%100==0:print('completed',i,flush=True)
pathlib.Path('/root/github/.host-data/crosssmudge/crossplay-ports/yahtzee-review-results.json').write_text(json.dumps(results,indent=2)+'\n')
print('Yahtzee cases',len(results),'passed',sum(x['returncode']==0 for x in results))
raise SystemExit(any(x['returncode'] for x in results))
