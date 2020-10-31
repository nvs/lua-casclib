#include "storage.h"
#include "common.h"
#include "file.h"
#include "finder.h"
#include "registry.h"
#include <CascLib.h>
#include <CascPort.h>
#include <compat-5.3.h>
#include <lauxlib.h>
#include <lua.h>
#include <string.h>

#define CASC_STORAGE_METATABLE "Casc Storage"

/**
 * `casc:files ([pattern [, plain]])`
 *
 * Returns an iterator `function` that, each time it is called, returns the
 * next file name (`string`) that matches `pattern` (`string`) (which is a
 * Lua pattern).  If `plain` (`boolean`) is specified, then pattern matching
 * is disabled and a plain text search is performed.  The default behavior,
 * should `pattern` be absent, is to return all files.
 *
 * In case of errors this function raises the error, instead of returning an
 * error code.
 */
static int
storage_files (lua_State *L)
{
	const struct CASC_Storage *storage = casc_storage_access (L, 1);

	if (!storage->handle)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		goto error;
	}

	const char *pattern = luaL_optstring (L, 2, NULL);
	const int plain = lua_toboolean (L, 3);

	lua_settop (L, 0);
	return casc_finder_initialize (L, storage, pattern, plain);

error:
	return casc_result (L, 0);
}

/**
 * `casc:open (name [, mode [, size]])`
 *
 * This function opens the file specified by `name` (`string`) within the
 * `casc` storage, with the specified `mode` (`string`), and returns a new
 * CASC File object.
 *
 * The `mode` can be any of the following, and must match exactly:
 *
 * - `"r"`: Read mode (the default).
 *
 * Additionally, `"b"` is accepted at the end of the mode, representing
 * binary mode.  However, it serves no actual purpose.
 *
 * In case of error, returns `nil`, a `string` describing the error, and
 * a `number` indicating the error code.
 */
static int
storage_open (lua_State *L)
{
	static const char * const
	modes [] = {
		"r",
		"rb",
	};

	const struct CASC_Storage *storage = casc_storage_access (L, 1);

	if (!storage->handle)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		goto error;
	}

	const char *name = luaL_checkstring (L, 2);
	luaL_checkoption (L, 3, "r", modes);

	lua_settop (L, 0);
	return casc_file_initialize (L, storage, name);

error:
	return casc_result (L, 0);
}

/**
 * `casc:close ()`
 *
 * Returns a `boolean` indicating that the `casc` storage, along with any of
 * its open files, was successfully closed.  Note that storages are
 * automatically closed when their handles are garbage collected.
 *
 * In case of error, returns `nil`, a `string` describing the error, and
 * a `number` indicating the error code.
 */
static int
storage_close (lua_State *L)
{
	struct CASC_Storage *storage = casc_storage_access (L, 1);
	int status = 0;

	if (!storage->handle)
	{
		SetLastError (ERROR_INVALID_HANDLE);
	}
	else
	{
		casc_registry_close (L, storage);
		status = CascCloseStorage (storage->handle);
		storage->handle = NULL;
	}

	return casc_result (L, status);
}

/**
 * `casc:__tostring ()`
 *
 * Returns a `string` representation of the `casc` storage, indicating
 * whether it is closed.
 */
static int
storage_to_string (lua_State *L)
{
	const struct CASC_Storage *storage = casc_storage_access (L, 1);
	const char *text = !storage->handle ? "%s (%p) (Closed)" : "%s (%p)";

	lua_pushfstring (L, text, CASC_STORAGE_METATABLE, storage);
	return 1;
}

static const luaL_Reg
storage_methods [] =
{
	{ "files", storage_files },
	{ "open", storage_open },
	{ "close", storage_close },
	{ "__tostring", storage_to_string },
	{ "__gc", storage_close },
	{ NULL, NULL }
};

static void
storage_metatable (lua_State *L)
{
	if (luaL_newmetatable (L, CASC_STORAGE_METATABLE))
	{
		luaL_setfuncs (L, storage_methods, 0);
		lua_pushvalue (L, -1);
		lua_setfield (L, -2, "__index");
	}

	lua_setmetatable (L, -2);
}

extern int
casc_storage_initialize (
	lua_State *L,
	const char *path,
	const int type)
{
	HANDLE handle;

	if (!CascOpenStorageEx (path, NULL, type, &handle))
	{
		goto error;
	}

	struct CASC_Storage *storage = lua_newuserdata (L, sizeof (*storage));
	storage->handle = handle;

	storage_metatable (L);
	casc_registry_open (L, storage);
	return 1;

error:
	return casc_result (L, 0);
}

extern struct CASC_Storage *
casc_storage_access (
	lua_State *L,
	int index)
{
	return luaL_checkudata (L, index, CASC_STORAGE_METATABLE);
}
