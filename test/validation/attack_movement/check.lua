-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return {
		name = "Attack movement validation",
		desc = "Opt-in integration checks",
		author = "OpenAI Codex",
		layer = -1000000,
		enabled = true,
	}
end
local options = Spring.GetModOptions()
local mode = options.attackmovementvalidation
if not mode then
	return false
end
if not gadgetHandler:IsSyncedCode() then
	function gadget:RecvFromSynced(name)
		if name == "attack_validation_speed" then
			Spring.SendCommands("setspeed 1000")
		end
		if name == "attack_validation_quit" then
			Spring.SendCommands("quitforce")
		end
	end
	return
end
local endFrame = tonumber(options.attackmovementendframe) or 36000
local cases, created, features = {}, {}, {}
local active, shooter, target, observed
local completed = 0
local validReasons = {
	clear = true,
	notChecked = true,
	invalidTarget = true,
	range = true,
	terrain = true,
	friendly = true,
	neutral = true,
	feature = true,
	blocked = true,
}
local function terrain(height, x1, z1, x2, z2)
	Spring.SetHeightMapFunc(function()
		for x = x1, x2, 8 do
			for z = z1, z2, 8 do
				Spring.SetHeightMap(x, z, height)
			end
		end
	end)
end
local function create(name, x, z, team)
	local id = assert(Spring.CreateUnit(name, x, Spring.GetGroundHeight(x, z), z, 0, team), name)
	created[#created + 1] = id
	Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, {})
	Spring.GiveOrderToUnit(id, CMD.MOVE_STATE, { 0 }, {})
	for w = 1, #UnitDefs[Spring.GetUnitDefID(id)].weapons do
		Spring.SetUnitWeaponState(id, w, "reloadState", 1000000)
	end
	return id
end
local function startCase(case)
	for _, id in ipairs(created) do
		if Spring.ValidUnitID(id) then
			Spring.DestroyUnit(id, false, true)
		end
	end
	for _, id in ipairs(features) do
		Spring.DestroyFeature(id)
	end
	created, features = {}, {}
	terrain(200, 1536, 1536, 3072, 3072)
	active, observed = case, false
	shooter = create(case.weapon, 2048, 2048, 0)
	target = create("armbanth", 2048, 2248, 1)
	if case.blocker == "terrain" then
		terrain(1000, 1920, 2104, 2176, 2184)
	elseif case.blocker == "friendly" or case.blocker == "neutral" or case.blocker == "mixed" then
		local blocker = create("armbanth", 2048, case.weapon == "armmerl" and 2048 or 2148, 0)
		Spring.SetUnitCollisionVolumeData(blocker, 200, 1200, 200, 0, 500, 0, 2, 1, 1)
		if case.blocker == "mixed" then
			terrain(1000, 1920, 2104, 2176, 2184)
			Spring.MoveCtrl.Enable(blocker)
			Spring.MoveCtrl.SetPosition(blocker, 2048, 200, 2148)
		end
		if case.blocker == "neutral" then
			Spring.TransferUnit(blocker, 1, false)
			Spring.SetUnitNeutral(blocker, true)
		end
	elseif case.blocker == "feature" then
		local id =
			assert(Spring.CreateFeature("armbanth_dead", 2048, 200, case.weapon == "armmerl" and 2048 or 2148, 0, 0))
		features[#features + 1] = id
		Spring.SetFeatureCollisionVolumeData(id, 200, 1200, 200, 0, 500, 0, 2, 1, 1)
	end
end
function gadget:Initialize()
	assert(Spring.GetUnitAttackMovementState and Spring.GetUnitAttackWeaponState and Spring.SetUnitAttackMovement)
	if mode ~= "blockers" then
		gadgetHandler:RemoveCallIn("AttackCommandMovement")
		return
	end
	Spring.SetGlobalLos(0, true)
	Spring.SetGlobalLos(1, true)
	for _, weapon in ipairs({ "corak", "cormort", "corstorm", "corban", "armmerl" }) do
		for _, blocker in ipairs({ "clear", "friendly", "neutral", "feature", "terrain", "mixed" }) do
			if not (weapon == "armmerl" and (blocker == "terrain" or blocker == "mixed")) then
				for _, ground in ipairs({ false, true }) do
					cases[#cases + 1] = { weapon = weapon, blocker = blocker, ground = ground }
				end
			end
		end
	end
end
function gadget:AttackCommandMovement(unitID)
	if unitID ~= shooter then
		return false
	end
	Spring.SetUnitAttackMovement(unitID, "stop")
	local state = Spring.GetUnitAttackMovementState(unitID)
	assert(state.object == not active.ground)
	local native = { Spring.GetUnitAttackWeaponState(unitID, 1) }
	local storedFlags = Spring.GetUnitWeaponState(unitID, 1, "avoidFlags")
	local eligible, rotate, heading, _, _, rotateReason, headingReason = Spring.GetUnitAttackWeaponState(unitID, 1, 0)
	local flags = Game.collisionFlags
	local friendly =
		{ Spring.GetUnitAttackWeaponState(unitID, 1, flags.noGround + flags.noNeutrals + flags.noFeatures + flags.noCloaked) }
	local ground = { Spring.GetUnitAttackWeaponState(unitID, 1, flags.noUnits + flags.noFeatures + flags.noCloaked) }
	local after = { Spring.GetUnitAttackWeaponState(unitID, 1) }
	assert(storedFlags == Spring.GetUnitWeaponState(unitID, 1, "avoidFlags"))
	for i = 1, #native do
		assert(native[i] == after[i], "filter leaked into native query")
	end
	local reasonIndex = active.ground and 7 or 6
	local friendlyExpected = (active.blocker == "friendly" or active.blocker == "mixed") and "friendly" or "clear"
	local groundExpected = (active.blocker == "terrain" or active.blocker == "mixed") and "terrain" or "clear"
	assert(friendly[reasonIndex] == friendlyExpected, "friendly-only: " .. tostring(friendly[reasonIndex]))
	assert(ground[reasonIndex] == groundExpected, "terrain-only: " .. tostring(ground[reasonIndex]))
	assert(eligible and validReasons[rotateReason] and validReasons[headingReason])
	assert(heading == (headingReason == "clear"))
	if not active.ground then
		assert(rotate == (rotateReason == "clear"))
	end
	local reason = active.ground and headingReason or rotateReason
	if reason == (active.blocker == "mixed" and "terrain" or active.blocker) then
		observed = true
	end
	Spring.Echo(
		"[AttackValidationReason]",
		active.weapon,
		active.blocker,
		active.ground and "ground" or "unit",
		rotateReason,
		headingReason
	)
	assert(not pcall(Spring.GetUnitAttackWeaponState, unitID, 0))
	assert(not pcall(Spring.GetUnitAttackMovementState, target))
	return true
end
local function snapshot(frame)
	local sum, weighted, ids = 1, 0, Spring.GetAllUnits()
	table.sort(ids)
	for _, id in ipairs(ids) do
		local x, y, z = Spring.GetUnitPosition(id)
		local cmd, opts, tag = Spring.GetUnitCurrentCommand(id)
		local value =
			string.format("%d:%.9g:%.9g:%.9g:%s:%s:%s;", id, x, y, z, tostring(cmd), tostring(opts), tostring(tag))
		-- Engine Lua uses float numbers: keep both checksum sums below 2^24.
		for i = 1, #value do
			sum = (sum + value:byte(i)) % 65521
			weighted = (weighted + sum) % 65521
		end
	end
	Spring.Echo(
		string.format("[AttackValidationSnapshot] frame=%d units=%d digest=%.0f:%.0f", frame, #ids, sum, weighted)
	)
end
function gadget:GameFrame(frame)
	if frame <= 0 then
		return
	end
	if frame == 1 then
		SendToUnsynced("attack_validation_speed")
	end
	if mode == "blockers" then
		local index = math.floor((frame - 1) / 90) + 1
		local phase = (frame - 1) % 90
		if not cases[index] then
			assert(completed == #cases)
			Spring.Echo("[AttackValidation] PASS blockers=" .. #cases)
			SendToUnsynced("attack_validation_quit")
			return
		end
		if phase == 0 then
			startCase(cases[index])
		end
		if phase == 30 then
			local params = active.ground and { 2048, 200, 2248 } or { target }
			Spring.GiveOrderToUnit(shooter, CMD.ATTACK, params, {})
			Spring.Echo(
				"[AttackValidationOrder]",
				shooter,
				Spring.GetUnitCurrentCommand(shooter),
				Spring.GetUnitIsStunned(shooter)
			)
		end
		if phase == 89 then
			assert(
				observed,
				active.weapon .. ": expected " .. active.blocker .. (active.ground and " ground" or " unit")
			)
			assert(not pcall(Spring.GetUnitAttackMovementState, shooter))
			completed = completed + 1
		end
	else
		if frame % 1800 == 0 then
			snapshot(frame)
		end
		if frame >= endFrame then
			Spring.Echo("[AttackValidation] PASS replay")
			SendToUnsynced("attack_validation_quit")
		end
	end
end

-- A failed check must stop the accelerated run rather than produce an error loop.
for _, name in ipairs({ "GameFrame", "AttackCommandMovement" }) do
	local run = gadget[name]
	gadget[name] = function(...)
		local ok, result = pcall(run, ...)
		if not ok then
			Spring.Echo("[AttackValidation] FAIL " .. tostring(result))
			SendToUnsynced("attack_validation_quit")
			return true
		end
		return result
	end
end
