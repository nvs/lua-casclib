# lua-casclib

[![License](https://img.shields.io/github/license/nvs/lua-casclib)](LICENSE)

## Contents

- [Overview](#overview)
- [Installation](#installation)
- [Documentation](#documentation)
- [Examples](#examples)

## Overview

**lua-casclib** provides a [Lua] binding to [CascLib].  It attempts to
adhere to the interface provided by Lua's [I/O] library.

[Lua]: https://www.lua.org
[CascLib]: https://github.com/ladislav-zezula/CascLib
[I/O]: https://www.lua.org/manual/5.3/manual.html#6.8

## Installation

The following two dependencies must be met to utilize this library:

- [CascLib] `=> 2.0`
- [Lua] `>= 5.1` or [LuaJIT] `>= 2.0`

The easiest (and only supported) way to install **lua-casclib** is to use
[Luarocks].  From your command line run the following:

```
luarocks install lua-casclib
```

[Luarocks]: https://luarocks.org
[LuaJIT]: https://luajit.org

## Documentation

For details regarding functionality, please consult the source files of this
library.  Each C function that represents part of the Lua API of
**lua-casclib** is appropriately documented.  It should be noted that an
attempt has been made to adhere to the inferface provide by Lua's [I/O]
library.  For basic references of how to use **lua-casclib**, see the
[Examples](#examples) section.

For the majority of the functions, if an error is encountered, `nil` will be
returned, along with a `string` describing the error, and a `number`
indicating the error code.  The exceptions to this rule would be the
iterators, which always raise.

### Caveats

This is a list of known limitations of **lua-casclib**.  Some of these may
be addressed over time.

1. **TL;DR: Your mileage may vary.**  This library has only been tested on
   Linux.
2. Functionality presently targets Warcraft III and its use cases.  As such,
   not all features of CascLib are currently exposed.

## Examples

``` lua
local casclib = require ('casclib')

local casc = casclib.open ('path/to/casc')
print (casc)

-- This will close the storage, as well as any of its open files.
casc:close ()

-- Iterate through a list of all file names.
for name in mpq:files () do
    -- All files in archive.
end

-- Can take a Lua pattern to refine the results.
for name in mpq:files ('%.slk$') do
    -- All matching files.
end

-- Can also take a plain string.
for name in mpq:files ('.slk', true) do
    -- All files that contain the matching string.
end

do
    local file = casc:open ('file.txt')
    print (file)
    file:close ()

    file:seek ()
    file:seek ('cur', 15)
    file:seek ('end', -8)
    file:seek ('set')

    file:read ()
    file:read ('*a')
    file:read ('l', '*L', 512)

    for line in file:lines () do
    end

    file:close ()
end

-- The archive, as well as any open files, will be garbage collected and
-- closed eventually.
--mpq:close ()
```
