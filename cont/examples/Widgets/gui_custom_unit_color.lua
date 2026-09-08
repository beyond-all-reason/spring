--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--
--  file:    gui_custom_unit_color.lua
--  brief:   Demonstrates custom unit color palette API
--  author:  RecoilEngine
--
--  Copyright (C) 2026.
--  Licensed under the terms of the GNU GPL, v2 or later.
--
--  Usage:
--    Select a unit and press 'U' to cycle through custom colors
--    Press Shift+U to reset to team color
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
  return {
    name      = "CustomUnitColor",
    desc      = "Demo widget for custom unit color palette API\n" ..
                "Press U to cycle custom colors, Shift+U to reset",
    author    = "RecoilEngine",
    date      = "2026",
    license   = "GNU GPL, v2 or later",
    layer     = 0,
    enabled   = true,
  }
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local spGetSelectedUnits      = Spring.GetSelectedUnits
local spGetUnitPaletteIndex   = Spring.GetUnitPaletteIndex
local spSetUnitPaletteIndex   = Spring.SetUnitPaletteIndex
local spGetCustomPaletteColor = Spring.GetCustomPaletteColor
local spSetCustomPaletteColor = Spring.SetCustomPaletteColor
local spEcho                  = Spring.Echo
local spGetKeyCode            = Spring.GetKeyCode

local currentCustomIndex = 0

local keyU = spGetKeyCode("u")

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:Initialize()
  -- Initialize some custom colors for demonstration
  -- These are stored at indices 0-7 in the custom palette
  assert(Engine.maxCustomPaletteID >= 7)
  spSetCustomPaletteColor(0, 1.0, 0.0, 0.0) -- Red
  spSetCustomPaletteColor(1, 0.0, 1.0, 0.0) -- Green
  spSetCustomPaletteColor(2, 0.0, 0.0, 1.0) -- Blue
  spSetCustomPaletteColor(3, 1.0, 1.0, 0.0) -- Yellow
  spSetCustomPaletteColor(4, 1.0, 0.0, 1.0) -- Magenta
  spSetCustomPaletteColor(5, 0.0, 1.0, 1.0) -- Cyan
  spSetCustomPaletteColor(6, 1.0, 0.5, 0.0) -- Orange
  spSetCustomPaletteColor(7, 0.5, 0.0, 1.0) -- Purple

  spEcho("[CustomUnitColor] Demo widget loaded")
  spEcho("[CustomUnitColor] Select a unit and press U to cycle colors")
end

--------------------------------------------------------------------------------

function widget:KeyPress(key)
  local alt, ctrl, meta, shift = Spring.GetModKeyState()

  if key == keyU then  -- 'U' key
    local selUnits = spGetSelectedUnits()
    if #selUnits == 0 then
      spEcho("[CustomUnitColor] No unit selected")
      return true
    end

    local unitID = selUnits[1]

    if shift then
      -- Reset to team color by passing nil
      spSetUnitPaletteIndex(unitID, nil)
      local teamID = Spring.GetUnitTeam(unitID)
      spEcho("[CustomUnitColor] Reset unit " .. unitID .. " to team color (team " .. teamID .. ")")
    else
      -- Cycle through custom colors
      currentCustomIndex = (currentCustomIndex + 1) % 8

      -- Get the color we just set
      local r, g, b = spGetCustomPaletteColor(currentCustomIndex)
      
      -- Apply custom color to unit (pass custom index directly)
      spSetUnitPaletteIndex(unitID, currentCustomIndex)
      
      spEcho("[CustomUnitColor] Unit " .. unitID .. " now uses custom color " .. 
             currentCustomIndex .. ": " ..
             string.format("%.2f,%.2f,%.2f", r, g, b))
    end
    return true
  end

  return false
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:IsAbove(x, y)
  local selUnits = spGetSelectedUnits()
  if #selUnits == 0 then
    return false
  end
  return true
end

function widget:GetTooltip(mx, my)
  local selUnits = spGetSelectedUnits()
  if #selUnits == 0 then
    return nil
  end

  local unitID = selUnits[1]
  local customIdx = spGetUnitPaletteIndex(unitID)
  
  if customIdx == nil then
    return "Unit " .. unitID .. " uses team color"
  else
    local r, g, b = spGetCustomPaletteColor(customIdx)
    return "Unit " .. unitID .. " uses custom color " .. customIdx .. 
           "\nColor: " .. string.format("%.2f, %.2f, %.2f", r, g, b)
  end
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------