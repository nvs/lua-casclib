#include "file.h"
#include "common.h"
#include "registry.h"
#include "storage.h"
#include <CascLib.h>
#include <CascPort.h>
#include <compat-5.3.h>
#include <lauxlib.h>
#include <luaconf.h>
#include <lua.h>
#include <lualib.h>
#include <string.h>

#define CASC_FILE_METATABLE "Casc File"

/**
 * `file:seek ([whence [, offset]])`
 *
 * Sets and gets the file position, measured from the beginning of the file,
 * to the position given by `offset` (`number`) plus a base specified by
 * `whence` (`string`), as follows:
 *
 * - `"set"`: Base is position `0` (i.e. the beginning of the file).
 * - `"cur"`: Base is the current position.
 * - `"end"`: Base is the end of the file.  For writable files, this is the
 *   last written position (not the absolute file size).
 *
 * In case of success, this function returns the final file position,
 * measured in bytes from the beginning of the file.  Otherwise, it returns
 * `nil`, a `string` describing the error, and a `number` indicating the
 * error code.
 *
 * The default value for `whence` is `"cur"`, and for offset is `0`.
 * Therefore, the call `file:seek ()` returns the current file position,
 * without changing it; the call `file:seek ('set')` sets the position to
 * the beginning of the file (and returns `0`); and the call `file:seek
 * ('end')` sets the position to the end of the file, and returns its size.
 */
static int
file_seek (lua_State *L)
{
	static const int
	modes [] = {
		FILE_BEGIN,
		FILE_CURRENT,
		FILE_END
	};

	static const char * const
	mode_options [] = {
		"set",
		"cur",
		"end",
		NULL
	};

	const struct CASC_File *file = casc_file_access (L, 1);
	const int option = luaL_checkoption (L, 2, "cur", mode_options);
	const int mode = modes [option];
	const lua_Integer offset = luaL_optinteger (L, 3, 0);

	if (!file->handle)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		goto error;
	}

	ULONGLONG position;

	if (!CascSetFilePointer64 (file->handle, offset, &position, mode))
	{
		goto error;
	}

	lua_pushinteger (L, (lua_Integer) position);
	return 1;

error:
	return casc_result (L, 0);
}

static int
read_line (
	lua_State *L,
	const struct CASC_File *file,
	int chop)
{
	luaL_Buffer line;
	luaL_buffinit (L, &line);

	DWORD bytes_read;
	char character = '\0';
	int status = 1;
	int error;

	while (status && character != '\n')
	{
		char *buffer = luaL_prepbuffer (&line);
		int index = 0;

		while (index < LUAL_BUFFERSIZE)
		{
			status = CascReadFile (
				file->handle, &character, 1, &bytes_read);
			error = GetLastError ();

			if (!status || bytes_read == 0 || character == '\n')
			{
				status = 0;
				break;
			}

			buffer [index++] = character;
		}

		luaL_addsize (&line, index);
	}

	if (!chop && character == '\n')
	{
		luaL_addchar (&line, character);
	}

	luaL_pushresult (&line);
	SetLastError (error);
	return status;
}

static int
read_characters (
	lua_State *L,
	const struct CASC_File *file,
	lua_Unsigned count)
{
	luaL_Buffer characters;
	luaL_buffinit (L, &characters);

	DWORD bytes_to_read;
	DWORD bytes_read;
	int status = 1;
	int error = ERROR_SUCCESS;

	while (count > 0 && status)
	{
		bytes_to_read = count > LUAL_BUFFERSIZE ? LUAL_BUFFERSIZE : count;
		count -= bytes_to_read;
		char *buffer = luaL_prepbuffsize (&characters, bytes_to_read);

		status = CascReadFile (
			file->handle, buffer, bytes_to_read, &bytes_read);
		error = GetLastError ();

		if (!status || bytes_read == 0)
		{
			status = 0;
			break;
		}

		luaL_addsize (&characters, bytes_read);
	}

	luaL_pushresult (&characters);
	SetLastError (error);
	return status;
}

/**
 * `file:read (...)`
 *
 * Reads the file, according to the given formats, which specify what to
 * read.  For each format, the function returns a `string` with the
 * characters read, or `nil` if it cannot read data.  In this latter case,
 * the function does not return subsequent formats.  When called without
 * formats, it uses a default format that reads the next line.
 *
 * The available formats are either a `string` or `number`:
 *
 * - `"a"`: Reads the whole file, starting at the current position.  On end
 *   of file, it returns the empty string.
 * - `"l"`: Reads the next line, skipping the end of line, returning `nil`
 *   on end of file.  This is the default format.
 * - `"L"`: Reads the next line, keeping the end of line character (if
 *   present), returning `nil` on end of file.
 * - `number`: Reads a string with up to this many bytes, returning `nil`
 *   on end of file.  If `number` is zero, it reads nothing and returns an
 *   empty string, or `nil` on end of file.
 *
 * The formats `"l"` and `"L"` should only be used for text files.
 *
 * In case of error, returns `nil`, a `string` describing the error, and
 * a `number` indicating the error code.
 */
static int
file_read (lua_State *L)
{
	const struct CASC_File *file = casc_file_access (L, 1);
	ULONGLONG size;

	if (!file->handle)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		goto error;
	}

	if (!CascGetFileSize64 (file->handle, &size))
	{
		goto error;
	}

	int index = 1;
	int arguments = lua_gettop (L) - index++;

	/* By default, read a line (with no line break) */
	if (arguments == 0)
	{
		arguments++;
		lua_pushliteral (L, "l");
	}

	/* Ensure stack space for all results and the buffer. */
	luaL_checkstack (L, arguments + LUA_MINSTACK, "too many arguments");

	SetLastError (ERROR_SUCCESS);
	int status = 1;

	for (; arguments-- && status; index++)
	{
		if (lua_type (L, index) == LUA_TNUMBER)
		{
			const lua_Integer count = luaL_checkinteger (L, index);
			status = read_characters (L, file, count);

			continue;
		}

		const char *format = luaL_checkstring (L, index);

		if (*format == '*')
		{
			format++;
		}

		switch (*format)
		{
			case 'l':
			{
				status = read_line (L, file, 1);
				break;
			}

			case 'L':
			{
				status = read_line (L, file, 0);
				break;
			}

			case 'a':
			{
				read_characters (L, file, (lua_Unsigned) size);
				break;
			}

			default:
			{
				return luaL_argerror (L, index, "invalid format");
			}
		}
	}

	if (!status)
	{
		if (GetLastError () != ERROR_SUCCESS)
		{
			goto error;
		}

		/*
		 * Note that compat-5.3 simply defines `lua_rawlen` as a macro to
		 * `lua_objlen` for Lua 5.1, despite the behavior not being
		 * analogous for `number`.  Ensure that only `string` is at the top
		 * of the stack to avoid this issue.
		 */
		if (lua_rawlen (L, -1) == 0)
		{
			lua_pop (L, 1);
			lua_pushnil (L);
		}
	}

	return index - 2;

error:
	return casc_result (L, 0);
}

static int
lines_iterator (lua_State *L)
{
	const struct CASC_File *file =
		casc_file_access (L, lua_upvalueindex (1));
	int results = 0;

	if (!file->handle)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		goto error;
	}

	lua_settop (L, 0);
	lua_pushvalue (L, lua_upvalueindex (1));

	const int arguments = (int) lua_tointeger (L, lua_upvalueindex (2));
	luaL_checkstack (L, arguments, "too many arguments");

	for (int index = 1; index <= arguments; index++)
	{
		lua_pushvalue (L, lua_upvalueindex (2 + index));
	}

	results = file_read (L);

	/* If the first result is not `nil`, return all results. */
	if (lua_toboolean (L, -results))
	{
		return results;
	}

error:

	/* A lack of results implies we did not attempt to read the file. */
	if (results == 0)
	{
		results = casc_result (L, 0);
	}

	/* Is there error information? */
	if (results > 1)
	{
		return luaL_error (L, "%s", lua_tostring (L, -results + 1));
	}

	/* Otherwise, this should only mean EOF. */
	return 0;
}

/*
 * This plus the number of upvalues used must be less than the maximum
 * number of upvalues to a C function (i.e. `255`).
 */
#define LINES_MAXIMUM_ARGUMENTS 250

/**
 * `file:lines (...)`
 *
 * Returns an interator `function` that, each time it is called, reads the
 * file according to the given formats.  When no format is given, uses `"l"`
 * as a default.  For details on the available formats, see `file:read ()`.
 *
 * In case of errors this function raises the error, instead of returning an
 * error code.
 */
static int
file_lines (lua_State *L)
{
	const struct CASC_File *file = casc_file_access (L, 1);

	if (!file->handle)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		goto error;
	}

	const int arguments = lua_gettop (L) - 1;

	luaL_argcheck (L, arguments <= LINES_MAXIMUM_ARGUMENTS,
		LINES_MAXIMUM_ARGUMENTS + 1, "too many arguments");

	lua_pushinteger (L, arguments);
	lua_insert (L, 2);

	lua_pushcclosure (L, lines_iterator, 2 + arguments);
	return 1;

error:
	return casc_result (L, 0);
}

/**
 * `file:write (...)`
 *
 * Returns `nil`.  A CASC `file` is not available for writing.
 *
 * This function exists to maintain consistency with Lua's I/O library.
 *
 * Returns `nil`, a `string` describing the error, and a `number` indicating
 * the error code.
 */
static int
file_write (lua_State *L)
{
	casc_file_access (L, 1);
	SetLastError (ERROR_INVALID_HANDLE);
	return casc_result (L, 0);
}

/**
 * `file:setvbuf ()`
 *
 * Returns `true`.  The buffering mode when writing cannot be altered.
 *
 * This function exists to maintain consistency with Lua's I/O library.
 *
 * In case of error, returns `nil`, a `string` describing the error, and
 * a `number` indicating the error code.
 */
static int
file_setvbuf (lua_State *L)
{
	struct CASC_File *file = casc_file_access (L, 1);
	int status = 1;

	if (!file->handle)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		status = 0;
	}

	return casc_result (L, status);
}

/**
 * `file:flush ()`
 *
 * Returns `true`.  A CASC `file` is not available for writing.
 *
 * This function exists to maintain consistency with Lua's I/O library.
 *
 * In case of error, returns `nil`, a `string` describing the error, and
 * a `number` indicating the error code.
 */
static int
file_flush (lua_State *L)
{
	struct CASC_File *file = casc_file_access (L, 1);
	int status = 1;

	if (!file->handle)
	{
		SetLastError (ERROR_INVALID_HANDLE);
		status = 0;
	}

	return casc_result (L, status);
}

/**
 * `file:close ()`
 *
 * Returns a `boolean` indicating that the file was successfully closed.
 * Note that files are automatically closed when their handles are garbage
 * collected or when the archive they belong to is closed.
 *
 * In case of error, returns `nil`, a `string` describing the error, and
 * a `number` indicating the error code.
 */
static int
file_close (lua_State *L)
{
	struct CASC_File *file = casc_file_access (L, 1);
	int status = 0;

	if (!file->handle)
	{
		SetLastError (ERROR_INVALID_HANDLE);
	}
	else
	{
		casc_registry_remove_file (L, file);
		status = CascCloseFile (file->handle);
		file->storage = NULL;
	}

	file->handle = NULL;

	return casc_result (L, status);
}

/**
 * `file:__tostring ()`
 *
 * Returns a `string` representation of the `Casc File` object, indicating
 * whether it is closed, open for writing, or open for reading.
 */
static int
file_to_string (lua_State *L)
{
	const struct CASC_File *file = casc_file_access (L, 1);
	const char *text = !file->handle ? "%s (%p) (Closed)" : "%s (%p)";

	lua_pushfstring (L, text, CASC_FILE_METATABLE, file);
	return 1;
}

static const luaL_Reg
file_methods [] =
{
	{ "seek", file_seek },
	{ "read", file_read },
	{ "lines", file_lines },
	{ "write", file_write },
	{ "setvbuf", file_setvbuf },
	{ "flush", file_flush },
	{ "close", file_close },
	{ "__tostring", file_to_string },
	{ "__gc", file_close },
	{ NULL, NULL }
};

static void
file_metatable (lua_State *L)
{
	if (luaL_newmetatable (L, CASC_FILE_METATABLE))
	{
		luaL_setfuncs (L, file_methods, 0);
		lua_pushvalue (L, -1);
		lua_setfield (L, -2, "__index");
	}

	lua_setmetatable (L, -2);
}

extern int
casc_file_initialize (
	lua_State *L,
	const struct CASC_Storage *storage,
	const char *name)
{
	HANDLE handle;

	if (!CascOpenFile (storage->handle, name, 0, 0, &handle))
	{
		goto error;
	}

	struct CASC_File *file = lua_newuserdata (L, sizeof (*file));
	file->handle = handle;
	file->storage = storage;

	file_metatable (L);
	casc_registry_insert_file (L, -1);

	return 1;

error:
	return casc_result (L, 0);
}

extern struct CASC_File *
casc_file_access (
	lua_State *L,
	int index)
{
	return luaL_checkudata (L, index, CASC_FILE_METATABLE);
}

