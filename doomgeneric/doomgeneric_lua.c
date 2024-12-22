#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

#include "doomgeneric.h"
#include "doomkeys.h"

#define LUA_LIB
#include "luajit-2.1/lua.h"
#include "luajit-2.1/lauxlib.h"

uint32_t begin;
bool want_redraw = false;

LUALIB_API int doom_start(lua_State *L)
{
    char *argv[] = {};
    doomgeneric_Create(0, argv);
    return 1;
}

LUALIB_API int doom_tick(lua_State* L)
{
    doomgeneric_Tick();
    return 1;
}

LUALIB_API int doom_get_width(lua_State* L)
{
    lua_pushnumber(L, DOOMGENERIC_RESX);
    return 1;
}

LUALIB_API int doom_get_height(lua_State* L)
{
    lua_pushnumber(L, DOOMGENERIC_RESY);
    return 1;
}

LUALIB_API int doom_get_pixel_at(lua_State* L)
{
    int x = lua_tonumber(L, 1);
    int y = lua_tonumber(L, 2);

    lua_pushnumber(L, DG_ScreenBuffer[(y * DOOMGENERIC_RESX) + x]);
    return 1;
}

LUALIB_API int doom_get_want_redraw(lua_State* L)
{
    lua_pushboolean(L, want_redraw);
    if (want_redraw) want_redraw = false;
    return 1;
}

static const luaL_Reg doomlib[] = {
    {"start", doom_start},
    {"tick", doom_tick},
    {"get_pixel_at", doom_get_pixel_at},
    {"get_width", doom_get_width},
    {"get_height", doom_get_height},
    {"get_want_redraw", doom_get_want_redraw},
    {NULL, NULL}};

LUALIB_API int luaopen_doom(lua_State* L)
{
    luaL_register(L, "doom", doomlib);
    return 1;
}

//

uint32_t get_ms() {
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    return floor(spec.tv_nsec / 1.0e6);
}

int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

//

void DG_Init() {
    begin = get_ms();
}

void DG_DrawFrame() {
    want_redraw = true;
}

void DG_SleepMs(uint32_t ms) {
    msleep(ms);
}

uint32_t DG_GetTicksMs() {
    uint32_t now = get_ms();
    return now - begin;
}

int DG_GetKey(int* pressed, unsigned char* key) {
    return 0;
}

void DG_SetWindowTitle(const char * title) {
    printf("TITLE: %s\n", title);
}