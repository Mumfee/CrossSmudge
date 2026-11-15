#!/usr/bin/env python3
import pathlib,subprocess,tempfile,shutil,json
ROOT=pathlib.Path(__file__).resolve().parents[2];HERE=pathlib.Path(__file__).resolve().parent
subprocess.run(['c++','-O2','-std=c++17',str(HERE/'checkers_oracle.cpp'),'-o','/tmp/checkers-oracle'],check=True)
rows=subprocess.check_output(['/tmp/checkers-oracle'],text=True).splitlines();results=[]
for row in rows:
 state,moves,best=row.split(';')
 script='''local M=dofile('core.lua')
local function key(m)
 local t={};for i=3,#m do t[#t+1]=m:byte(i) end;table.sort(t)
 local s=m:byte(1)..':'..m:byte(2);for _,v in ipairs(t) do s=s..':'..v end;return s
end
'''+f'local g=assert(M.decode({json.dumps(state)}));local expected={json.dumps(moves)};local out={{}}\n'+"for _,m in ipairs(M.moves(g)) do out[#out+1]=key(m) end;assert(table.concat(out,'/')==expected)\n"+f'assert(key(M.choose(g))=={json.dumps(best)})\n'
 with tempfile.TemporaryDirectory() as tmp:
  shutil.copytree(ROOT/'apps/cp-checkers',tmp,dirs_exist_ok=True);pathlib.Path(tmp,'test.lua').write_text(script)
  r=subprocess.run(['/tmp/crossplay-lua-runner','test.lua','76800'],cwd=tmp,text=True,capture_output=True)
  results.append(dict(state=state,returncode=r.returncode,stdout=r.stdout,stderr=r.stderr))
  if r.returncode:print(row,r.stderr)
pathlib.Path('/root/github/.host-data/crosssmudge/crossplay-ports/checkers-oracle-results.json').write_text(json.dumps(results,indent=2)+'\n')
assert all(r['returncode']==0 for r in results)
print(len(results),'original C++ legal-move and two-ply choices matched')
