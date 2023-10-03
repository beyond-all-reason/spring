RmlSolLua
===================

Generate bindings for [RmlUi](https://github.com/mikke89/RmlUi) using [sol3](https://github.com/ThePhD/sol2).

**RmlUi**'s built-in Lua plugin only supports Lua 5.2 and up.  This project leverages the **sol3** library to generate bindings that are compatible with Lua versions 5.1+ (and LuaJIT 2.1+ and MoonJIT).  **RmlSolLua** also extends the base Lua implementation with many new bindings.

This project requires C++17.

## Usage

```c++
#include <RmlUi/RmlUi.h>
#include <sol/sol.hpp>
#include <RmlSolLua/RmlSolLua.h>

int main()
{
  // Open the sol3 Lua state.
  sol::state lua;
  lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::table, sol::lib::math);

  // Initialize RmlUi.
  Backend::Initialize(...);
  Rml::Initialise();

  // Initialize RmlSolLua.
  Rml::SolLua::Initialise(&lua);

  // Do stuff.
  {
    // ...
  }

  // Collect garbage before shutdown.
  // This is needed in RmlUi 5.0 and below to prevent a crash.
  // Alternatively, make sure initialize the RmlSolLua plugin LAST.
  lua.collect_garbage();

  // Shutdown RmlUi.
  // This will also close RmlSolLua.
  Rml::Shutdown();
  Backend::Shutdown();

  return 0;
}

```

> ##### Note
> **RmlSolLua** does not manage your **sol3** `sol::state`.  You must ensure it stays in scope until after the call to `Rml::Shutdown`.
>
> **RmlUi 5.0** has a use-after-free bug with plugin shutdown that involves plugins that monitor `Rml::Element` creation and deletion (like the Debugger plugin).  To avoid a crash, you must either initialize RmlSolLua LAST (after Debugger), or call the `collect_garbage` function on your Lua state right before you shut down **RmlUi**.

## Coverage

**RmlSolLua** contains all the **RmlUi 5.0** Lua bindings EXCEPT:
- custom element instancing
- datagrid sources and formatters

**RmlSolLua** contains many extra Lua bindings not covered by **RmlUi 5.0**.  There are too many to list, but they can be found in the bindings under `src/bind/*.cpp` below any `//--` comments.  Any binding not covered by **RmlUi**'s base Lua bindings are kept separate to be easily identified.

## License

**RmlSolLua** is published under the [MIT license](LICENSE).
