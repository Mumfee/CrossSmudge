#!/usr/bin/env python3
import pathlib,subprocess,shutil,tempfile,json
root=pathlib.Path(__file__).resolve().parents[2];results=[]
for name in ['notes_capacity.lua','notes_capacity_unique.lua','notes_review_scroll.lua']:
 stage=pathlib.Path(tempfile.mkdtemp(prefix='crossplay-notes-capacity-review-',dir='/tmp'))
 shutil.copytree(root/'apps/cp-notes',stage,dirs_exist_ok=True)
 if name=='notes_capacity_unique.lua':
  fixture=(root/'test/crossplay_ports/notes_capacity.lua').read_text()
  first="local doc=string.rep('- [ ] '..string.rep('a',25)..'\\n',96)"
  replacement="local parts={};for i=1,96 do parts[i]='- [ ] '..string.format('%025d',i)..'\\n' end;local doc=table.concat(parts);parts=nil;collectgarbage('collect')"
  assert first in fixture
  (stage/'review.lua').write_text(fixture.replace(first,replacement))
 else: shutil.copy(root/'test/crossplay_ports'/name,stage/'review.lua')
 r=subprocess.run(['/tmp/crossplay-lua-runner','review.lua','76800'],cwd=stage,text=True,capture_output=True)
 row={'fixture':name,'stage':str(stage),'returncode':r.returncode,'stdout':r.stdout,'stderr':r.stderr};results.append(row);print(json.dumps(row),flush=True)
pathlib.Path('/root/github/.host-data/crosssmudge/crossplay-ports/notes-capacity-review-results.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(any(x['returncode'] for x in results))
