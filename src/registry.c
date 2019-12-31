#include "registry.h"
#include "file.h"
#include "finder.h"
#include "storage.h"
#include <CascPort.h>
#include <compat-5.3.h>
#include <lauxlib.h>
#include <lua.h>

#define CASC_REGISTRY_METATABLE "Casc Registry"

extern void
casc_registry_open (
	lua_State *L,
	const struct CASC_Storage *storage)
{
	lua_newtable (L);

	if (luaL_newmetatable (L, CASC_REGISTRY_METATABLE))
	{
		lua_pushliteral (L, "v");
		lua_setfield (L, -2, "__mode");
	}

	lua_setmetatable (L, -2);
	lua_rawsetp (L, LUA_REGISTRYINDEX, storage->handle);
}

extern void
casc_registry_close (
	lua_State *L,
	const struct CASC_Storage *storage)
{
	lua_rawgetp (L, LUA_REGISTRYINDEX, storage->handle);
	lua_pushnil (L);

	while (lua_next (L, -2))
	{
		lua_getmetatable (L, -1);
		lua_getfield (L, -1, "__gc");
		lua_remove (L, -2);
		lua_insert (L, -2);
		lua_call (L, 1, 0);
	}

	lua_pop (L, 1);
	lua_pushnil (L);
	lua_rawsetp (L, LUA_REGISTRYINDEX, storage->handle);
}

static void
registry_insert (
	lua_State *L,
	HANDLE *storage,
	HANDLE *handle,
	int index)
{
	lua_pushvalue (L, index);
	lua_rawgetp (L, LUA_REGISTRYINDEX, storage);
	lua_insert (L, -2);
	lua_rawsetp (L, -2, handle);
	lua_pop (L, 1);
}

extern void
casc_registry_insert_file (
	lua_State *L,
	int index)
{
	const struct CASC_File *file = casc_file_access (L, index);
	registry_insert (L, file->storage->handle, file->handle, index);
}

extern void
casc_registry_insert_finder (
	lua_State *L,
	int index)
{
	const struct CASC_Finder *finder = casc_finder_access (L, index);
	registry_insert (L, finder->storage->handle, finder->handle, index);
}

static void
registry_remove (
	lua_State *L,
	HANDLE *storage,
	HANDLE *handle)
{
	lua_rawgetp (L, LUA_REGISTRYINDEX, storage);
	lua_pushnil (L);
	lua_rawsetp (L, -2, handle);
	lua_pop (L, 1);
}

extern void
casc_registry_remove_file (
	lua_State *L,
	const struct CASC_File *file)
{
	registry_remove (L, file->storage->handle, file->handle);
}

extern void
casc_registry_remove_finder (
	lua_State *L,
	const struct CASC_Finder *finder)
{
	registry_remove (L, finder->storage->handle, finder->handle);
}
