+++
title = 'RmlUi'
date = 2025-05-11T13:17:05-07:00
draft = false
author = "Slashscreen"
+++

RmlUi is a UI framework that is defined using a HTML/CSS style workflow (using Lua instead of JS) intended to simplify UI development especially for those already familiar with web development. It is designed for interactive applications, and so is reactive by default. You can learn more about it on the [RmlUi website] and [differences in the Recoil version here](#differences-between-upstream-rmlui-and-rmlui-in-recoil).

## How does RmlUi Work?

To get started, it's important to learn a few key concepts.
- Context: This is a bundle of documents and data models.
- Document: This is a document tree/DOM (document object model) holding the actual UI.
- RML: This is the markup language used to define the document, it is very similar to XHTML (HTML but must be well-formed XML).
- RCSS: This is the styling language used to style the document, it is very similar to CSS2 but does differ in some places.
- Data Model: This holds information (a Lua table) that is used in the UI code in data bindings.

On the Lua side, you are creating 1 or more contexts and adding data models and documents to them.

On the Rml side, you are creating documents to be loaded by the Lua which will likely include data bindings to show information from the data model, and references to lua functions for event handlers.

As each widget/component you create will likely comprise of several files (.lua, .rml & .rcss) you may find having a folder for each widget/component a useful way to organise your files.

## Getting Started
 
RmlUi is available in LuaUI (the game UI) with future availability in LuaIntro & LuaMenu planned, so it is already there for you to use. 
However, to get going with it there is some setup code that should be considered for loading fonts, cursors and configuring ui scaling, an example script is provided below.
If you are working on a widget for an existing game, it is likely that the game already has some form of this setup code in, so you may be able to skip this section.

### Setup script

Here is the "[handler](https://github.com/beyond-all-reason/Beyond-All-Reason/blob/master/luaui/rml_setup.lua)", written by lov and ChrisFloofyKitsune, 2 of the people responsible for the RmlUi implementation.

```lua
--  luaui/rml_setup.lua
--  author:  lov + ChrisFloofyKitsune
--
--  Copyright (C) 2024.
--  Licensed under the terms of the GNU GPL, v2 or later.

if (RmlGuard or not RmlUi) then
	return
end
-- don't allow this initialization code to be run multiple times
RmlGuard = true

--[[
	Recoil uses a custom set of Lua bindings (check out rts/Rml/SolLua/bind folder in the C++ engine code)
	Aside from the Lua API, the rest of the RmlUi documentation is still relevant
		https://mikke89.github.io/RmlUiDoc/index.html
]]

--[[ create a common Context to be used for widgets
	pros:
		* Documents in the same Context can make use of the same DataModels, allowing for less duplicate data
		* Documents can be arranged in front/behind of each other dynamically
	cons:
		* Documents in the same Context can make use of the same data models, leading to side effects
		* DataModels must have unique names within the same Context

	If you have lots of DataModel use you may want to create your own Context
	otherwise you should be able to just use the shared Context

	Contexts created with the Lua API are automatically disposed of when the LuaUi environment is unloaded
]]

local oldCreateContext = RmlUi.CreateContext

local function NewCreateContext(name)
	local context = oldCreateContext(name)

	-- set up dp_ratio considering the user's UI scale preference and the screen resolution
	local viewSizeX, viewSizeY = Spring.GetViewGeometry()

	local userScale = Spring.GetConfigFloat("ui_scale", 1)

	local baseWidth = 1920
	local baseHeight = 1080
	local resFactor = math.min(viewSizeX / baseWidth, viewSizeY / baseHeight)

	context.dp_ratio = resFactor * userScale

	context.dp_ratio = math.floor(context.dp_ratio * 100) / 100
	return context
end

RmlUi.CreateContext = NewCreateContext

-- Load fonts
local font_files = {
}
for _, file in ipairs(font_files) do
	Spring.Echo("loading font", file)
	RmlUi.LoadFontFace(file, true)
end


-- Mouse Cursor Aliases
--[[
	These let standard CSS cursor names be used when doing styling.
	If a cursor set via RCSS does not have an alias, it is unchanged.
	CSS cursor list: https://developer.mozilla.org/en-US/docs/Web/CSS/cursor
	RmlUi documentation: https://mikke89.github.io/RmlUiDoc/pages/rcss/user_interface.html#cursor
]]

-- when "cursor: normal" is set via RCSS, "cursornormal" will be sent to the engine... and so on for the rest
RmlUi.SetMouseCursorAlias("default", 'cursornormal')
RmlUi.SetMouseCursorAlias("pointer", 'Move') -- command cursors use the command name. TODO: replace with actual pointer cursor?
RmlUi.SetMouseCursorAlias("move", 'uimove')
RmlUi.SetMouseCursorAlias("nesw-resize", 'uiresized2')
RmlUi.SetMouseCursorAlias("nwse-resize", 'uiresized1')
RmlUi.SetMouseCursorAlias("ns-resize", 'uiresizev')
RmlUi.SetMouseCursorAlias("ew-resize", 'uiresizeh')

RmlUi.CreateContext("shared")
```

What this does is create a unified context 'shared' for all your documents and data models, which is currently the recommended way to architect documents. If you have any custom font files, list them in `font_files`, otherwise leave it empty.

The setup script above can then be included from your `luaui/main.lua` (main.lua is the entry point for the LuaUI environment).

```lua
VFS.Include(LUAUI_DIRNAME .. "rml_setup.lua",  nil, VFS.ZIP) -- Runs the script
```
> [!NOTE] The rml_setup.lua script included in base content only imports a font and cursor and doesn't create a context or set scaling


### Writing Your First Document

You will be creating files with .rml and .rcss extensions, as these closely resemble HTML and CSS it is worth configuring your editor to treat these file extensions as HTML and CSS respectively, see the [IDE Setup](#ide-setup) section for more information.

Now, create an RML file somewhere under `luaui/widgets/`, like `luaui/widgets/getting_started.rml`. This is the UI document.

Writing it is much like HTML by design. There are some differences, the most immediate being the root tag is called rml instead of html, most other RML/HTML differences relate to attributes for databindings and events, but for the time being, we don't need to worry about them.

By default RmlUi has *NO* styles, this includes setting default element behaviour like block/inline and styles web developers would expect like input elements default appearances, as a starting point you can use [RmlUi documentation](https://mikke89.github.io/RmlUiDoc/pages/rml/html4_style_sheet.html) though these do not include styles for form elements. 

Here's a basic widget written by Mupersega.

```html
<rml>
<head>
    <title>Rml Starter</title>

    <style>
        #rml-starter-widget {
            pointer-events: auto;
            width: 400dp;
            right: 0;
            top: 50%;
            transform: translateY(-90%);
            position: absolute;
            margin-right: 10dp;
        }
        #main-content {
            padding: 10dp;
            border-radius: 8dp;
            z-index: 1;
        }
        #expanding-content {
            transform: translateY(0%);
            transition: top 0.1s linear-in-out;
            z-index: 0;
            height: 100%;
            width: 100%;
            position: absolute;
            top: 0dp;
            left: 0dp;
            border-radius: 8dp;
            display: flex;
            flex-direction: column;
            justify-content: flex-end;
            align-items: center;
            padding-bottom: 20dp;
        }
        /* This is just a checkbox sitting above the entirety of the expanding content */
        /* It is bound directly with data-checked attr to the expanded value */
        #expanding-content>input {
            height: 100%;
            width: 100%;
            z-index: 1;
            position: absolute;
            top: 0dp;
            left: 0dp;
        }
        #expanding-content.expanded {
            top: 90%;
        }
        #expanding-content:hover {
            background-color: rgba(255, 0, 0, 125);
        }
    </style>
</head>
<body>
    <div id="rml-starter-widget" class="relative" data-model="starter_model">
        <div id="main-content">
            <h1 class="text-primary">Welcome to an Rml Starter Widget</h1>
            <p>This is a simple example of an RMLUI widget.</p>
            <div>
                <button onclick="widget:Reload()">reload widget</button>
            </div>
        </div>
        <div id="expanding-content" data-class-expanded="expanded">
            <input type="checkbox" value="expanded" data-checked="expanded"/>
            <p>{{message}}</p>
            <div>
                <span data-for="test, i: testArray">name:{{test.name}} index:{{i}}</span>
            </div>
        </div>
    </div>
</body>
</rml>
```

Let's take a look at different areas that are important to look at.

`<div id="rml-starter-widget" class="relative" data-model="starter_model">`
1. Here, we bind to the data model using `data-model`. This is what we will need to name the data model in our Lua script later. Everything inside the model will be in scope and beneath the div.
2. Typically, it is recommended to bind your data model inside of a div beneath `body` rather than `body` itself.

```html
<div id="expanding-content" data-class-expanded="expanded">
    <input type="checkbox" value="expanded" data-checked="expanded"/>
```
any attribute starting with `data-` is a "data event". We will go through a couple below, but you can find out more here [on the RmlUi docs site](https://mikke89.github.io/RmlUiDoc/pages/data_bindings/views_and_controllers.html).
1. Double curly braces are used to show values from the data model within the document text. e.g. `{{message}}` shows the value of `message` in the data model.
2. `data-class-expanded="expanded"` applies the `expanded` class to the div if the value `expanded` in the data model is `true`.
3. `<input type="checkbox" value="expanded" data-checked="expanded"/>` This checkbox will set the data model value in `data-checked` to true.
4. When the data model is changed, the document is rerendered automatically. So, the expanding div will have the `expanded` class applied to it or removed whenever the check box is toggled.

There are data bindings to allow you to loop through arrays (data-for) have conditional sections of the document (data-if) and many others. 

### The Lua

> [!NOTE] For information on setting up your editor to provide intellisense behaviour see the [Lua Language Server guide]({{% ref "lua-language-server" %}}).

To load your document into the shared context we created earlier and to define and add the data model you will need to have a lua script something like the one below.

```lua
-- luaui/widgets/getting_started.lua
if not RmlUi then
    return
end

local widget = widget ---@type Widget

function widget:GetInfo()
    return {
        name = "Rml Starter",
        desc = "This widget is a starter example for RmlUi widgets.",
        author = "Mupersega",
        date = "2025",
        license = "GNU GPL, v2 or later",
        layer = -1000000,
        enabled = true
    }
end

local document
local dm_handle
local init_model = {
    expanded = false,
    message = "Hello, find my text in the data model!",
    testArray = {
        { name = "Item 1", value = 1 },
        { name = "Item 2", value = 2 },
        { name = "Item 3", value = 3 },
    },
}

local main_model_name = "starter_model"

function widget:Initialize()
    widget.rmlContext = RmlUi.GetContext("shared") -- Get the context from the setup lua

    -- Open the model, using init_model as the template. All values inside are copied.
    -- Returns a handle, which we will touch on later.
    dm_handle = widget.rmlContext:OpenDataModel(main_model_name, init_model)
    if not dm_handle then
        Spring.Echo("RmlUi: Failed to open data model ", main_model_name)
        return
    end
    -- Load the document we wrote earlier.
    document = widget.rmlContext:LoadDocument("luaui/widgets/getting_started.rml", widget)
    if not document then
        Spring.Echo("Failed to load document")
        return
    end

    -- uncomment the line below to enable debugger
    -- RmlUi.SetDebugContext('shared')

    document:ReloadStyleSheet()
    document:Show()
end

function widget:Shutdown()
    widget.rmlContext:RemoveDataModel(main_model_name)
    if document then
        document:Close()
    end
end

-- This function is only for dev experience, ideally it would be a hot reload, and not required at all in a completed widget.
function widget:Reload(event)
    Spring.Echo("Reloading")
    Spring.Echo(event)
    widget:Shutdown()
    widget:Initialize()
end
```

### The Data Model Handle

In the script, we are given a data model handle. This is a proxy for the Lua table used as the data model; as the Recoil RmlUi integration uses Sol2 as a wrapper data cannot be accessed directly.

In most cases, you can simply do `dm_handle.expanded = true`, but this only works for table entries with string keys. What if you have an array, like `testArray` above? To loop through on the Lua side, you will need to get the underlying table:

```lua
local model_handle = dm_handle:__GetTable()
```

As of writing, this function is not documented in the Lua API, due to some problems with language server generics that haven't been sorted out yet. It is there, however, and will be added back in in the future. it returns the table with the shape of your inital model.
You can then iterate through it, and change things as you please:

```lua
for i, v in pairs(model_handle.testArray) do
    Spring.Echo(i, v.name)
    v.value = v.value + 1
end
-- If you modified anything in the table, as we did here, we need to set the model handle "dirty", so it refreshes.
dm_handle:__SetDirty("testArray") -- We modified something nested in the array, so we mark the top level table entry as dirty
```

### Debugging

RmlUi comes with a debugger that lets you see and interact with the DOM. It's very handy! To use it, use this:

```lua
RmlUi.SetDebugContext(DATA_MODEL_NAME)
```

And it will appear in-game. A useful idiom I like is to put this in `rml_setup.lua`:

```lua
local DEBUG_RMLUI = true

--...

if DEBUG_RMLUI then
    RmlUi.SetDebugContext('shared')
end
```

If you use the shared context, you can see everything that happens in it! Neat!

## Things to Know and Best Practices

### Things to know
Some of the rough edges you are likely to run into have already been discussed, like the data model thing, but here are some more:

---

Unlike a web browser a default set of styles is not included, as a starting point you can look at the [RmlUi documentation](https://mikke89.github.io/RmlUiDoc/pages/rml/html4_style_sheet.html) though this doesn't provide styles for form elements.

When using Context:OpenDataModel in Lua you must assign the return value to a variable, not doing so will cause the engine to crash when the model is reference in RML.

Input elements of type submit & button behave differently to HTML and more like Button elements in that their text is not set by the value attribute. (This is likely to be corrected in a future version)

The alpha/transparency value of an RGBA colour is different to CSS (0-1) and instead uses 0-255. The css opacity does still use 0-1.

List styling is unavailable (list-style-type etc.), you can still use UL/OL/LI elements but there is no special meaning to them, whilst you could use background images to replicate bullets there isn't a practical way to achieve numbered list items with RML/RCSS.

Only solid borders are supported, so the border-style property is unavailable and the shorthand border property doesn't include a style part (```border: 1dp solid black;``` won't work, instead use ```border: 1dp black;```).

background-color behaves as expected, all other background styles are different and use decorators instead see the [RmlUi documentation for more information on decorators.](https://mikke89.github.io/RmlUiDoc/pages/rcss/decorators.html) 

There are two kinds of events: data events, like `data-mouseover`, and normal events, like `onmouseover`. These have different data in their scopes.
- Data events have the data model in their scope.
- Normal events don't have the data model, but they *do* have whatever is passed into `widget` on `widget.rmlContext:LoadDocument`. `widget` doesn't have to be a widget, just any table with data in it.

For example, take this:

```lua
local model = {
    add = function(a, b)
        Spring.Echo(a + b)
    end
}
local document_table = {
    print = function(msg)
        Spring.Echo(msg)
    end,
}

dm_handle = widget.rmlContext:OpenDataModel("test", model)
document = widget.rmlContext:LoadDocument("document.rml", document_table)
```

```html
<h1>Normal Events</h1>
<input type="button" onclick="add(1, 2)">Won't work!</input>
<input type="button" onclick="print('test')">Will work!</input>

<h1>Data Events</h1>
<input type="button" data-click="add(1, 2)">Will work!</input>
<input type="button" data-click="print('test')">Won't work!</input>
```

### Best Practices

- To create a scalable interface the use of the dp unit over px is recommended as the scale can be set per context with SetDensityIndependentPixelRatio.
- For styles unique to a document, put them in a `style` tag. For shared styles, put them in an `rcss` file.
- Rely on Recoil's RmlUi Lua bindings doc for what you can and can't do. The Recoil implementation has some extra stuff the RmlUi docs don't.
- The Beyond All Reason devs prefer to use one shared context for all rmlui widgets.


### Differences between upstream RmlUi and RmlUi in Recoil

- The SVG element allows either a filepath or raw SVG data in the src attribute, allowing for inline svg to be used (this may change to svg being supported between the opening and closing tag when implemented upstream)
- An additional element ```<texture>``` is available which allows for textures loaded in Recoil to be used, this behaves the same as an ```<img>``` element except the src attribute takes a [texture reference]({{% ref "articles/texture-reference-strings" %}})


### IDE Setup

To get the best experience with RmlUi, you should set up your editor to use HTML syntax highlighting for .rml file extensions and CSS syntax highlighting for .rcss file extensions.

In VS Code, this can be done by opening a file with the extension you want to setup, then clicking on the language mode  in the bottom right corner of the window (probably shows as Plain Text). From there, you can select "Configure File Association for '.rml'" from the top menu that appears and choose "HTML" from the list. Do the same for .rcss files, but select "CSS".

[RmlUI website]: https://mikke89.github.io/RmlUiDoc/
