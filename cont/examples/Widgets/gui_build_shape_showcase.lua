--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--  Demonstrates:
--    - Using the GetBuildShape callin to pick the shape a build-drag traces out
--      from what is being built and how the drag looks, with no modifier keys
--
--  The engine asks GetBuildShape while a building placement is active; see
--  gui_build_shape_default.lua for the callin basics and an exact reimplementation
--  of the default modifier-key behaviour. This example instead ignores the
--  modifiers entirely (dragging always paints, no queue key needed):
--
--    - metal extractors snap to spots, so they never paint: "single"
--    - armed buildings ring whatever building the cursor points at ("surround"),
--    - a squarish drag covers an area: filled with buildings while small
--      ("flood"), only fenced off along its perimeter once covering it solid
--      would get silly ("hollowbox")
--    - an elongated drag traces a line: locked to its axis when dragged close
--      to one ("cardinalline"), following the drag freely otherwise
--      ("freeangleline")
--
--  Holding meta hands the decision back to the engine, because returning nil
--  (or any unknown name) keeps the default modifier-key behaviour.
--
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------

function widget:GetInfo()
	return {
		name = "Build Shape: Showcase",
		desc = "Selects build-drag shapes from the building and drag geometry instead of modifier keys",
		author = "RecoilEngine contributors",
		date = "2026",
		license = "GNU GPL, v2 or later",
		layer = 0,
		enabled = false,
	}
end

-- a squarish drag spanning more area than this (in elmos²) is fenced off
-- along its perimeter instead of being filled solid
local FENCE_AREA = 512 * 512

function widget:GetBuildShape(unitDefID, facing, startX, startY, startZ, endX, endY, endZ)
	local alt, ctrl, meta, shift = Spring.GetModKeyState()

	-- escape hatch: nil keeps the engine's default modifier-key behaviour
	if meta then
		return nil
	end

	local unitDef = UnitDefs[unitDefID]

	-- extractors snap to metal spots; painting rows of them makes no sense
	if unitDef.extractsMetal > 0 then
		return "single"
	end

	-- armed buildings ring the building under the cursor, degrading to an
	-- axis-locked line when the cursor points at nothing
	if #unitDef.weapons > 0 then
		return "surround"
	end

	local dx = math.abs(endX - startX)
	local dz = math.abs(endZ - startZ)
	local long = math.max(dx, dz)
	local short = math.min(dx, dz)

	if short > 0.5 * long then
		-- a squarish drag covers the area
		return (dx * dz > FENCE_AREA) and "hollowbox" or "flood"
	end

	if short < 0.2 * long then
		-- dragged close to a map axis: lock the line to it
		return "cardinalline"
	end

	-- an elongated drag at a proper angle: a free line following it
	return "freeangleline"
end
