-- SPDX-License-Identifier: GPL-2.0-or-later
function gadget:GetInfo()
	return { name = "Weapon event validation", layer = 0, enabled = true }
end
if not gadgetHandler:IsSyncedCode() then
	return false
end

local cases, failure = {}, false
local names = { "single", "multi", "cancel", "blocked", "silent", "onlyfired", "beam", "zero" }
local function check(ok, message)
	if not ok then
		failure = true
		Spring.Echo("WEAPON_EVENTS FAIL: " .. message)
	end
end

function gadget:Initialize()
	check(type(Script.SetWatchWeaponFired) == "function", "missing shot watch API")
	if failure then
		return
	end
	for _, name in ipairs(names) do
		local wd = WeaponDefNames[name].id
		check(not Script.GetWatchWeaponFired(wd), "shot watch defaults off")
		check(not Script.GetWatchWeaponBurst(wd), "burst watch defaults off")
		Script.SetWatchWeapon(wd, true)
		check(Script.GetWatchWeapon(wd), "legacy setter enables legacy watches")
		check(not Script.GetWatchWeaponFired(wd), "legacy setter leaves shot watch off")
		check(not Script.GetWatchWeaponBurst(wd), "legacy setter leaves burst watch off")
		Script.SetWatchWeapon(wd, false)
		Script.SetWatchWeaponFired(wd, true)
		check(not Script.GetWatchWeapon(wd), "legacy getter excludes shot watch")
		check(not Script.GetWatchWeaponBurst(wd), "shot watch leaves burst watch off")
		Script.SetWatchWeaponFired(wd, false)
		Script.SetWatchWeaponBurst(wd, true)
		check(not Script.GetWatchWeapon(wd), "legacy getter excludes burst watch")
		check(not Script.GetWatchWeaponFired(wd), "burst watch leaves shot watch off")
		Script.SetWatchWeaponFired(wd, true)
		Script.SetWatchWeapon(wd, false)
		check(Script.GetWatchWeaponFired(wd), "legacy setter leaves shot watch on")
		check(Script.GetWatchWeaponBurst(wd), "legacy setter leaves burst watch on")
		Script.SetWatchWeaponFired(wd, name ~= "silent")
		Script.SetWatchWeaponBurst(wd, name ~= "onlyfired")
		Script.SetWatchProjectile(wd, true)
	end
end

function gadget:GameFrame(frame)
	if failure then
		SendToUnsynced("weapon_events_done")
		return
	end
	if frame == 1 then
		for i, name in ipairs(names) do
			local x, z = Game.mapSizeX / 2, Game.mapSizeZ / 2 + i * 64
			local u = Spring.CreateUnit(name, x, Spring.GetGroundHeight(x, z), z, 0, 0)
			check(u ~= nil, "create " .. name)
			if u then
				cases[u] = { name = name, starts = 0, fired = 0, ends = 0, projectiles = 0, scriptShots = 0 }
				Spring.UnitScript.CreateScript(u, {
					QueryWeapon = function()
						return 1
					end,
					AimFromWeapon = function()
						return 1
					end,
					AimWeapon = function(w)
						Spring.SetUnitWeaponState(u, w, "aimReady", 1)
					end,
					BlockShot = function(w)
						return name == "blocked" or (name == "multi" and w == 1)
					end,
					Shot = function()
						cases[u].scriptShots = cases[u].scriptShots + 1
					end,
				})
				-- The single-shot case keeps cloak intent off, reproducing the missing
				-- notification context from BAR #6618. The multi case starts cloaked.
				Spring.SetUnitCloak(u, name == "multi" and 1 or 0)
				local y = math.max(0, Spring.GetGroundHeight(x, z), Spring.GetGroundHeight(x + 300, z)) + 100
				Spring.MoveCtrl.Enable(u)
				Spring.MoveCtrl.SetPosition(u, x, y, z)
				cases[u].target = { x + 300, y, z }
			end
		end
	end
	for u, case in pairs(cases) do
		if frame == 32 then
			check(Spring.GetUnitIsCloaked(u) == (case.name == "multi"), case.name .. " initial cloak state")
			Spring.GiveOrderToUnit(u, CMD.ATTACK, case.target, 0)
		end

		if case.name == "cancel" and case.fired == 2 and not case.dropped then
			-- Drop the target between the second and third shots, outside callbacks.
			Spring.GiveOrderToUnit(u, CMD.STOP, {}, 0)
			Spring.SetUnitTarget(u, nil)
			case.dropped = true
		end
	end
end

local function eventCase(u, ud, team, weapon)
	local case = cases[u]
	if not case then
		return
	end
	check(ud == UnitDefNames[case.name].id and team == 0, "unit identity")
	check(weapon == (case.name == "multi" and 2 or 1), "one-based weapon slot")
	return case
end

function gadget:UnitWeaponBurstStart(u, ud, team, weapon)
	local case = eventCase(u, ud, team, weapon)
	if case then
		case.starts = case.starts + 1
		check(case.fired == 0 and case.projectiles == 0, "start precedes firing")
	end
end

function gadget:ProjectileCreated(_, owner)
	local case = cases[owner]
	if case then
		case.projectiles = case.projectiles + 1
	end
end

function gadget:UnitWeaponFired(u, ud, team, weapon)
	local case = eventCase(u, ud, team, weapon)
	if not case then
		return
	end
	case.fired = case.fired + 1
	check(case.ends == 0, "shot precedes burst end")
	local perShot = case.name == "zero" and 0 or case.name == "multi" and 4 or 1
	check(case.scriptShots == case.fired * perShot, "shot event follows all script Shot calls")
	if case.name ~= "beam" then
		check(case.projectiles == case.fired * perShot, "shot event follows all projectiles")
	end
end

function gadget:UnitWeaponBurstEnd(u, ud, team, weapon)
	local case = eventCase(u, ud, team, weapon)
	if case then
		case.ends = case.ends + 1
	end
end

function gadget:GameFramePost(frame)
	if frame ~= 120 then
		return
	end
	for _, case in pairs(cases) do
		local shots = case.name == "multi" and 3
			or case.name == "cancel" and 2
			or case.name == "beam" and 3
			or case.name == "blocked" and 0
			or 1
		local bursts = case.name == "blocked" or case.name == "onlyfired"
		check(case.starts == (bursts and 0 or 1), case.name .. " start count")
		check(case.ends == (bursts and 0 or 1), case.name .. " end count")
		check(case.fired == (case.name == "silent" and 0 or shots), case.name .. " fired count")
		if case.name ~= "beam" then
			check(
				case.projectiles == shots * (case.name == "zero" and 0 or case.name == "multi" and 4 or 1),
				case.name .. " projectile count"
			)
		end
		Spring.Echo(
			string.format(
				"WEAPON_EVENTS %s: start=%d fired=%d end=%d projectiles=%d",
				case.name,
				case.starts,
				case.fired,
				case.ends,
				case.projectiles
			)
		)
	end
	if not failure then
		Spring.Echo("WEAPON_EVENTS PASS")
	end
	SendToUnsynced("weapon_events_done")
end
