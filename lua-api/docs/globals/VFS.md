---
layout: default
title: VFS
parent: Lua API
permalink: lua-api/globals/VFS
---

# global VFS




## methods


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

@param `environment` - (Default: `_G`)

The environment arg sets the global environment (see generic lua refs). In
almost all cases, this should be left `nil` to preserve Spring default.

If the provided, any non-local variables and functions defined in
`filename.lua` are then accessable via env or `_G`. Vise-versa, any variables
defined in env prior to passing to `VFS.Include` are available to code in the
included file. Code running in `filename.lua` will see the contents of env in
place of the normal `_G` environment.

@param `mode` - VFS modes are single char strings and can be concatenated;
doing specifies an order of preference for the mode (i.e. location) from
which to include files.


@return `module` - The return value of the included file.





Loads and compiles lua code from a file in the VFS.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L261-L295" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L379-L399" target="_blank">source</a>]

Returns file contents as a string. Unlike `VFS.Include` the file will not be
executed.


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L430-L455" target="_blank">source</a>]

Example usage:

```lua
if VFS.FileExists("maps/Castles.sdz") then
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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L476-L503" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L535-L565" target="_blank">source</a>]

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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L593-L608" target="_blank">source</a>]


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






[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L632-L647" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L672-L680" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L800-L810" target="_blank">source</a>]


### VFS.ZlibCompress

```lua
function VFS.ZlibCompress(uncompressed: string) -> compressed string?
```
@param `uncompressed` - Data to compress.


@return `compressed` - Compressed data, or `nil` on error.





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L846-L850" target="_blank">source</a>]


### VFS.ZlibDecompress

```lua
function VFS.ZlibDecompress(compressed: string) -> uncompressed string?
```
@param `compressed` - Data to decompress.


@return `uncompressed` - Uncompressed data, or `nil` on error.





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L866-L870" target="_blank">source</a>]


### VFS.CalculateHash

```lua
function VFS.CalculateHash(
  input: string,
  hashType: HashType
) -> hash string?
```
@param `hashType` - Hash type.






Calculates hash (in base64 form) of a given string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L893-L900" target="_blank">source</a>]


### VFS.PackU8

```lua
function VFS.PackU8(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert unsigned 8-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L982-L987" target="_blank">source</a>]


### VFS.PackU8

```lua
function VFS.PackU8(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert unsigned 8-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L988-L993" target="_blank">source</a>]


### VFS.PackU16

```lua
function VFS.PackU16(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert unsigned 16-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L996-L1001" target="_blank">source</a>]


### VFS.PackU16

```lua
function VFS.PackU16(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert unsigned 16-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1002-L1007" target="_blank">source</a>]


### VFS.PackU32

```lua
function VFS.PackU32(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert unsigned 32-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1010-L1015" target="_blank">source</a>]


### VFS.PackU32

```lua
function VFS.PackU32(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert unsigned 32-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1016-L1021" target="_blank">source</a>]


### VFS.PackS8

```lua
function VFS.PackS8(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert signed 8-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1024-L1029" target="_blank">source</a>]


### VFS.PackS8

```lua
function VFS.PackS8(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert signed 8-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1030-L1035" target="_blank">source</a>]


### VFS.PackS16

```lua
function VFS.PackS16(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert signed 16-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1038-L1043" target="_blank">source</a>]


### VFS.PackS16

```lua
function VFS.PackS16(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert signed 16-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1044-L1049" target="_blank">source</a>]


### VFS.PackS32

```lua
function VFS.PackS32(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert signed 32-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1052-L1057" target="_blank">source</a>]


### VFS.PackS32

```lua
function VFS.PackS32(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert signed 32-bit integer(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1058-L1063" target="_blank">source</a>]


### VFS.PackS32

```lua
function VFS.PackS32(...: integer) ->  string
```
@param `...` - Numbers to pack.






Convert signed 32-bit float(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1066-L1071" target="_blank">source</a>]


### VFS.PackS32

```lua
function VFS.PackS32(numbers: integer[]) ->  string
```
@param `numbers` - Numbers to pack.






Convert signed 32-bit float(s) to binary string.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1072-L1077" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1125-L1131" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1134-L1140" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1143-L1149" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1152-L1158" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1161-L1167" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1170-L1176" target="_blank">source</a>]


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

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L1179-L1185" target="_blank">source</a>]


### VFS.GetMaps

```lua
function VFS.GetMaps() -> mapNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaArchive.cpp#L52-L55" target="_blank">source</a>]


### VFS.GetGames

```lua
function VFS.GetGames() -> gameNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaArchive.cpp#L63-L66" target="_blank">source</a>]


### VFS.GetAllArchives

```lua
function VFS.GetAllArchives() -> archiveNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaArchive.cpp#L82-L85" target="_blank">source</a>]


### VFS.HasArchive

```lua
function VFS.HasArchive() -> hasArchive boolean
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaArchive.cpp#L101-L104" target="_blank">source</a>]


### VFS.GetLoadedArchives

```lua
function VFS.GetLoadedArchives() -> archiveNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaArchive.cpp#L115-L118" target="_blank">source</a>]


### VFS.GetArchivePath

```lua
function VFS.GetArchivePath(archiveName: string) -> archivePath string?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaArchive.cpp#L129-L133" target="_blank">source</a>]


### VFS.GetArchiveInfo

```lua
function VFS.GetArchiveInfo(archiveName: string) -> archiveInfo ArchiveInfo?
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaArchive.cpp#L169-L173" target="_blank">source</a>]


### VFS.GetArchiveDependencies

```lua
function VFS.GetArchiveDependencies(archiveName: string) -> archiveNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaArchive.cpp#L214-L218" target="_blank">source</a>]


### VFS.GetArchiveReplaces

```lua
function VFS.GetArchiveReplaces(archiveName: string) -> archiveNames string[]
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaArchive.cpp#L232-L236" target="_blank">source</a>]


### VFS.GetArchiveChecksum

```lua
function VFS.GetArchiveChecksum(archiveName: string)
 -> singleArchiveChecksum string
 -> completeArchiveChecksum string

```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaArchive.cpp#L254-L259" target="_blank">source</a>]


### VFS.GetNameFromRapidTag

```lua
function VFS.GetNameFromRapidTag(rapidTag: string) -> archiveName string
```





[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaArchive.cpp#L279-L283" target="_blank">source</a>]


### VFS.GetAvailableAIs

```lua
function VFS.GetAvailableAIs(
  gameArchiveName: string?,
  mapArichiveName: string?
) -> ais AIInfo[]
```





Gets a list of all Spring AIs. The optional gameName and mapName parameters
can be used to include game/map specific LuaAIs in the list.

[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaArchive.cpp#L305-L313" target="_blank">source</a>]





## fields


### VFS.RAW

```lua
VFS.RAW: string = "r"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L139-L139" target="_blank">source</a>]
Only select uncompressed files.


### VFS.RAW

```lua
VFS.RAW: string = "M"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L141-L141" target="_blank">source</a>]


### VFS.GAME

```lua
VFS.GAME: string = "M"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L143-L143" target="_blank">source</a>]


### VFS.MAP

```lua
VFS.MAP: string = "m"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L145-L145" target="_blank">source</a>]


### VFS.BASE

```lua
VFS.BASE: string = "b"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L147-L147" target="_blank">source</a>]


### VFS.MENU

```lua
VFS.MENU: string = "e"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L149-L149" target="_blank">source</a>]


### VFS.ZIP

```lua
VFS.ZIP: string = "Mmeb"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L151-L151" target="_blank">source</a>]
Only select compressed files (`.sdz`, `.sd7`).


### VFS.RAW_FIRST

```lua
VFS.RAW_FIRST: string = "rMmeb"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L153-L153" target="_blank">source</a>]
Try uncompressed files first, then compressed.


### VFS.RAW_FIRST

```lua
VFS.RAW_FIRST: string = "Mmebr"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L155-L155" target="_blank">source</a>]
Try compressed files first, then uncompressed.


### VFS.RAW_ONLY

```lua
VFS.RAW_ONLY: string = "r"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L157-L160" target="_blank">source</a>]


### VFS.ZIP_ONLY

```lua
VFS.ZIP_ONLY: string = "Mmeb"
```



[<a href="https://github.com/beyond-all-reason/spring/blob/7956ca9fcba24cb28088d05e5408f5c53951a7a4/rts/Lua/LuaVFS.cpp#L162-L165" target="_blank">source</a>]


