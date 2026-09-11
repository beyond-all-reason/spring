-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html

local gadget = gadget ---@type Gadget

function gadget:GetInfo()
	return {
		name = "Ground Attack Stall Reproducer",
		desc = "Reproduces a Wolverine stalling on a ground attack",
		author = "local test setup",
		layer = 1000000,
		enabled = true,
	}
end

if not gadgetHandler:IsSyncedCode() then
	return
end

local spawnX = 2166.310
local spawnZ = 5476.650
local spawnDirection = { 0.139139, -0.001191, -0.990264 }
local attack = { 2688.928, 143.608, 4618.413 }

local attackerID
local attackerWeaponDefID
local projectilesCreated = 0
local projectilesAt900 = 0
local startX
local startZ

local function ReproducerEnabled()
	local value = Spring.GetModOptions().groundattackstallreproducer
	return value == true or value == 1 or value == "1"
end

local function CreateScenario()
	Spring.SetGlobalLos(0, true)

	local spawnY = Spring.GetGroundHeight(spawnX, spawnZ)
	attackerID = Spring.CreateUnit("corwolv", spawnX, spawnY, spawnZ, "south", 0)
	assert(attackerID, "Ground attack stall reproducer could not create corwolv")

	Spring.SetUnitDirection(attackerID, spawnDirection[1], spawnDirection[2], spawnDirection[3])
	Spring.GiveOrderToUnit(attackerID, CMD.FIRE_STATE, { 2 }, 0)
	Spring.GiveOrderToUnit(attackerID, CMD.MOVE_STATE, { 0 }, 0)

	local unitDefID = Spring.GetUnitDefID(attackerID)
	attackerWeaponDefID = UnitDefs[unitDefID].weapons[1].weaponDef
	Script.SetWatchProjectile(attackerWeaponDefID, true)

	startX, _, startZ = Spring.GetUnitPosition(attackerID)
	Spring.Echo(("GROUND_ATTACK_STALL_SCENARIO_READY attacker=%d weaponDef=%d"):format(attackerID, attackerWeaponDefID))
end

local function IssueAttack()
	Spring.GiveOrderToUnit(attackerID, CMD.ATTACK, attack, 0)
	Spring.Echo("GROUND_ATTACK_STALL_VALIDATION_START")
end

local function ReportResult()
	local x, _, z = Spring.GetUnitPosition(attackerID)
	local moved = math.sqrt((x - startX) ^ 2 + (z - startZ) ^ 2)
	local current = (Spring.GetUnitCommands(attackerID, 1) or {})[1]
	local params = current and current.params or {}
	local moveTypeData = Spring.GetUnitMoveTypeData(attackerID) or {}
	local testRange = current and Spring.GetUnitWeaponTestRange(attackerID, 1, params[1], params[2], params[3])
	local tryTarget = current and Spring.GetUnitWeaponTryTarget(attackerID, 1, params[1], params[2], params[3])
	local canFire = Spring.GetUnitWeaponCanFire(attackerID, 1)
	local stalled =
		current
		and projectilesCreated == 0
		and current.id == CMD.ATTACK
		and moveTypeData.progressState ~= "active"
		and testRange == true
		and tryTarget == false
		and canFire == false

	Spring.Echo(
		("GROUND_ATTACK_STALL_RESULT stalled=%s attacker=%d projectiles=%d command=%s target=%.3f,%.3f,%.3f moved=%.1f moveState=%s testRange=%s tryTarget=%s canFire=%s"):format(
			tostring(stalled), attackerID, projectilesCreated, tostring(current and current.id),
			params[1] or -1, params[2] or -1, params[3] or -1, moved,
			tostring(moveTypeData.progressState), tostring(testRange), tostring(tryTarget), tostring(canFire)
		)
	)
end

function gadget:Initialize()
	if not ReproducerEnabled() then
		gadgetHandler:RemoveGadget(self)
	end
end

function gadget:GameFrame(frame)
	if frame == 1 then
		CreateScenario()
	elseif frame == 30 then
		IssueAttack()
	elseif frame == 480 or frame == 900 or frame == 2100 then
		Spring.Echo("WEAPON_ARC_FRAME", frame, "HEADING", Spring.GetUnitHeading(attackerID))
		ReportResult()
		if frame == 900 then
			projectilesAt900 = projectilesCreated
		end
	elseif frame == 2101 then
		Spring.Echo("WEAPON_ARC_RESULT", (projectilesAt900 > 0 and projectilesCreated > projectilesAt900) and "PASS" or "FAIL", "projectiles", projectilesCreated)
		Spring.SendCommands("quitforce")
	end
end

function gadget:ProjectileCreated(_, ownerID, weaponDefID)
	if ownerID == attackerID and weaponDefID == attackerWeaponDefID then
		projectilesCreated = projectilesCreated + 1
	end
end
