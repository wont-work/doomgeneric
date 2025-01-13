#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define VC_EXTRALEAN
	#include <Windows.h>
#else
	#include <sys/time.h>
	#include <unistd.h>
#endif

#include "doomgeneric.h"
#include "doomkeys.h"

#define LUA_LIB
#ifdef _WIN32
	#include <luajit/lua.h>
	#include <luajit/lauxlib.h>
#else
	#include <lua.h>
	#include <lauxlib.h>
#endif

uint32_t begin;
bool want_redraw = false;
bool initialized = false;
char *doom_argv[] = {"", "-iwad\0", ""};

#define KEYQUEUE_SIZE 64
static unsigned short key_queue[KEYQUEUE_SIZE];
static unsigned int key_queue_write_idx = 0;
static unsigned int key_queue_read_idx = 0;

lua_State* callback_quit_L = NULL;
int callback_quit = LUA_NOREF;

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

static void add_key(int pressed, const char *luaKey)
{
    unsigned char key = lua_to_doom_key(luaKey);
    if (key == 0xff)
        return;

    key_queue[key_queue_write_idx] = (pressed << 8) | key;
    key_queue_write_idx++;
    key_queue_write_idx %= KEYQUEUE_SIZE;
}

static uint32_t get_ms()
{
#ifdef _WIN32
    return GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
#endif
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
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

uint32_t DG_GetTicksMs()
{
    uint32_t now = get_ms();
    return now - begin;
}

int DG_GetKey(int *out_pressed, unsigned char *out_doom_key)
{
    if (key_queue_read_idx == key_queue_write_idx)
    {
        // key queue is empty
        return 0;
    }
    else
    {
        unsigned short keyData = key_queue[key_queue_read_idx];
        key_queue_read_idx++;
        key_queue_read_idx %= KEYQUEUE_SIZE;

        *out_pressed = keyData >> 8;
        *out_doom_key = keyData & 0xFF;

        return 1;
    }

    return 0;
}

void DG_SetWindowTitle(const char *title) {}

void DG_Quit()
{
    if (callback_quit == LUA_NOREF || callback_quit_L == NULL) {
		printf("DG_Quit: no quit callback registered\n");
        return;
    }

    printf("DG_Quit\n");
    lua_rawgeti(callback_quit_L, LUA_REGISTRYINDEX, callback_quit);
    lua_call(callback_quit_L, 0, 0);
}

//

LUALIB_API int doom_start(lua_State *L)
{
    if (initialized)
        return 1;
    initialized = true;

    doom_argv[2] = strdup(lua_tostring(L, 1));
    doomgeneric_Create(3, doom_argv);
    return 0;
}

LUALIB_API int doom_tick(lua_State *L)
{
    doomgeneric_Tick();
    return 0;
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
    lua_Integer x = luaL_checkinteger(L, 1);
    lua_Integer y = luaL_checkinteger(L, 2);

    pixel_t packed = DG_ScreenBuffer[(y * DOOMGENERIC_RESX) + x];
    lua_Number b = ( packed        & 0xff) / 255.0;
    lua_Number g = ((packed >> 8)  & 0xff) / 255.0;
    lua_Number r = ((packed >> 16) & 0xff) / 255.0;

    lua_pushnumber(L, r);
    lua_pushnumber(L, g);
    lua_pushnumber(L, b);
    return 3;
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
    const char *key = luaL_checkstring(L, 1);
    add_key(1, key);

    return 0;
}

LUALIB_API int doom_release_key(lua_State *L)
{
    const char *key = luaL_checkstring(L, 1);
    add_key(0, key);

    return 0;
}

LUALIB_API int doom_on_quit(lua_State* L)
{
    luaL_argcheck(L, lua_isfunction(L, 1), 1, "invalid quit callback");

    if (callback_quit != LUA_NOREF && callback_quit_L != NULL) {
        luaL_unref(callback_quit_L, LUA_REGISTRYINDEX, callback_quit);
    }

	callback_quit = luaL_ref(L, LUA_REGISTRYINDEX);
	callback_quit_L = L;

    return 0;
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
    {"on_quit", doom_on_quit},
    {NULL, NULL}};

LUALIB_API int luaopen_doom(lua_State *L)
{
    printf("Registering native doom module\n");
    luaL_register(L, "doom", doomlib);

    return 0;
}