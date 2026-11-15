#!/usr/bin/env python3
"""Lifecycle API checks for unchanged upstream apps using a labeled native adapter.
Not a renderer, real device, or full game-equivalence test. All writes are staged.
"""
import pathlib,subprocess,shutil,json,hashlib
ROOT=pathlib.Path(__file__).resolve().parents[2]
base=(ROOT/'test/crossplay_ports/runner.c').read_text()
extra=r'''
/* Stock persistence semantics, isolated to test working directory. */
static void savepath(lua_State*L,char out[160]){const char*k=luaL_checkstring(L,1);if(strlen(k)>100||strchr(k,'/')||strchr(k,'\\')||strstr(k,".."))luaL_error(L,"unsafe save key");snprintf(out,160,"__saved_%s.dat",k);}
static int savefn(lua_State*L){char s[160];savepath(L,s);const char*v=luaL_checkstring(L,2);FILE*f=fopen(s,"wb");if(f){fwrite(v,1,strlen(v),f);fclose(f);}return 0;}
static int loadfn(lua_State*L){char s[160];savepath(L,s);FILE*f=fopen(s,"rb");if(!f){if(lua_gettop(L)>=2)lua_pushvalue(L,2);else lua_pushnil(L);return 1;}fseek(f,0,SEEK_END);size_t n=ftell(f);rewind(f);luaL_Buffer b;char*p=luaL_buffinitsize(L,&b,n);size_t got=fread(p,1,n,f);fclose(f);luaL_pushresultsize(&b,got);return 1;}
static int memoryfn(lua_State*L){void*ud;lua_getallocf(L,&ud);Budget*b=ud;lua_createtable(L,0,2);lua_pushinteger(L,b->used/1024);lua_setfield(L,-2,"lua_kb");lua_pushinteger(L,b->limit/1024);lua_setfield(L,-2,"lua_max_kb");return 1;}
static int notdown(lua_State*L){luaL_checkstring(L,1);lua_pushboolean(L,0);return 1;}
static int spritefn(lua_State*L){int w=luaL_checkinteger(L,3),h=luaL_checkinteger(L,4);const char*p=luaL_checkstring(L,5);FILE*f=fopen(p,"rb");if(!f)return luaL_error(L,"missing sprite %s",p);fseek(f,0,SEEK_END);long n=ftell(f);fclose(f);if(n<((w+7)/8)*h)return luaL_error(L,"short sprite %s",p);return draw(L);}
'''
base=base.replace('static const luaL_Reg api[]={',extra+'\nstatic const luaL_Reg api[]={\n {"save",savefn},{"load",loadfn},{"get_memory",memoryfn},{"is_button_down",notdown},{"draw_sprite",spritefn},{"rounded_rect",draw},{"rect_dither",draw},{"thick_line",draw},')
source=pathlib.Path('/tmp/crossplay-upstream-adapter.c');source.write_text(base)
subprocess.run(['cc','-O2','-I'+str(ROOT/'lib/Lua/src'),str(source),*[str(p) for p in (ROOT/'lib/Lua/src').glob('*.c')],'-lm','-o','/tmp/crossplay-upstream-adapter'],check=True)
# Touch receives precisely stock signature, and no held buttons are modeled.
script='''assert(math.maxinteger==2147483647)
dofile('main.lua')
if on_init then on_init() end
if on_draw then on_draw() end
if on_button then on_button('confirm',true) end
if on_draw then on_draw() end
if on_tap then on_tap(100,200) end
if on_touch then on_touch(100,200,'tap') end
if on_button then on_button('right',true);on_button('down',true) end
if on_draw then on_draw() end
if on_exit then on_exit() end
print('upstream native adapter lifecycle OK; no HAL render claim')
'''
results=[]
for name in ['chess','sudoku','solitaire','wordle']:
 for cap in [76800,2097152]:
  stage=pathlib.Path('/tmp')/f'crossplay-upstream-{name}-{cap}'
  stage.mkdir(exist_ok=True)
  shutil.copytree(ROOT/'apps'/name,stage,dirs_exist_ok=True)
  # These are test-owned isolated save files, never user app saves.
  for f in stage.glob('__saved_*.dat'): f.unlink()
  (stage/'upstream-test.lua').write_text(script)
  for phase in ['fresh','resume']:
   r=subprocess.run(['/tmp/crossplay-upstream-adapter','upstream-test.lua',str(cap)],cwd=stage,text=True,capture_output=True)
   row={'app':name,'phase':phase,'cap':cap,'returncode':r.returncode,'stdout':r.stdout,'stderr':r.stderr}
   results.append(row);print(json.dumps(row),flush=True)
   if r.returncode: break
pathlib.Path('/root/github/.host-data/crosssmudge/crossplay-ports/upstream-runtime-results.json').write_text(json.dumps(results,indent=2)+'\n')
