#ifndef CASC_FINDER_H
#define CASC_FINDER_H

#include <CascPort.h>
#include <lua.h>

struct CASC_Storage;

struct CASC_Finder
{
	HANDLE handle;
	const struct CASC_Storage *storage;
};

extern int
casc_finder_initialize (
	lua_State *L,
	const struct CASC_Storage *storage,
	const char *pattern,
	const int plain);

extern struct CASC_Finder *
casc_finder_access (
	lua_State *L,
	int index);

#endif
