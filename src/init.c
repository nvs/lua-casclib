#include "storage.h"
#include <compat-5.3.h>
#include <lauxlib.h>
#include <lua.h>
#include <stddef.h>

/**
 * `casclib.open (path [, type])`
 *
 * This function opens the CASC storage specified by `path` (`string`) as
 * the specified `type` (`string`).
 *
 * The `type` can be any of the following, and must match exactly:
 *
 * - `"local"`: Opens a local storage.
 * - `"online"`: Opens an online storage.
 *
 * In case of success, this function returns a new `Casc Storage` object.
 * Otherwise, it returns `nil`, a `string` describing the error, and a
 * `number` indicating the error code.
 */
static int
casc_open (lua_State *L)
{
	static const char * const
	types [] = {
		"local",
		"online",
		NULL
	};

	const char *path = luaL_checkstring (L, 1);
	const int type = luaL_checkoption (L, 2, "local", types);

	lua_settop (L, 0);
	return casc_storage_initialize (L, path, type);
}

static const luaL_Reg
casc_functions [] =
{
	{ "open", casc_open },
	{ NULL, NULL }
};

extern int
luaopen_casclib (lua_State *L)
{
	luaL_newlib (L, casc_functions);
	return 1;
}
