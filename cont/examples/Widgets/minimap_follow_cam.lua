function widget:GetInfo()
	return {
		name = "MiniMapFollowCam",
		desc = "Minimap rotates depending on player camera rotation in all directions [0, 3π/2]",
		author = "TheFutureKnight",
		date = "2025-2-20",
		license = "GNU GPL, v2 or later",
		layer = 0,
		enabled = true,
	}
end

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

local spSetMiniMapRot     = Spring.SetMiniMapRotation
local spGetMiniMapGeo     = Spring.GetMiniMapGeometry

local mapAspect
local prevSnappedRot = -1

--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:CameraRotationChanged(_, roty)
	local snappedRot = math.pi/2 * (math.floor((roty/(math.pi/2)) + 0.5) % 4)

	if snappedRot == prevSnappedRot then return end
	prevSnappedRot = snappedRot

	local shouldBeWider = (mapAspect > 1.0) ~= (snappedRot == math.pi/2) or (snappedRot == 3*math.pi/2)
	
	local px, py, sx, sy = spGetMiniMapGeo()
	if shouldBeWider ~= (sx > sy) then
		gl.ConfigMiniMap(px, py, sy, sx)
	end

	spSetMiniMapRot(snappedRot) --roty also works here
end

function widget:Initialize()
	mapAspect = Game.mapSizeX/Game.mapSizeZ
end