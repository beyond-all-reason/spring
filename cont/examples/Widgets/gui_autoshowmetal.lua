function widget:GetInfo()
  return {
    name      = "Auto Show Metal",
    desc      = "Toggles metal view for extractor building commands",
    license   = "GNU GPL, v2 or later",
    layer     = -50,
    enabled   = true
  }
end

local isMex = {}

local function initMexes()
  for uDefID, uDef in pairs(UnitDefs) do
    if uDef.extractsMetal > 0 then
      isMex[uDefID] = true
    end
  end
end

function widget:Initialize()
  if Engine.FeatureSupport.noAutoShowMetal == nil then
    -- engine doesn't yet have support for disabling autometal
    widgetHandler:RemoveWidget()
    return
  end
  if Spring.SetAutoShowMetal then
    -- disable automatic showmetal control, check existance so this will
    -- keep working when AutoShowMetal gets removed from future engine version.
    Spring.SetAutoShowMetal(false)
  end

  local success, mapinfo = pcall(VFS.Include, "mapinfo.lua")

  if mapinfo and mapinfo["autoshowmetal"] then
    initMexes()
  else
    widgetHandler:RemoveWidget()
  end
end

function widget:ActiveCommandChanged(cmdID)
  local wantsMetal = (cmdID and isMex[-cmdID]) or false
  local isMetalViewOn = (Spring.GetMapDrawMode() == 'metal')

  if wantsMetal ~= isMetalViewOn then
    Spring.SendCommands("showmetalmap")
  end
end
