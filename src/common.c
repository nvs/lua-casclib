#include "common.h"
#include <CascLib.h>
#include <CascPort.h>
#include <lua.h>
#include <string.h>

extern int
casc_result (
	lua_State* L,
	const int status)
{
	const DWORD error = GetCascError ();

	if (status || error == ERROR_SUCCESS)
	{
		lua_pushboolean (L, status);

		return 1;
	}

	lua_pushnil (L);
	lua_pushstring (L, strerror (error));
	lua_pushinteger (L, (lua_Integer) error);

	return 3;
}
