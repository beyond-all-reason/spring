# Lua doc comments guide

Lua documentation is generated from cpp files. These are automatically exported to be consumed by games that use Recoil (e.g. BAR). The types are compatible with [Lua Language Server](https://luals.github.io/), providing static analysis and intellisense.

```cpp
/***
 * @function Spring.GetFeaturesInScreenRectangle
 *
 * Get features inside a rectangle area on the map
 *
 * @param left number
 * @param top number
 * @param right number
 * @param bottom number
 * @return nil|number[] featureIDs
 */
int LuaUnsyncedRead::GetFeaturesInScreenRectangle(lua_State* L)
{
 // CPP code
}
```

## Export

Special comments blocks are parsed by [lua-doc-extractor]( https://github.com/rhys-vdw/lua-doc-extractor) and converted into [definition files](https://luals.github.io/wiki/definition-files/).

Comment blocks must start with `/**`, and by convention each line starts with `*`.

The body of the comment is translated literally, but some code generation is required. For these [custom tags](https://github.com/rhys-vdw/lua-doc-extractor?tab=readme-ov-file#custom-tags) must be used.

Markdown is supported in all text.

## Annotations

[Full list of annotations](https://luals.github.io/wiki/annotations/).

### Common annotations

- [`@function`](https://github.com/rhys-vdw/lua-doc-extractor?tab=readme-ov-file#function-name)
- [`@table`](https://github.com/rhys-vdw/lua-doc-extractor?tab=readme-ov-file#table-name)
- [`@param`](https://luals.github.io/wiki/annotations/#param)
- [`@return`](https://luals.github.io/wiki/annotations/#return)
- [`@field`](https://luals.github.io/wiki/annotations/#field)

## Types

[List of types](https://luals.github.io/wiki/annotations/#documenting-types)

### Common types

- `integer`
- `nil`
- `any`
- `boolean`
- `string`
- `number` (for floating point numbers)
- `integer`
- `table<,>`

> [!TIP]
> Literals (e.g. `true`, `false`, `5`) are also available as types. `true` is useful in the case where a table is being used as a set, e.g.
> ```
> table<string, true>
> ```

### Unions

Union types can be specified with `|`.

```
string|boolean|number
```

Suffix with `?` as a shorthand for a union with `nil`, e.g.

```
string|nil
```
can be expressed as:
```
string?
```

### Arrays

An array type is expressed as `type[]`.

Array of numbers
```
number[]
```
Array of string or null
```
string?[]
```
A single number, or an array of strings:
```
number|string[]
```
An array with a mix of number and string.
```
(number|string)[]
```

## Examples

### Function

- Must start with `@function TableName.FunctionName`.
- Can have any amount of description. This should be added after the first 
- Specify parameters with `@param parameterName type Description...`
- Specify return type with `@return type name Description...`
- For multiple returns use one per line.

> [!IMPORTANT]
> For `@return` must specify the type _before_ the name, whereas `@param` takes the name before the type.

```cpp
/**
 * @function Math.Add
 * 
 * Add two integers together.
 * 
 * This function will add two numbers together and return the result.
 * 
 * ```lua
 * local totalHeight = Math.Add(legLength, upperBodyHeight)
 * ```
 * 
 * @param a integer The first number.
 * @param b integer The second number.
 * @returns integer result The sum of `a` and `b`.
```

### Class

Structured data is expressed as a class. This represents a table with expected key/value pairs.

- Must start with `@class ClassName`. This name becomes a type that can be used in annotations.
- Fields are specified by `@field fieldName type Description...`.

```cpp
/**
 * @class Color
 * 
 * Describes an RGB color value.
 * 
 * @field red number The red value.
 * @field green number The green value.
 * @field blue number The blue value.
 */

/**
 * @function ColorUtility.LerpColor
 * @param from Color
 * @param to Color
 * @param value number The mix (in range `[0,1]`) of colors to combine. `1` will return `to` and `0` will return `from`.
 * @return Color color
 */
```

### Table

A global table can be defined like so:

```cpp
/**
 * @table Spring
 */
```

This will define a stub table. For functions that are added to a table, one of these must be defined. The table name should be used in the function name e.g. `@function Spring.MyFunction`.

A table of constants can also be expressed using `@field`:

```cpp
/**
 * @table CoolNumbers
 * @field number Pi
 * @field integer SixyNine
 * @field integer FourTwenty
 */
```

> [!NOTE]
> A table is a global that can be accessed in Lua and not a type like `@class`.

## Library files

All files under `Lua/Library/` are available to the IDE.

The `generated` folder is the result of exporting from cpp.

If a type is used in multiple files, you can define it in `Lua/Library/Types.lua` (or any other file outside of the `generated`) folder.

## Generation

Exporting the library is automated by the [Generate Lua library workflow](.github/workflows/generate-lua-library.yml). You can test it locally by following the same steps locally.

See doc [README](/doc/site/README.md) for info on doc generation.