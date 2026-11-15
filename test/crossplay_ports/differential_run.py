#!/usr/bin/env python3
"""Compile original C++ oracle; run each independent game in capped vendored Lua.
Requires test adapter /tmp/crossplay-lua-runner, compiled by main task runner.
No production rendering is claimed. CrossPlay sources remain read-only.
"""
import pathlib, subprocess, shutil, json
ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCE = pathlib.Path('/root/github/crossplay/src/apps_local')
subprocess.run(['c++','-std=c++17','-O2','-I'+str(SOURCE),str(pathlib.Path(__file__).with_name('differential.cpp')),'-o','/tmp/crossplay-differential'],check=True)
results=[]
for game in ['minesweeper','knucklebones','connectfour']:
    fixture=subprocess.check_output(['/tmp/crossplay-differential',game],text=True)
    header,*lines=fixture.splitlines()
    cases=[]; current=[]
    for line in lines:
        if line.startswith('g=') and current: cases.append(current);current=[]
        if not line.startswith("print("):current.append(line)
    if current:cases.append(current)
    stage=pathlib.Path('/tmp')/('crossplay-differential-'+game);stage.mkdir(exist_ok=True)
    shutil.copy(ROOT/'apps'/('cp-'+game)/'core.lua',stage/'core.lua')
    for i,case in enumerate(cases):
        (stage/'test.lua').write_text(header+'\n'+'\n'.join(case)+'\n')
        r=subprocess.run(['/tmp/crossplay-lua-runner','test.lua','76800'],cwd=stage,text=True,capture_output=True)
        record={'game':game,'case':i,'exit_code':r.returncode,'output':r.stdout.strip(),'error':r.stderr.strip()}
        results.append(record)
        if r.returncode: print(json.dumps(record),flush=True)
    print(game, 'cases',len(cases),'passed',sum(x['exit_code']==0 for x in results if x['game']==game),flush=True)
path=pathlib.Path('/root/github/.host-data/crosssmudge/crossplay-ports/differential-results.json')
path.write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(any(x['exit_code'] for x in results))
