#include <stdio.h>
#include <stdlib.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
_Static_assert(sizeof(lua_Integer) == 4, "stock int32 required");
_Static_assert(sizeof(lua_Number) == 4, "stock float32 required");
typedef struct { size_t used, peak; } Budget;
static void *alloc(void *ud, void *ptr, size_t old, size_t size) {
    Budget *b = ud;
    if (!ptr) old = 0;
    if (!size) { free(ptr); b->used -= old; return NULL; }
    if (b->used - old + size > 2 * 1024 * 1024) return NULL;
    void *p = realloc(ptr, size);
    if (p) { b->used = b->used - old + size; if (b->used > b->peak) b->peak = b->used; }
    return p;
}
int main(int argc, char **argv) {
    if (argc != 5) return 2;
    Budget b = {0, 0};
    lua_State *L = lua_newstate(alloc, &b);
    if (!L) return 2;
    luaL_openlibs(L);
    const char *names[] = {"SOURCE", "DEVICE", "MODE"};
    for (int i = 0; i < 3; ++i) { lua_pushstring(L, argv[i+2]); lua_setglobal(L, names[i]); }
    int result = luaL_dofile(L, argv[1]);
    if (result) fprintf(stderr, "%s\n", lua_tostring(L, -1));
    fprintf(stderr, "Lua int32/float32 peak=%zu budget=2097152\n", b.peak);
    lua_close(L);
    return result ? 1 : 0;
}
