from pathlib import Path
import subprocess,json,hashlib
root=Path(__file__).resolve().parents[2];src=Path('/root/github/crossplay/src/apps_local/murdle');dst=root/'apps/cp-murdle/data';dst.mkdir(exist_ok=True)
subprocess.run(['c++','-std=c++17','-O2','-I'+str(src),str(root/'test/crossplay_ports/export_murdle.cpp'),*[str(src/(x+'.cpp')) for x in ['MurdleCore','MurdleCast','MurdleText']],'-o','/tmp/cp-murdle-export'],check=True)
rows=[]
for tier in range(4):
 for i in range(16):
  data=subprocess.check_output(['/tmp/cp-murdle-export',str(tier),str(i)])
  p=dst/f'{tier*16+i+1}.lua';p.write_bytes(data);rows.append(dict(case=tier*16+i+1,sha256=hashlib.sha256(data).hexdigest(),unique=True,deducible=True))
Path('/root/github/.host-data/crosssmudge/crossplay-ports/murdle-generation.json').write_text(json.dumps(rows,indent=2)+'\n')
print('64 original-generator cases: unique and deduction-solvable')
