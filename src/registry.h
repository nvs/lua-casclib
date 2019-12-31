#ifndef CASC_REGISTRY_H
#define CASC_REGISTRY_H

#include <lua.h>

struct CASC_File;
struct CASC_Finder;
struct CASC_Storage;

extern void
casc_registry_open (
	lua_State *L,
	const struct CASC_Storage *storage);

extern void
casc_registry_close (
	lua_State *L,
	const struct CASC_Storage *storage);

extern void
casc_registry_insert_file (
	lua_State *L,
	int index);

extern void
casc_registry_insert_finder (
	lua_State *L,
	int index);

extern void
casc_registry_remove_file (
	lua_State *L,
	const struct CASC_File *file);

extern void
casc_registry_remove_finder (
	lua_State *L,
	const struct CASC_Finder *finder);

#endif
