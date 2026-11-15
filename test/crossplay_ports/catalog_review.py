#!/usr/bin/env python3
"""Validate prepared catalog dependencies/paths. Remote bytes require parent push."""
import pathlib,json,hashlib,re,subprocess,tempfile,shutil
ROOT=pathlib.Path(__file__).resolve().parents[2]
OUT=pathlib.Path('/root/github/.host-data/crosssmudge/crossplay-ports')
PREFIX='https://raw.githubusercontent.com/tobiasweissert/CrossSmudge/feat/crossplay-lua-apps/apps/'
cat=json.loads((ROOT/'apps/catalog.json').read_text());ids=[];files=[];launches=[]
original=json.loads(subprocess.check_output(['git','show','HEAD:apps/catalog.json'],cwd=ROOT,text=True))
assert cat['apps'][:len(original['apps'])]==original['apps']
for entry in cat['apps']:
 assert entry['id'] not in ids;ids.append(entry['id'])
 if not entry['id'].startswith('cp-'):continue
 app=entry['id'];root=ROOT/'apps'/app;manifest=json.loads((root/'manifest.json').read_text())
 assert manifest['id']==app and manifest['version']==entry['version']
 paths=entry['files'];assert len(paths)==len(set(paths))
 assert set(paths)=={p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file()}
 assert {'main.lua','manifest.json','LICENSE-CrossPlay.txt','PARITY.md'}<=set(paths)
 with tempfile.TemporaryDirectory() as tmp:
  for rel in paths:
   p=pathlib.PurePosixPath(rel);assert not p.is_absolute() and '..' not in p.parts and '\\' not in rel
   assert not rel.endswith('.dat') and not (root/rel).is_symlink()
   data=(root/rel).read_bytes();dest=pathlib.Path(tmp)/rel;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
   if rel.endswith('.lua'):
    for dep in re.findall(r"dofile\(['\"]([^'\"]+)['\"]\)",data.decode()):assert dep in paths,(app,dep)
   files.append(dict(app=app,file=rel,install_path='/.crosssmudge/applications/'+app+'/'+rel,url=PREFIX+app+'/'+rel,bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),remote_verified=False))
  pathlib.Path(tmp,'__test.lua').write_text("dofile('main.lua');on_init();on_draw();on_exit()")
  r=subprocess.run(['/tmp/crossplay-lua-runner','__test.lua','76800'],cwd=tmp,text=True,capture_output=True)
  assert r.returncode==0,(app,r.stderr);launches.append(dict(app=app,stdout=r.stdout))
(OUT/'catalog-url-manifest.json').write_text(json.dumps(dict(prefix=PREFIX,remote_readback='Pending parent commit/push; URLs are prepared source mappings, not published-byte proof',files=files,install_launches=launches),indent=2)+'\n')
print(len(launches),'catalog-only installs launched;',len(files),'paths/hashes/URLs validated locally; remote readback pending push')
