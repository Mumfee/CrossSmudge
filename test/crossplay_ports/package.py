#!/usr/bin/env python3
"""Build deterministic, non-overwriting SD/web ZIPs; read back every payload."""
import hashlib,json,pathlib,re,shutil,stat,subprocess,tempfile,zipfile
ROOT=pathlib.Path(__file__).resolve().parents[2]
OUT=pathlib.Path('/root/github/.host-data/crosssmudge/crossplay-ports')
DEST=OUT/'packages';DEST.mkdir(exist_ok=True)
STOCK='bae8322b7933fc46007682d2b2d13db7c157f729'
NEW=['cp-minesweeper','cp-connectfour','cp-knucklebones','cp-yahtzee','cp-calculator','cp-notes','cp-insider','cp-forehead','cp-checkers','cp-wavelength','cp-hearts','cp-hex','cp-battleship','cp-jaipur','cp-seasalt','cp-toybattle','cp-murdle','cp-go']
OLD=['chess','sudoku','solitaire','wordle']
source=subprocess.check_output(['git','show',STOCK+':src/activities/apps/lua/LuaRunner.cpp'],cwd=ROOT,text=True)
known=set(re.findall(r'lua_setfield\(L, -2, "([^"]+)"\)',source))
index={'stock_commit':STOCK,'new_count':len(NEW),'upstream_count':len(OLD),'packages':[],'readback':[]}
def digest(data):return hashlib.sha256(data).hexdigest()
def app_files(app,upstream=False):
 root=ROOT/'apps'/app;manifest=json.loads((root/'manifest.json').read_text())
 assert manifest['id']==app and re.fullmatch(r'[a-z0-9-]+',app)
 assert manifest.get('entry','main.lua')=='main.lua'
 files={}
 for path in sorted(root.rglob('*')):
  assert not path.is_symlink(),path
  if not path.is_file():continue
  rel=path.relative_to(root).as_posix()
  assert not rel.endswith('.dat'),'Runtime save accidentally included'
  files[rel]=path.read_bytes()
  if path.suffix=='.lua':
   used=set(re.findall(r'\bsmudge\.([A-Za-z_]+)',files[rel].decode()))
   assert used<=known,(app,used-known)
   for dep in re.findall(r"\bdofile\(['\"]([^'\"]+)['\"]\)",files[rel].decode()):assert (root/dep).is_file(),(app,dep)
 if upstream:
  # Every original runtime file must still be byte-identical to this checkout's git object.
  for rel,data in files.items():
   assert subprocess.check_output(['git','show','HEAD:apps/'+app+'/'+rel],cwd=ROOT)==data
  files['LICENSE-CrossSmudge.txt']=(ROOT/'LICENSE').read_bytes()
  files['UPSTREAM-NOTICE.txt']=('Unchanged upstream '+app+' from '+subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()+'.\nNot a new CrossPlay port. Install ONLY if this app ID is absent; do not overwrite\nexisting folders or saved data. Tested fresh/resume in a native API adapter at\nSticky 2 MiB, not physical hardware. See upstream-equivalents.md.\n'+('Stock touch callback is incompatible: use physical buttons.\n' if app=='solitaire' else '')).encode()
 return files
all_files={app:app_files(app,app in OLD) for app in NEW+OLD}
def writezip(name,entries,kind,apps):
 path=DEST/name
 with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for rel,data in sorted(entries.items()):
   p=pathlib.PurePosixPath(rel);assert not p.is_absolute() and '..' not in p.parts and '\\' not in rel
   info=zipfile.ZipInfo(rel,date_time=(2026,11,15,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16
   z.writestr(info,data)
 with zipfile.ZipFile(path) as z:
  assert len(z.namelist())==len(set(z.namelist()))==len(entries)
  assert z.testzip() is None
  for entry in z.infolist():
   assert not stat.S_ISLNK(entry.external_attr>>16)
   assert z.read(entry.filename)==entries[entry.filename]
 row={'file':name,'kind':kind,'apps':apps,'sha256':digest(path.read_bytes()),'bytes':path.stat().st_size,
      'entries':[{'path':p,'bytes':len(d),'sha256':digest(d)} for p,d in sorted(entries.items())]}
 index['packages'].append(row)
 return path
for app in NEW+OLD:
 entries={app+'/'+p:d for p,d in all_files[app].items()}
 writezip(app+('-upstream' if app in OLD else '')+'-stock-v1.6.5.3.zip',entries,'optional-upstream' if app in OLD else 'new-local-port',[app])
combined={'.crosssmudge/applications/'+app+'/'+p:d for app in NEW for p,d in all_files[app].items()}
combined_path=writezip('crossplay-local-eighteen-stock-v1.6.5.3-SD.zip',combined,'new-ports-SD-only',NEW)
full={'.crosssmudge/applications/'+app+'/'+p:d for app in NEW+OLD for p,d in all_files[app].items()}
full['INSTALL.txt']=b'Contains 18 new local adaptations and 4 unchanged optional equivalents. Copy only absent app folders; never overwrite existing apps or saves. Stock v1.6.5.3. Official store merge pending. See each PARITY.md and UPSTREAM-NOTICE.txt.\n'
writezip('crossplay-feasible-local-22-stock-v1.6.5.3-SD.zip',full,'full-feasible-local-SD',NEW+OLD)
# Execute actual single-app archive bytes, including rules and UI for additions.
for app in NEW+OLD:
 with tempfile.TemporaryDirectory(prefix='cp-package-readback-') as tmp:
  archive=DEST/(app+('-upstream' if app in OLD else '')+'-stock-v1.6.5.3.zip')
  with zipfile.ZipFile(archive) as z:z.extractall(tmp)
  root=pathlib.Path(tmp)/app
  scripts=[None]
  if app in NEW:
   scripts.append(ROOT/'test/crossplay_ports'/(app[3:]+'.lua'))
  for script in scripts:
   fixture=root/'__readback_test.lua'
   fixture.write_text(script.read_text() if script else "dofile('main.lua');if on_init then on_init() end;if on_draw then on_draw() end;if on_exit then on_exit() end")
   cap='2097152' if app in OLD else '76800'
   runner='/tmp/crossplay-upstream-adapter' if app in OLD else '/tmp/crossplay-lua-runner'
   r=subprocess.run([runner,fixture.name,cap],cwd=root,text=True,capture_output=True,timeout=120)
   index['readback'].append({'app':app,'test':script.name if script else 'launch','cap':int(cap),'returncode':r.returncode,'stdout':r.stdout,'stderr':r.stderr})
   assert r.returncode==0,(app,r.stderr)
(DEST/'package-index.json').write_text(json.dumps(index,indent=2)+'\n')
(DEST/'SHA256SUMS').write_text(''.join(row['sha256']+'  '+row['file']+'\n' for row in index['packages']))
print('Verified',len(index['packages']),'ZIPs;',len(NEW),'new;',len(OLD),'unchanged upstream;',len(index['readback']),'readback launches')
