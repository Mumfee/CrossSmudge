from pathlib import Path
import subprocess,tempfile,shutil,json
R=Path(__file__).resolve().parents[2];H=R/'test/crossplay_ports';O=Path('/root/github/.host-data/crosssmudge/crossplay-ports')
rows=[]
for app in ['jaipur','seasalt','toybattle','go','murdle']:
 tests=[app+'.lua','five_ui.lua','journal.lua','review_five_persistence.lua']
 if app in ['toybattle','go','seasalt']:tests+=['review_five_'+('toy' if app=='toybattle' else app)+'.lua']
 if app=='murdle':tests+=['review_five_murdle.lua']
 if app=='seasalt':tests+=['review_five_seasalt_capacity.lua']
 for test in tests:
  for cap in [76800,2097152]:
   for w,h in [(480,800),(800,480)]:
    with tempfile.TemporaryDirectory() as t:
     shutil.copytree(R/'apps'/('cp-'+app),t,dirs_exist_ok=True)
     r=subprocess.run(['/tmp/crossplay-lua-runner',str(H/test),str(cap),str(w),str(h)],cwd=t,text=True,capture_output=True,timeout=240)
     row=dict(app=app,test=test,cap=cap,width=w,height=h,returncode=r.returncode,stdout=r.stdout,stderr=r.stderr);rows.append(row)
     print(app,test,cap,w,r.returncode,r.stderr.strip(),flush=True)
(O/'five-runtime-results.json').write_text(json.dumps(rows,indent=2)+'\n')
assert all(r['returncode']==0 for r in rows)
