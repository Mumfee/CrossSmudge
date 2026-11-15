#!/usr/bin/env python3
import pathlib,subprocess,shutil,json,re,random
root=pathlib.Path(__file__).resolve().parents[2];source=pathlib.Path('/root/github/crossplay');dec=source/'lib/decNumber/src'
for unit in ['decnumber_impl','deccontext_impl']:
 subprocess.run(['cc','-std=c99','-O2','-I'+str(dec),'-c',str(dec/(unit+'.c')),'-o','/tmp/crossplay-'+unit+'.o'],check=True)
subprocess.run(['c++','-std=c++17','-O2','-I'+str(source/'src/apps_local/calculator'),'-I'+str(dec),str(pathlib.Path(__file__).with_name('calculator_review.cpp')),'/tmp/crossplay-decnumber_impl.o','/tmp/crossplay-deccontext_impl.o','-o','/tmp/crossplay-calculator-review'],check=True)
tests=(source/'host-tests/calculator/test_calculator.cpp').read_text()
cases=list(dict.fromkeys(re.findall(r'run\(e, "([^"]*)"\)',tests)))
cases.extend(['2+3===','200+10%+5=','0.00000000001','9999999999x9999999999=%%.n','1/10'+'='*110,'1x10'+'='*110,'1/0E5','0/0C3'])
rng=random.Random(4291)
for _ in range(200):cases.append(''.join(rng.choice('0123456789...++--xx//===%nCE<') for _ in range(50)))
for _ in range(200):
 a=str(rng.randrange(1,9999999999));b=str(rng.randrange(1,9999999999));c=str(rng.randrange(1,9999999999))
 if rng.randrange(2): a=a[:3]+'.'+a[3:]
 if rng.randrange(2): b=b[:2]+'.'+b[2:]
 cases.append(a+rng.choice(['+','-','x','/'])+b+'='+rng.choice(['+','-','x','/'])+c+'=')
cases.extend(['1/10'+'='*120,'1/3='+'/10'+'='*110,'1/7='+'/10'+'='*110,'1x10'+'='*105])
expected=subprocess.check_output(['/tmp/crossplay-calculator-review'],input='\n'.join(cases)+'\n',text=True).splitlines()
assert len(expected)==len(cases)
p=pathlib.Path('/tmp/crossplay-calculator-review-stage');p.mkdir(exist_ok=True)
for name in ['core.lua','decimal.lua']:shutil.copy(root/'apps/cp-calculator'/name,p/name)
header="local C=dofile('core.lua');local g=C.new();local map={x='*',n='+/-',C='AC',E='CE',['<']='BS'};local expect="
results=[]
for i,(seq,want) in enumerate(zip(cases,expected)):
 sequence=seq.replace(' ','');values=want.split('|')
 # JSON quoted ASCII strings are valid Lua strings for this fixture alphabet.
 fixture=header+'{'+','.join(json.dumps(v) for v in values)+'};local seq='+json.dumps(sequence)+'''\nfor i=1,#seq do local k=seq:sub(i,i);C.press(g,map[k] or k);local got=C.display(g);assert(got==expect[i], 'key '..i..' ('..k..') expected '..expect[i]..' got '..got) end\n'''
 (p/'test.lua').write_text(fixture)
 r=subprocess.run(['/tmp/crossplay-lua-runner','test.lua','76800'],cwd=p,text=True,capture_output=True)
 row={'case':i,'sequence':seq,'returncode':r.returncode,'stdout':r.stdout,'stderr':r.stderr};results.append(row)
 if r.returncode and sum(x['returncode']!=0 for x in results)<=15:print(json.dumps(row),flush=True)
pathlib.Path('/root/github/.host-data/crosssmudge/crossplay-ports/calculator-review-results.json').write_text(json.dumps(results,indent=2)+'\n')
print('calculator source cases',len(results),'passed',sum(x['returncode']==0 for x in results))
raise SystemExit(any(x['returncode'] for x in results))
