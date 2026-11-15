#!/usr/bin/env python3
import pathlib,subprocess,shutil,json
root=pathlib.Path(__file__).resolve().parents[2];source=pathlib.Path('/root/github/crossplay/src/apps_local')
subprocess.run(['c++','-std=c++17','-O2','-I'+str(source),str(pathlib.Path(__file__).with_name('notes_review.cpp')),str(source/'notes/NotesCore.cpp'),'-o','/tmp/crossplay-notes-review'],check=True)
s=subprocess.check_output(['/tmp/crossplay-notes-review'],text=True).splitlines();header=s[0];chunks=[];case=[]
for line in s[1:]:
 if line.startswith('s=') and case:chunks.append(case);case=[]
 if not line.startswith('print('):case.append(line)
if case:chunks.append(case)
p=pathlib.Path('/tmp/crossplay-notes-review-stage');p.mkdir(exist_ok=True);shutil.copy(root/'apps/cp-notes/core.lua',p/'core.lua')
results=[]
for i,lines in enumerate(chunks):
 intentional=(i==10)
 if intentional:
  # Deliberate byte-preservation difference authorized by the parent reviewer:
  # original drops a standalone terminal CR, Lua retains it.
  lines=["local s='standalone\\r'; assert(N.clear(s)==s);assert(N.strip(s)==s);assert(N.tolist(s)=='- [ ] '..s)"]
 (p/'test.lua').write_text(header+'\n'+'\n'.join(lines)+'\n')
 r=subprocess.run(['/tmp/crossplay-lua-runner','test.lua','76800'],cwd=p,text=True,capture_output=True)
 results.append({'case':i,'intentional_terminal_cr_preservation':intentional,'returncode':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
 print(i,r.returncode,r.stderr.strip())
pathlib.Path('/root/github/.host-data/crosssmudge/crossplay-ports/notes-review-results.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(any(x['returncode'] for x in results))
