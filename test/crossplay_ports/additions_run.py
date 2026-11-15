#!/usr/bin/env python3
import pathlib,subprocess,tempfile,shutil,json
ROOT=pathlib.Path(__file__).resolve().parents[2]
HERE=pathlib.Path(__file__).resolve().parent
OUT=pathlib.Path('/root/github/.host-data/crosssmudge/crossplay-ports')
APPS=['insider','forehead','checkers','wavelength','hearts','hex','battleship']
results=[]
for app in APPS:
 tests=[app+'.lua',('integration.lua' if app in ['checkers','hex','battleship'] else 'party_integration.lua'),'journal.lua']
 if app in ['insider','forehead','wavelength']:tests.append('party_capacity.lua')
 if app in ['insider','forehead']:tests.append('timer_privacy.lua')
 for test in tests:
  for cap in [76800,2097152]:
   for w,h in [(480,800),(800,480)]:
    with tempfile.TemporaryDirectory() as tmp:
     shutil.copytree(ROOT/'apps'/('cp-'+app),tmp,dirs_exist_ok=True)
     r=subprocess.run(['/tmp/crossplay-lua-runner',str(HERE/test),str(cap),str(w),str(h)],cwd=tmp,text=True,capture_output=True,timeout=120)
     row=dict(app=app,test=test,cap=cap,w=w,h=h,returncode=r.returncode,stdout=r.stdout,stderr=r.stderr);results.append(row)
     print(app,test,cap,w,r.returncode,r.stdout.strip(),r.stderr)
(OUT/'additions-results.json').write_text(json.dumps(results,indent=2)+'\n')
assert all(r['returncode']==0 for r in results)
