from pathlib import Path
import subprocess,tempfile,shutil,json,hashlib
R=Path(__file__).resolve().parents[2];H=R/'test/crossplay_ports';S=Path('/root/github/crossplay/src/apps_local');O=Path('/root/github/.host-data/crosssmudge/crossplay-ports')
cmd=['c++','-std=c++17','-O2',str(H/'five_oracle.cpp')]
for a,c in [('jaipur','JaipurCore'),('seasalt','SeaSaltCore'),('toybattle','ToyBattleCore'),('go','GoCore')]:cmd+=['-I'+str(S/a),str(S/a/(c+'.cpp'))]
subprocess.run(cmd+['-o','/tmp/cp-five-oracle'],check=True)
rows=[]
for app in ['jaipur','seasalt','go','toybattle']:
 for seed in [1,123,919]:
  args=[9,13,19] if app=='go' else list(range(10)) if app=='toybattle' else [0]
  for arg in args:
   with tempfile.TemporaryDirectory() as t:
    shutil.copytree(R/'apps'/('cp-'+app),t,dirs_exist_ok=True);p=Path(t)
    (p/'trace-config.lua').write_text(f"return {{app='{app}',seed={seed},arg={arg if app in ['go','toybattle'] else 'nil'}}}")
    r=subprocess.run(['/tmp/crossplay-lua-runner',str(H/'five_trace.lua'),'2097152'],cwd=t,text=True,capture_output=True,timeout=240)
    row=dict(app=app,seed=seed,arg=arg,cap=2097152,returncode=r.returncode,stdout=r.stdout,stderr=r.stderr);rows.append(row)
    if r.returncode:print(row,flush=True);continue
    oracle=subprocess.run(['/tmp/cp-five-oracle',app,str(seed),str(arg)],input=(p/'input.txt').read_text(),text=True,capture_output=True,check=True)
    want=(p/'expected.txt').read_text().splitlines();actual=oracle.stdout.splitlines();row['input_sha256']=hashlib.sha256((p/'input.txt').read_bytes()).hexdigest();row['expected_sha256']=hashlib.sha256((p/'expected.txt').read_bytes()).hexdigest();row['actions']=len(want);row['oracle_match']=want==actual
    if want!=actual:
     for i,(a,b) in enumerate(zip(want,actual)):
      if a!=b:print(app,seed,arg,'mismatch',i,'lua',a,'cpp',b,flush=True);break
    print(app,seed,arg,len(want),row['oracle_match'],flush=True)
(O/'five-oracle-results.json').write_text(json.dumps(rows,indent=2)+'\n')
assert all(r['returncode']==0 and r.get('oracle_match') for r in rows)
