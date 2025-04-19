---
layout: default
title: VFS
parent: Lua API
permalink: lua-api/globals/VFS
---

{% raw %}

# global VFS




## methods


### VFS.GetMaps

```lua
function VFS.GetMaps() -> mapNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaArchive.cpp#L52-L55" target="_blank">source</a>]


### VFS.GetGames

```lua
function VFS.GetGames() -> gameNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaArchive.cpp#L63-L66" target="_blank">source</a>]


### VFS.GetAllArchives

```lua
function VFS.GetAllArchives() -> archiveNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaArchive.cpp#L82-L85" target="_blank">source</a>]


### VFS.HasArchive

```lua
function VFS.HasArchive() -> hasArchive boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaArchive.cpp#L101-L104" target="_blank">source</a>]


### VFS.GetLoadedArchives

```lua
function VFS.GetLoadedArchives() -> archiveNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaArchive.cpp#L115-L118" target="_blank">source</a>]


### VFS.GetArchivePath

```lua
function VFS.GetArchivePath(archiveName: string) -> archivePath string?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaArchive.cpp#L129-L133" target="_blank">source</a>]


### VFS.GetArchiveInfo

```lua
function VFS.GetArchiveInfo(archiveName: string) -> archiveInfo ArchiveInfo?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaArchive.cpp#L169-L173" target="_blank">source</a>]


### VFS.GetArchiveDependencies

```lua
function VFS.GetArchiveDependencies(archiveName: string) -> archiveNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaArchive.cpp#L214-L218" target="_blank">source</a>]


### VFS.GetArchiveReplaces

```lua
function VFS.GetArchiveReplaces(archiveName: string) -> archiveNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaArchive.cpp#L232-L236" target="_blank">source</a>]


### VFS.GetArchiveChecksum

```lua
function VFS.GetArchiveChecksum(archiveName: string)
 -> singleArchiveChecksum string
 -> completeArchiveChecksum string

```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaArchive.cpp#L254-L259" target="_blank">source</a>]


### VFS.GetNameFromRapidTag

```lua
function VFS.GetNameFromRapidTag(rapidTag: string) -> archiveName string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaArchive.cpp#L279-L283" target="_blank">source</a>]


### VFS.GetAvailableAIs

```lua
function VFS.GetAvailableAIs(
  gameArchiveName: string?,
  mapArichiveName: string?
) -> ais AIInfo[]
```





Gets a list of all Spring AIs. The optional gameName and mapName parameters
can be used to include game/map specific LuaAIs in the list.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaArchive.cpp#L305-L313" target="_blank">source</a>]


### VFS.Include

```lua
function VFS.Include(
  filename: string,
  environment: table?,
  mode: string?
) -> module any
```
@param `filename` - Path to file, lowercase only. Use linux style path separators, e.g.
`"foo/bar.txt"`.

@param `environment` - (Default: the current function environment)

The environment arg sets the global environment (see generic lua refs). In
almost all cases, this should be left `nil` to preserve the current env.

If the provided, any non-local variables and functions defined in
`filename.lua` are then accessable via env. Vise-versa, any variables
defined in env prior to passing to `VFS.Include` are available to code in the
included file. Code running in `filename.lua` will see the contents of env in
place of the normal global environment.

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.


@return `module` - The return value of the included file.





Loads and runs lua code from a file in the VFS.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L276-L310" target="_blank">source</a>]

The path is relative to the main Spring directory, e.g.

```lua
VFS.Include('LuaUI/includes/filename.lua', nil, vfsmode)
```


### VFS.LoadFile

```lua
function VFS.LoadFile(
  filename: string,
  mode: string?
) -> data string?
```
@param `filename` - Path to file, lowercase only. Use linux style path separators, e.g.
`"foo/bar.txt"`.

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.


@return `data` - The contents of the file.





Load raw text data from the VFS.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L394-L414" target="_blank">source</a>]

Returns file contents as a string. Unlike `VFS.Include` the file will not be
executed. This lets you pre-process the code. Use `loadstring` afterwards.


### VFS.FileExists

```lua
function VFS.FileExists(
  filename: string,
  mode: string?
) -> exists boolean
```
@param `filename` - Path to file, lowercase only. Use linux style path separators, e.g.
`"foo/bar.txt"`.

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.


@return `exists` - `true` if the file exists, otherwise `false`.





Check if file exists in VFS.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L445-L470" target="_blank">source</a>]

Example usage:

```lua
if VFS.FileExists("mapconfig/custom_lava_config.lua", VFS.MAP) then
  # ...
end
```


### VFS.DirList

```lua
function VFS.DirList(
  directory: string,
  pattern: string?,
  mode: string?,
  recursive: boolean?
) -> filenames string[]
```
@param `directory` - Path to directory, lowercase only. Use linux style path separators, e.g.
`"foo/bar/"`.

@param `pattern` - (Default: `"*"`)

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.

@param `recursive` - (Default: `false`)






List files in a directory.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L491-L518" target="_blank">source</a>]

Example usage:

```lua
local luaFiles = VFS.DirList('units/', '*.lua', nil, true)
```


### VFS.SubDirs

```lua
function VFS.SubDirs(
  directory: string,
  pattern: string?,
  mode: string?,
  recursive: boolean?
) -> dirnames string[]
```
@param `directory` - Path to directory, lowercase only. Use linux style path separators, e.g.
`"foo/bar/"`.

@param `pattern` - (Default: `"*"`)

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.

@param `recursive` - (Default: `false`)






List sub-directories in a directory.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L550-L580" target="_blank">source</a>]

Example usage:

```lua
local files = VFS.SubDirs('sounds/voice/' .. language, '*')
for _, file in ipairs(files) do
    # ...
end
```


### VFS.GetFileAbsolutePath

```lua
function VFS.GetFileAbsolutePath(
  filename: string,
  mode: string?
) -> absolutePath string?
```
@param `filename` - Path to file, lowercase only. Use linux style path separators, e.g.
`"foo/bar.txt"`.

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L608-L623" target="_blank">source</a>]


### VFS.GetArchiveContainingFile

```lua
function VFS.GetArchiveContainingFile(
  filename: string,
  mode: string?
) -> archiveName string?
```
@param `filename` - Path to file, lowercase only. Use linux style path separators, e.g.
`"foo/bar.txt"`.

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.






[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L647-L662" target="_blank">source</a>]


### VFS.UseArchive

```lua
function VFS.UseArchive(
  archiveName: string,
  fun: unknown
) -> Results any...
```

@return `Results` - of of the given function





Temporarily load an archive from the VFS and run the given function,
which can make usage of the files in the archive.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L687-L695" target="_blank">source</a>]


### VFS.CompressFolder

```lua
function VFS.CompressFolder(
  folderPath: string,
  archiveType: string?,
  compressedFilePath: string?,
  includeFolder: boolean?,
  mode: string?
)
```
@param `archiveType` - (Default: `"zip"`)The compression type (can
currently be only `"zip"`).

@param `compressedFilePath` - (Default: `folderPath .. ".sdz"`)

@param `includeFolder` - (Default: `false`) Whether the archive should
have the specified folder as root.






Compresses the specified folder.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L815-L825" target="_blank">source</a>]


### VFS.ZlibCompress

```lua
function VFS.ZlibCompress(uncompressed: string) -> compressed string?
```
@param `uncompressed` - Data to compress.


@return `compressed` - Compressed data, or `nil` on error.





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L861-L865" target="_blank">source</a>]


### VFS.ZlibDecompress

```lua
function VFS.ZlibDecompress(compressed: string) -> uncompressed string?
```
@param `compressed` - Data to decompress.


@return `uncompressed` - Uncompressed data, or `nil` on error.





[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L881-L885" target="_blank">source</a>]


### VFS.CalculateHash

```lua
function VFS.CalculateHash(
  input: string,
  hashType: HashType
) -> hash string?
```
@param `hashType` - Hash type.






Calculates hash (in base64 form) of a given string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L908-L915" target="_blank">source</a>]


### VFS.PackU8

```lua
function VFS.PackU8(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert unsigned 8-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L997-L1002" target="_blank">source</a>]


### VFS.PackU8

```lua
function VFS.PackU8(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert unsigned 8-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1003-L1008" target="_blank">source</a>]


### VFS.PackU16

```lua
function VFS.PackU16(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert unsigned 16-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1011-L1016" target="_blank">source</a>]


### VFS.PackU16

```lua
function VFS.PackU16(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert unsigned 16-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1017-L1022" target="_blank">source</a>]


### VFS.PackU32

```lua
function VFS.PackU32(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert unsigned 32-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1025-L1030" target="_blank">source</a>]


### VFS.PackU32

```lua
function VFS.PackU32(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert unsigned 32-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1031-L1036" target="_blank">source</a>]


### VFS.PackS8

```lua
function VFS.PackS8(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert signed 8-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1039-L1044" target="_blank">source</a>]


### VFS.PackS8

```lua
function VFS.PackS8(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert signed 8-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1045-L1050" target="_blank">source</a>]


### VFS.PackS16

```lua
function VFS.PackS16(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert signed 16-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1053-L1058" target="_blank">source</a>]


### VFS.PackS16

```lua
function VFS.PackS16(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert signed 16-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1059-L1064" target="_blank">source</a>]


### VFS.PackS32

```lua
function VFS.PackS32(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert signed 32-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1067-L1072" target="_blank">source</a>]


### VFS.PackS32

```lua
function VFS.PackS32(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert signed 32-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1073-L1078" target="_blank">source</a>]


### VFS.PackS32

```lua
function VFS.PackS32(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert signed 32-bit float(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1081-L1086" target="_blank">source</a>]


### VFS.PackS32

```lua
function VFS.PackS32(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert signed 32-bit float(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1087-L1092" target="_blank">source</a>]


### VFS.UnpackU8

```lua
function VFS.UnpackU8(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to an unsigned 8-bit integer.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1140-L1146" target="_blank">source</a>]


### VFS.UnpackU16

```lua
function VFS.UnpackU16(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to an unsigned 16-bit integer.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1149-L1155" target="_blank">source</a>]


### VFS.UnpackU32

```lua
function VFS.UnpackU32(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to an unsigned 32-bit integer.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1158-L1164" target="_blank">source</a>]


### VFS.UnpackS8

```lua
function VFS.UnpackS8(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to a signed 8-bit integer.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1167-L1173" target="_blank">source</a>]


### VFS.UnpackS16

```lua
function VFS.UnpackS16(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to a signed 16-bit integer.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1176-L1182" target="_blank">source</a>]


### VFS.UnpackS32

```lua
function VFS.UnpackS32(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to a signed 32-bit integer.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1185-L1191" target="_blank">source</a>]


### VFS.UnpackF32

```lua
function VFS.UnpackF32(
  str: string,
  pos: integer?
) ->  integer
```
@param `str` - Binary string.

@param `pos` - Byte offset.






Convert a binary string to a signed 32-bit float.

[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L1194-L1200" target="_blank">source</a>]





## fields


### VFS.RAW

```lua
VFS.RAW: string = "r"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L150-L150" target="_blank">source</a>]
Only select uncompressed files.


### VFS.GAME

```lua
VFS.GAME: string = "M"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L152-L152" target="_blank">source</a>]


### VFS.MAP

```lua
VFS.MAP: string = "m"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L154-L154" target="_blank">source</a>]


### VFS.BASE

```lua
VFS.BASE: string = "b"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L156-L156" target="_blank">source</a>]


### VFS.MENU

```lua
VFS.MENU: string = "e"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L158-L158" target="_blank">source</a>]


### VFS.ZIP

```lua
VFS.ZIP: string = "Mmeb"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L160-L160" target="_blank">source</a>]
Only select compressed files (`.sdz`, `.sd7`).


### VFS.RAW_FIRST

```lua
VFS.RAW_FIRST: string = "rMmeb"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L162-L162" target="_blank">source</a>]
Try uncompressed files first, then compressed.


### VFS.ZIP_FIRST

```lua
VFS.ZIP_FIRST: string = "Mmebr"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L164-L164" target="_blank">source</a>]
Try compressed files first, then uncompressed.


### VFS.MOD

```lua
VFS.MOD: string = "M"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L167-L170" target="_blank">source</a>]


### VFS.RAW_ONLY

```lua
VFS.RAW_ONLY: string = "r"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L172-L175" target="_blank">source</a>]


### VFS.ZIP_ONLY

```lua
VFS.ZIP_ONLY: string = "Mmeb"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/0a561a37ee97c7883fd3f5a4bc995f9a4f6fdea0/rts/Lua/LuaVFS.cpp#L177-L180" target="_blank">source</a>]




{% endraw %}