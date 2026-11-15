/* Host-only regression runner; compile against this repo's 32-bit Lua. */
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct { size_t used, peak, limit; } Budget;
static void *allocate(void *ud, void *p, size_t old, size_t size) {
    Budget *b = ud;
    if (!p) old = 0;
    if (!size) { free(p); b->used -= old; return NULL; }
    if (size > old && size - old > b->limit - b->used) return NULL;
    void *next = realloc(p, size);
    if (next) {
        b->used = b->used - old + size;
        if (b->used > b->peak) b->peak = b->used;
    }
    return next;
}
int main(int argc, char **argv) {
    if (argc != 7 || sizeof(lua_Integer) != 4 || sizeof(lua_Number) != 4) return 2;
    Budget budget = {0, 0, strtoul(argv[6], NULL, 10)};
    lua_State *L = lua_newstate(allocate, &budget);
    if (!L) return 3;
    lua_gc(L, LUA_GCINC, 105, 250, 0);
    const luaL_Reg libs[] = {{"_G", luaopen_base}, {"table", luaopen_table},
        {"string", luaopen_string}, {"math", luaopen_math}, {"os", luaopen_os}, {NULL, NULL}};
    for (const luaL_Reg *r = libs; r->func; ++r) {
        luaL_requiref(L, r->name, r->func, 1);
        lua_pop(L, 1);
    }
    lua_pushinteger(L, atoi(argv[2])); lua_setglobal(L, "test_width");
    lua_pushinteger(L, atoi(argv[3])); lua_setglobal(L, "test_height");
    lua_pushstring(L, argv[4]); lua_setglobal(L, "test_case");
    lua_pushstring(L, argv[5]); lua_setglobal(L, "test_device");
    int err = luaL_loadfile(L, argv[1]) || lua_pcall(L, 0, 0, 0);
    if (err) fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_close(L);
    printf("%s %sx%s %s device=%s peak=%zu/%zu bytes; after_close=%zu\n",
           argv[4], argv[2], argv[3], err ? "FAIL" : "PASS", argv[5], budget.peak, budget.limit, budget.used);
    return err || budget.used != 0;
}
