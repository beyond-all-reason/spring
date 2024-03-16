--  file:    rml.lua
--  brief:   RmlUi Setup
--  author:  lov
--
--  Copyright (C) 2023.
--  Licensed under the terms of the GNU GPL, v2 or later.

if (RmlGuard or not RmlUi) then
	return
end
RmlGuard = true

-- Load fonts
RmlUi.LoadFontFace("Fonts/FreeMonoBold.ttf", true)
