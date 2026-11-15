/* Native test adapter, NOT the production renderer/HAL. Uses vendored Lua. */
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef struct {size_t used, peak, limit;} Budget;
static int width=480,height=800,draws=0;
static void *alloc(void *ud,void *p,size_t old,size_t n) {
  Budget *b=ud; if(!p) old=0;
  if(!n){free(p);b->used-=old;return NULL;}
  if(n>old && n-old>b->limit-b->used)return NULL;
  void *q=realloc(p,n);if(q){b->used=b->used-old+n;if(b->used>b->peak)b->peak=b->used;}return q;
}
static const char *path(lua_State *L) {
 const char *s=luaL_checkstring(L,1);
 if(*s=='/'||strstr(s,"..")||strchr(s,'\\'))luaL_error(L,"unsafe app path");
 return s;
}
static int readfile(lua_State *L){const char *s=path(L);size_t cap=luaL_optinteger(L,2,65536);FILE*f=fopen(s,"rb");if(!f){lua_pushnil(L);return 1;}
 fseek(f,0,SEEK_END);size_t n=ftell(f);rewind(f);if(n>cap)n=cap;
 luaL_Buffer b;char*p=luaL_buffinitsize(L,&b,n);size_t got=fread(p,1,n,f);fclose(f);luaL_pushresultsize(&b,got);return 1;}
static int writefile(lua_State *L){const char*s=path(L);size_t n;const char*p=luaL_checklstring(L,2,&n);FILE*f=fopen(s,lua_toboolean(L,3)?"ab":"wb");int ok=0;if(f){ok=fwrite(p,1,n,f)==n;ok=(fclose(f)==0)&&ok;}lua_pushboolean(L,ok);return 1;}
static int dofile_app(lua_State *L){const char*s=path(L);int base=lua_gettop(L);if(luaL_loadfile(L,s)||lua_pcall(L,0,LUA_MULTRET,0))return lua_error(L);return lua_gettop(L)-base;}
static int bounds(lua_State*L){lua_pushinteger(L,width);lua_pushinteger(L,height);return 2;}
static int metrics(lua_State*L){lua_createtable(L,0,4);lua_pushinteger(L,50);lua_setfield(L,-2,"header_height");lua_pushinteger(L,12);lua_setfield(L,-2,"top_padding");lua_pushinteger(L,40);lua_setfield(L,-2,"button_hints_height");lua_pushinteger(L,20);lua_setfield(L,-2,"content_side_padding");return 1;}
static int nothing(lua_State*L){(void)L;return 0;}
static int draw(lua_State*L){for(int i=1;i<=lua_gettop(L);i++)if(lua_type(L,i)==LUA_TNUMBER){double v=lua_tonumber(L,i);if(v!=v||v< -2000||v>2000)return luaL_error(L,"invalid drawing coordinate");}draws++;return 0;}
static int millis(lua_State*L){lua_pushinteger(L,123456);return 1;}
static int randomfn(lua_State*L){lua_Integer a=luaL_checkinteger(L,1),b=luaL_checkinteger(L,2);lua_pushinteger(L,a+rand()%(b-a+1));return 1;}
static int textwidth(lua_State*L){lua_pushinteger(L,strlen(luaL_checkstring(L,1))*10);return 1;}
static int unsupported(lua_State*L){return luaL_error(L,"unmodeled stock API called by test");}
static int yes(lua_State*L){lua_pushboolean(L,1);return 1;}
static const luaL_Reg api[]={
 {"get_bounds",bounds},{"get_metrics",metrics},{"read_file",readfile},{"write_file",writefile},{"dofile",dofile_app},
 {"clear",draw},{"text",draw},{"centered_text",draw},{"rect",draw},{"circle",draw},{"line",draw},{"header",draw},{"button_hints",draw},
 {"prevent_sleep",nothing},{"request_update",nothing},{"exit",nothing},{"full_refresh",nothing},{"log",nothing},{"millis",millis},{"time",millis},{"random",randomfn},{"text_width",textwidth},{"has_touch",yes},{NULL,NULL}};
int main(int argc,char**argv){if(argc<2)return 2;Budget b={0,0,76800};if(argc>2)b.limit=strtoul(argv[2],0,10);if(argc>4){width=atoi(argv[3]);height=atoi(argv[4]);}
 lua_State*L=lua_newstate(alloc,&b);if(!L)return 3;lua_gc(L,LUA_GCINC,105,250,0);
 const luaL_Reg libs[]={{"_G",luaopen_base},{"table",luaopen_table},{"string",luaopen_string},{"math",luaopen_math},{"os",luaopen_os},{NULL,NULL}};
 for(const luaL_Reg*r=libs;r->func;r++){luaL_requiref(L,r->name,r->func,1);lua_pop(L,1);}
 lua_newtable(L);
 const char*stock_names[]={"clear","refresh","text","centered_text","text_width","wrapped_text","line_height","pixel","line","thick_line","rect","rect_dither","rounded_rect","circle","heart","draw_sprite","sprite","bitmap","get_bounds","get_metrics","button_hints","header","save","load","read_file","find_section","find_file_section","read_lines","file_exists","exit","log","time","get_date","date","random","millis","get_time_ms","request_update","redraw","is_button_down","full_refresh","invert_rect","invert_screen","invert","triangle","set_orientation","get_orientation","prevent_sleep","keep_awake","is_prevent_sleep","is_keep_awake","in_rect","point_in_rect","has_touch","has_touchscreen","get_battery","battery","get_device","write_file","list_files","delete_file","popup","draw_popup","get_memory","memory","dofile","version","get_version","api_version","get_api_version","has_feature",NULL};
 for(int i=0;stock_names[i];i++){lua_pushcfunction(L,unsupported);lua_setfield(L,-2,stock_names[i]);}
 lua_pushinteger(L,2);lua_setfield(L,-2,"api_version");luaL_setfuncs(L,api,0);lua_setglobal(L,"smudge");lua_pushcfunction(L,dofile_app);lua_setglobal(L,"dofile");
 int err=luaL_loadfile(L,argv[1])||lua_pcall(L,0,0,0);if(err)fprintf(stderr,"FAIL: %s\n",lua_tostring(L,-1));
 lua_close(L);printf("{\"ok\":%s,\"peak_bytes\":%zu,\"limit_bytes\":%zu,\"after_close\":%zu,\"draw_calls\":%d}\n",err?"false":"true",b.peak,b.limit,b.used,draws);return err?1:0;}
