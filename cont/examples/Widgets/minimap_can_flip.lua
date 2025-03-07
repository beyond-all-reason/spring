function widget:GetInfo()
	return {
		name = "MiniMapCanFlip",
		desc = "Minimap flips between 0 and 180 degrees based on player camera rotation",
		author = "TheFutureKnight",
		date = "2025-2-20",
		license = "GNU GPL, v2 or later",
		layer = 0,
		enabled = true,
	}
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

  local spSetMiniRot		= 	Spring.SetMiniMapRotation
  local spGetCamRot     	=   Spring.GetCameraRotation

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:Update()
    local _, roty, _ = spGetCamRot()  -- Camera rotation in radians
    
    -- Snap to nearest 180 degree increment
    local newRot = math.pi * math.floor((roty/math.pi) + 0.5)
    
    spSetMiniRot(newRot)
end