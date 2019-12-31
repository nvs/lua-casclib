#ifndef CASC_FILE_H
#define CASC_FILE_H

#include <CascPort.h>
#include <lua.h>

struct CASC_Storage;

struct CASC_File
{
	HANDLE handle;
	const struct CASC_Storage *storage;
};

extern int
casc_file_initialize (
	lua_State *L,
	const struct CASC_Storage *storage,
	const char *name);

extern struct CASC_File *
casc_file_access (
	lua_State *L,
	int index);

#endif
