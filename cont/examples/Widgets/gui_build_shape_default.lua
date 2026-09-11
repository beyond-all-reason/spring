function widget:GetInfo()
	return {
		name = "Build Shape: Engine Defaults",
		desc = "Reimplements the default build-drag shapes via the GetBuildShape callin",
		author = "RecoilEngine contributors",
		date = "2026",
		license = "GNU GPL, v2 or later",
		layer = 0,
		enabled = false,
	}
end

function widget:GetBuildShape(unitDefID, facing, startX, startY, startZ, endX, endY, endZ)
	local alt, ctrl, meta, shift = Spring.GetModKeyState()

	-- dragging out more than one building requires the queue key
	local queue = shift
	if Spring.GetConfigInt("InvertQueueKey", 0) ~= 0 then
		queue = not shift
	end

	if not queue then
		-- no queue key means no drag: always a single building, so none of the
		-- drag shapes below can be reached without it
		return "single"
	end

	-- a drag too small to fit a second building degrades to "single" engine-side,
	-- so the drag shapes below need no size checks of their own

	if ctrl and alt then
		return "hollowbox"
	end

	if alt then
		-- fill the dragged box with buildings
		return "flood"
	end

	if ctrl then
		local mx, my = Spring.GetMouseState()
		local thingType = Spring.TraceScreenRay(mx, my)
		return thingType == "unit" and "surround" or "cardinalline"
	end

	-- queue key alone: a free-angle line following the drag
	return "freeangleline"
end
