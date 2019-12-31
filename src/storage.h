#ifndef CASC_STORAGE_H
#define CASC_STORAGE_H

#include <CascPort.h>
#include <lua.h>

struct CASC_Storage
{
	HANDLE handle;
};

extern int
casc_storage_initialize (
	lua_State* L,
	const char *path,
	const int type);

extern struct CASC_Storage *
casc_storage_access (
	lua_State *L,
	int index);

#endif
