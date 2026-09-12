-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return { name = "Clear attackers validation", layer = 0, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then return false end

local failed, checks = false, 0
local attackers, all, expected = {}, {}, {}
local a, b, c, queued, split, weaponOnly, ground, empty, auto, autoTarget
local function check(ok, message)
	checks = checks + 1
	if not ok then
		failed = true
		Spring.Echo("CLEAR_ATTACKERS FAIL: " .. message)
	end
end
local function target(u, slot)
	local kind, _, id = Spring.GetUnitWeaponTarget(u, slot or 1)
	return kind == 1 and id or nil
end
local function create(name, x, z, team)
	local y = math.max(0, Spring.GetGroundHeight(x, z)) + 200
	local u = assert(Spring.CreateUnit(name, x, y, z, 0, team))
	all[#all + 1] = u
	-- Strafing-aircraft command AI pauses while MoveCtrl is active.
	if name ~= "fighter" then
		Spring.MoveCtrl.Enable(u)
		Spring.MoveCtrl.SetPosition(u, x, y, z)
	end
	Spring.SetUnitAlwaysVisible(u, true)
	Spring.GiveOrderToUnit(u, CMD.FIRE_STATE, { 0 }, 0)
	Spring.UnitScript.CreateScript(u, {
		QueryWeapon = function() return 1 end,
		AimFromWeapon = function() return 1 end,
		AimWeapon = function(w) Spring.SetUnitWeaponState(u, w, "aimReady", 1) end,
		BlockShot = function() return true end, -- Keep targets alive and queues stable.
	})
	return u
end
local function attack(u, id, shift)
	Spring.GiveOrderToUnit(u, CMD.ATTACK, { id }, shift and { "shift" } or 0)
end
local function commands(u)
	return Spring.GetUnitCommands(u, -1)
end
local function remember(u)
	local keep = {}
	for _, cmd in ipairs(commands(u)) do
		if not (#cmd.params == 1 and cmd.params[1] == a and
			(cmd.id == CMD.ATTACK or cmd.id == CMD.FIGHT or cmd.id == CMD.MANUALFIRE)) then
			keep[#keep + 1] = cmd.tag
		end
	end
	expected[u] = keep
end
local function checkQueues()
	for u, tags in pairs(expected) do
		local queue = commands(u)
		check(#queue == #tags, "remaining queue length for " .. u)
		for i, tag in ipairs(tags) do
			check(queue[i] and queue[i].tag == tag, "remaining command order/tag for " .. u)
		end
	end
end

function gadget:Initialize()
	check(type(Spring.ClearUnitAttackers) == "function", "missing ClearUnitAttackers API")
end

function gadget:GameFrame(frame)
	if failed then SendToUnsynced("clear_attackers_done"); return end
	local x, z = Game.mapSizeX / 2, Game.mapSizeZ / 2
	if frame == 1 then
		a = create("target", x + 300, z, 1)
		b = create("target", x + 300, z + 100, 1)
		c = create("target", x + 300, z + 200, 1)
		for i, name in ipairs({ "static", "mobile", "fighter" }) do
			attackers[i] = create(name, x, z + (i - 1) * 100, i == 3 and 2 or 0)
		end
		queued = create("static", x - 50, z, 0)
		split = create("static", x - 100, z, 0)
		weaponOnly = create("static", x - 150, z, 2)
		ground = create("static", x - 200, z, 0)
		empty = create("static", x - 250, z, 0)
		-- Out of range of B/C: automatic acquisition can only choose this target.
		auto = create("static", 200, 200, 0)
		autoTarget = create("target", 500, 200, 1)
		Spring.GiveOrderToUnit(auto, CMD.FIRE_STATE, { 2 }, 0)
	elseif frame == 32 then
		for _, u in ipairs(attackers) do
			attack(u, a); attack(u, b, true); attack(u, c, true)
			-- INSERT preserves a duplicate rather than toggling the earlier order.
			Spring.GiveOrderToUnit(u, CMD.INSERT, { -1, CMD.ATTACK, 0, a }, { "alt" })
		end
		attack(queued, b); attack(queued, a, true)
		Spring.GiveOrderToUnit(queued, CMD.MANUALFIRE, { a }, { "shift" })
		Spring.GiveOrderToUnit(queued, CMD.FIGHT, { a }, { "shift" })
		attack(split, b)
		Spring.GiveOrderToUnit(ground, CMD.ATTACK, { x + 300, 300, z }, 0)
		attack(ground, a, true)
		attack(empty, a)
	elseif frame == 64 then
		check(Script.GetFullCtrl(), "synced LuaRules has full control")
		check(Spring.ClearUnitAttackers(-1) == nil, "invalid ID rejected")
		for _, u in ipairs(attackers) do check(target(u) == a, "active attack on A before clearing") end
		check(target(queued) == b, "queued-only attacker still targets B")
		local queueBefore = commands(queued)
		check(#queueBefore == 4, "queued ATTACK/MANUALFIRE/FIGHT fixture")
		for _, team in ipairs({ 0, 1, Script.NO_ACCESS_TEAM }) do
			CallAsTeam({ ctrl = team }, function()
				check(not Script.GetFullCtrl(), "restricted context lacks full control")
				check(Spring.ClearUnitAttackers(a) == nil, "restricted context rejected")
			end)
			check(target(attackers[1]) == a and #commands(queued) == #queueBefore, "denied call has no effects")
		end
		Spring.SetUnitTarget(split, a, false, true, 1)
		Spring.SetUnitTarget(weaponOnly, a, false, true, 1)
		check(target(split, 1) == a and target(split, 2) == b, "independent weapon targets fixture")
		check(target(weaponOnly) == a and #commands(weaponOnly) == 0, "weapon-only target fixture")
		check(target(auto) == autoTarget, "automatic target acquired")
		check(Spring.ClearUnitAttackers(autoTarget) == true and target(auto) == nil, "automatic target cleared")
		for _, u in ipairs(all) do remember(u) end
		local neutral = Spring.GetUnitNeutral(a)
		-- Unit scripts retain full control in CallAsUnit, just like their LuaRules owner.
		Spring.UnitScript.CallAsUnit(a, function()
			check(Script.GetFullCtrl(), "unit script context retains full control")
			check(Spring.ClearUnitAttackers(a) == true, "unit script context can clear attackers")
		end)
		check(Spring.GetUnitNeutral(a) == neutral, "neutral state unchanged")
		checkQueues()
		for _, u in ipairs(all) do
			for w = 1, #UnitDefs[Spring.GetUnitDefID(u)].weapons do
				check(target(u, w) ~= a, "no weapon still targets A")
			end
		end
		check(target(split, 2) == b, "other weapon target preserved")
		check(Spring.GetUnitWeaponTarget(ground, 1) == 2, "ground target preserved")
		check(Spring.ClearUnitAttackers(a) == true, "repeated call succeeds")
		checkQueues()
	elseif frame == 96 then
		for _, u in ipairs(attackers) do check(target(u) == b, "next attack B initialized") end
		check(#commands(empty) == 0 and target(empty) == nil, "sole attack cleared")
		check(target(auto) == autoTarget, "automatic reacquisition remains possible")
		Spring.DestroyUnit(a, false, true)
	elseif frame == 128 then
		for _, u in ipairs(attackers) do check(target(u) == b, "A's deletion does not skip B") end
		attack(weaponOnly, b)
	elseif frame == 160 then
		check(target(weaponOnly) == b, "cleared attacker accepts new orders")
		if not failed then Spring.Echo("CLEAR_ATTACKERS PASS checks=" .. checks) end
		SendToUnsynced("clear_attackers_done")
	end
end
