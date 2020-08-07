#include "finder.h"
#include "common.h"
#include "registry.h"
#include "storage.h"
#include <CascLib.h>
#include <CascPort.h>
#include <compat-5.3.h>
#include <lauxlib.h>
#include <lua.h>
#include <stddef.h>

#define CASC_FINDER_METATABLE "CASC Finder"

/**
 * `finder:__gc ()`
 *
 * Returns a `boolean` indicating that the CASC Finder was successfully
 * closed.
 *
 * In case of error, returns `nil`, a `string` describing the error, and
 * a `number` indicating the error code.
 */
static int
finder_close (lua_State *L)
{
	struct CASC_Finder *finder = casc_finder_access (L, 1);
	int status = 0;

	if (!finder->handle)
	{
		SetLastError (ERROR_INVALID_HANDLE);
	}
	else
	{
		casc_registry_remove_finder (L, finder);
		status = CascFindClose (finder->handle);
		finder->handle = NULL;
	}

	return casc_result (L, status);
}

/**
 * `finder:__tostring ()`
 *
 * Returns a `string` representation of the `Casc Finder` object,
 * indicating whether it is closed.
 */
static int
finder_to_string (lua_State *L)
{
	const struct CASC_Finder *finder = casc_finder_access (L, 1);
	const char *text = !finder->handle ? "%s (%p) (Closed)" : "%s (%p)";

	lua_pushfstring (L, text, CASC_FINDER_METATABLE, finder);
	return 1;
}

static int
finder_iterator (lua_State *L)
{
	struct CASC_Finder *finder =
		casc_finder_access (L, lua_upvalueindex (1));
	const char *pattern = luaL_optstring (L, lua_upvalueindex (2), NULL);
	const int plain = lua_toboolean (L, lua_upvalueindex (3));

	CASC_FIND_DATA data;
	int status;
	int error;
	int results = 0;

	while (true)
	{
		if (!finder->handle)
		{
			finder->handle = CascFindFirstFile (
				finder->storage->handle, "*", &data, NULL);
			status = !!finder;

			if (status)
			{
				casc_registry_insert_finder (L, lua_upvalueindex (1));
			}
		}
		else
		{
			status = CascFindNextFile (finder->handle, &data);
		}

		if (!status)
		{
			break;
		}

		if (pattern)
		{
			lua_getglobal (L, "string");
			lua_getfield (L, -1, "find");
			lua_remove (L, -2);

			lua_pushstring (L, data.szFileName);
			lua_pushstring (L, pattern);
			lua_pushnil (L);
			lua_pushboolean (L, plain);
			lua_call (L, 4, 1);

			status = !lua_isnil (L, -1);
			lua_pop (L, 1);
		}

		if (status)
		{
			lua_pushstring (L, data.szFileName);
			results = 1;
			break;
		}
	}

	if (!status)
	{
		error = GetLastError ();
		lua_settop (L, 0);
		lua_pushvalue (L, lua_upvalueindex (1));
		finder_close (L);
	}

	if (status || error == ERROR_SUCCESS)
	{
		return results;
	}

	SetLastError (error);
	results = casc_result (L, 0);

error:
	return luaL_error (L, "%s", lua_tostring (L, -results + 1));
}

static const luaL_Reg
finder_methods [] =
{
	{ "__tostring", finder_to_string },
	{ "__gc", finder_close },
	{ NULL, NULL }
};

static void
finder_metatable (lua_State *L)
{
	if (luaL_newmetatable (L, CASC_FINDER_METATABLE))
	{
		luaL_setfuncs (L, finder_methods, 0);
		lua_pushvalue (L, -1);
		lua_setfield (L, -2, "__index");
	}

	lua_setmetatable (L, -2);
}

extern int
casc_finder_initialize (
	lua_State *L,
	const struct CASC_Storage *storage,
	const char *pattern,
	const int plain)
{
	struct CASC_Finder *finder = lua_newuserdata (L, sizeof (*finder));
	finder->handle = NULL;
	finder->storage = storage;

	finder_metatable (L);

	lua_pushstring (L, pattern);
	lua_pushboolean (L, plain);
	lua_pushcclosure (L, finder_iterator, 3);
	return 1;
}

extern struct CASC_Finder *
casc_finder_access (
	lua_State *L,
	int index)
{
	return luaL_checkudata (L, index, CASC_FINDER_METATABLE);
}
