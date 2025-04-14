--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    main.lua
--  brief:   the entry point from gui.lua, relays call-ins to the widget manager
--  author:  Dave Rodgers
--
--  Copyright (C) 2007.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

Spring.SendCommands({"ctrlpanel " .. LUAUI_DIRNAME .. "ctrlpanel.txt"})

VFS.Include(LUAUI_DIRNAME .. "rml_setup.lua",  nil, VFS.ZIP)
VFS.Include(LUAUI_DIRNAME .. 'utils.lua', nil, VFS.ZIP)

include("setupdefs.lua")
include("savetable.lua")

include("debug.lua")
include("fonts.lua")
include("layout.lua")   -- contains a simple LayoutButtons()
include("widgets.lua")  -- the widget handler


--------------------------------------------------------------------------------
--
-- print the header
--

if (RestartCount == nil) then
  RestartCount = 0
else 
  RestartCount = RestartCount + 1
end

do
  local restartStr = ""
  if (RestartCount > 0) then
    restartStr = "  (" .. RestartCount .. " Restarts)"
  end
  Spring.SendCommands({"echo " .. LUAUI_VERSION .. restartStr})
end


--------------------------------------------------------------------------------
-------------------------------------------------------------------------------
--
--  A few helper functions
--

function Say(msg)
  Spring.SendCommands({'say ' .. msg})
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
--
--  Update()  --  called every frame
--

activePage = 0

forceLayout = true


function Update()
  local currentPage = Spring.GetActivePage()
  if (forceLayout or (currentPage ~= activePage)) then
    Spring.ForceLayoutUpdate()  --  for the page number indicator
    forceLayout = false
  end
  activePage = currentPage

  fontHandler.Update()

  widgetHandler:Update()

  return
end


-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
--
--  WidgetHandler fixed calls
--

function Shutdown()
  return widgetHandler:Shutdown()
end

function GotChatMsg(command)
  return widgetHandler:GotChatMsg(command)
end

function ActiveCommandChanged(id, cmdType)
  return widgetHandler:ActiveCommandChanged(id, cmdType)
end

function CameraRotationChanged(rotx, roty, rotz)
  return widgetHandler:CameraRotationChanged(rotx, roty, rotz)
end

function CameraPositionChanged(posx, posy, posz)
  return widgetHandler:CameraPositionChanged(posx, posy, posz)
end

function CommandNotify(id, params, options)
  return widgetHandler:CommandNotify(id, params, options)
end

function DrawScreen(vsx, vsy)
  widgetHandler:SetViewSize(vsx, vsy)
  return widgetHandler:DrawScreen()
end

function KeyMapChanged()
  return widgetHandler:KeyMapChanged()
end

function KeyPress(key, mods, isRepeat, label, unicode, scanCode, actions)
  return widgetHandler:KeyPress(key, mods, isRepeat, label, unicode, scanCode, actions)
end

function KeyRelease(key, mods, label, unicode, scanCode, actions)
  return widgetHandler:KeyRelease(key, mods, label, unicode, scanCode, actions)
end

function MouseMove(x, y, dx, dy, button)
  return widgetHandler:MouseMove(x, y, dx, dy, button)
end

function MousePress(x, y, button)
  return widgetHandler:MousePress(x, y, button)
end

function MouseRelease(x, y, button)
  return widgetHandler:MouseRelease(x, y, button)
end

function IsAbove(x, y)
  return widgetHandler:IsAbove(x, y)
end

function GetTooltip(x, y)
  return widgetHandler:GetTooltip(x, y)
end

function AddConsoleLine(msg, priority)
  return widgetHandler:AddConsoleLine(msg, priority)
end

function GroupChanged(groupID)
  return widgetHandler:GroupChanged(groupID)
end


--
-- The unit (and some of the Draw) call-ins are handled
-- differently (see LuaUI/widgets.lua / UpdateCallIns())
--


--------------------------------------------------------------------------------

