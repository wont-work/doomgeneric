#define _POSIX_C_SOURCE 200809L
#include <math.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#include "doomgeneric.h"
#include "doomkeys.h"

#define LUA_LIB
#include "luajit-2.1/lua.h"
#include "luajit-2.1/lauxlib.h"

uint32_t begin;
bool want_redraw = false;
bool initialized = false;

#define KEYQUEUE_SIZE 16
static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

//

unsigned char lua_to_doom_key(const char *key)
{
    if (strcmp(key, "right") == 0)
        return KEY_RIGHTARROW;
    if (strcmp(key, "left") == 0)
        return KEY_LEFTARROW;
    if (strcmp(key, "up") == 0)
        return KEY_UPARROW;
    if (strcmp(key, "down") == 0)
        return KEY_DOWNARROW;
    if (strcmp(key, "a") == 0)
        return KEY_STRAFE_L;
    if (strcmp(key, "d") == 0)
        return KEY_STRAFE_R;
    if (strcmp(key, "e") == 0)
        return KEY_USE;
    if (strcmp(key, "space") == 0)
        return KEY_FIRE;
    if (strcmp(key, "return") == 0)
        return KEY_ENTER;
    if (strcmp(key, "escape") == 0)
        return KEY_ESCAPE;
    if (strcmp(key, "tab") == 0)
        return KEY_TAB;
    if (strcmp(key, "lshift") == 0)
        return KEY_RSHIFT;

    printf("unhandled key: %s\n", key);
    return 0xff;
}

void add_key(int pressed, const char *luaKey)
{
    unsigned char key = lua_to_doom_key(luaKey);
    if (key == 0xff)
        return;

    s_KeyQueue[s_KeyQueueWriteIndex] = (pressed << 8) | key;
    s_KeyQueueWriteIndex++;
    s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

uint32_t get_ms()
{
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
    return ((spec.tv_sec * 1000) + (spec.tv_nsec / 1000000));
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

    do
    {
        res = nanosleep(&ts, &ts);
    } while (res && errno == EINTR);

    return res;
}

//

void DG_Init()
{
    begin = get_ms();
}

void DG_DrawFrame()
{
    want_redraw = true;
}

void DG_SleepMs(uint32_t ms)
{
    msleep(ms);
}

uint32_t DG_GetTicksMs()
{
    uint32_t now = get_ms();
    return now - begin;
}

int DG_GetKey(int *pressed, unsigned char *doomKey)
{
    if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex)
    {
        // key queue is empty
        return 0;
    }
    else
    {
        unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
        s_KeyQueueReadIndex++;
        s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

        *pressed = keyData >> 8;
        *doomKey = keyData & 0xFF;

        return 1;
    }

    return 0;
}

void DG_SetWindowTitle(const char *title) {}

//

LUALIB_API int doom_start(lua_State *L)
{
    if (initialized)
        return 1;
    initialized = true;

    char *argv[] = {};
    doomgeneric_Create(0, argv);
    return 1;
}

LUALIB_API int doom_tick(lua_State *L)
{
    doomgeneric_Tick();
    return 1;
}

LUALIB_API int doom_get_width(lua_State *L)
{
    lua_pushnumber(L, DOOMGENERIC_RESX);
    return 1;
}

LUALIB_API int doom_get_height(lua_State *L)
{
    lua_pushnumber(L, DOOMGENERIC_RESY);
    return 1;
}

LUALIB_API int doom_get_pixel_at(lua_State *L)
{
    int x = lua_tonumber(L, 1);
    int y = lua_tonumber(L, 2);

    lua_pushnumber(L, DG_ScreenBuffer[(y * DOOMGENERIC_RESX) + x]);
    return 1;
}

LUALIB_API int doom_get_want_redraw(lua_State *L)
{
    lua_pushboolean(L, want_redraw);
    if (want_redraw)
        want_redraw = false;
    return 1;
}

LUALIB_API int doom_press_key(lua_State *L)
{
    const char *key = lua_tostring(L, 1);
    add_key(1, key);

    return 1;
}

LUALIB_API int doom_release_key(lua_State *L)
{
    const char *key = lua_tostring(L, 1);
    add_key(0, key);

    return 1;
}

static const luaL_Reg doomlib[] = {
    {"start", doom_start},
    {"tick", doom_tick},
    {"get_pixel_at", doom_get_pixel_at},
    {"get_width", doom_get_width},
    {"get_height", doom_get_height},
    {"get_want_redraw", doom_get_want_redraw},
    {"press_key", doom_press_key},
    {"release_key", doom_release_key},
    {NULL, NULL}};

LUALIB_API int luaopen_doom(lua_State *L)
{
    printf("Registering native doom module\n");
    luaL_register(L, "doom", doomlib);
    return 1;
}