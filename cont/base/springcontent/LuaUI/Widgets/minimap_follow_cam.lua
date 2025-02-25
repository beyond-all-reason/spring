function widget:GetInfo()
	return {
		name = "MiniMapFollowCam",
		desc = "Minimap rotates depending on player camera rotation",
		author = "TheFutureKnight",
		date = "2025-2-20",
		license = "GNU GPL, v2 or later",
		layer = 0,
		enabled = true,
	}
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

  -- Automatically generated local definitions

  local spSetMiniRot		= 	Spring.SetMiniMapRotation
  local spGetCamRot     =   Spring.GetCameraRotation

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:Update()
    local _, roty, _ = spGetCamRot()  -- Camera rotation in radians
    
    -- Snap to nearest 180 degree increment
    local snapped = math.pi * math.floor((roty/math.pi) + 0.5)
    local clampedRot = snapped % (2 * math.pi)  -- Keep within [0, 2π] for BAR Support
    
    spSetMiniRot(clampedRot)
end