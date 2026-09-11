-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function gadget:GetInfo()
	return {
		name = "Command ended validation",
		desc = "Command lifecycle integration checks",
		author = "OpenAI Codex",
		layer = -2000000,
		enabled = true,
	}
end
if Spring.GetModOptions().attackmovementvalidation ~= "ended" then
	return false
end
if not gadgetHandler:IsSyncedCode() then
	function gadget:RecvFromSynced(name)
		if name == "ended_speed" then
			Spring.SendCommands("setspeed 1000")
		end
		if name == "ended_quit" then
			Spring.SendCommands("quitforce")
		end
	end
	return
end
local cases = {
	"removed_empty",
	"removed_wait",
	"removed_move",
	"targetLost_empty",
	"targetLost_move",
	"replaced",
	"inserted",
	"paused",
	"completed",
	"queued_removed",
}
local shooter, target, active, tag, events, stopX, stopZ
local finished = false
local function setup(name)
	active = nil
	if shooter and Spring.ValidUnitID(shooter) then
		Spring.DestroyUnit(shooter, false, true)
	end
	if target and Spring.ValidUnitID(target) then
		Spring.DestroyUnit(target, false, true)
	end
	Spring.SetHeightMapFunc(function()
		for x = 1800, 3200, 8 do
			for z = 1800, 3200, 8 do
				Spring.SetHeightMap(x, z, 200)
			end
		end
	end)
	shooter = assert(Spring.CreateUnit("corak", 2048, 200, 2048, 0, 0))
	target = assert(Spring.CreateUnit("armbanth", 2048, 200, 3000, 0, 1))
	for _, id in ipairs({ shooter, target }) do
		Spring.GiveOrderToUnit(id, CMD.FIRE_STATE, { 0 }, {})
	end
	active, events, tag = name, {}, nil
end
function gadget:Initialize()
	Spring.SetGlobalLos(0, true)
	Spring.SetGlobalLos(1, true)
end
function gadget:UnitCommandEnded(id, cmdID, cmdTag, reason)
	if id ~= shooter or cmdID ~= CMD.ATTACK then
		return
	end
	events[#events + 1] = { tag = cmdTag, reason = reason }
	assert(cmdTag == tag, "wrong finished tag")
	if active == "queued_removed" then
		error("inactive queued removal emitted event")
	end
	local current = Spring.GetUnitCurrentCommand(id)
	if active == "inserted" or active == "paused" then
		assert(current == CMD.WAIT, "inserted command must be visible")
	else
		assert(select(3, Spring.GetUnitCurrentCommand(id)) ~= tag, "ended command still current")
	end
	if active:find("_move", 1, true) then
		assert(current == CMD.MOVE, "next MOVE must be visible before execution")
	else
		-- Equivalent to CMobileCAI::StopMove: no CMD_STOP and no queue mutation.
		Spring.ClearUnitGoal(id, false)
	end
end
function gadget:AttackCommandMovement(id, cmdTag)
	if id == shooter and active == "completed" then
		tag = cmdTag
		Spring.SetUnitAttackMovement(id, "finish")
		return true
	end
	return false
end
function gadget:GameFrame(frame)
	if finished or frame < 1 then
		return
	end
	if frame == 1 then
		SendToUnsynced("ended_speed")
	end
	local index = math.floor((frame - 1) / 150) + 1
	local phase = (frame - 1) % 150
	if not cases[index] then
		finished = true
		Spring.Echo("[AttackValidation] PASS ended=" .. #cases)
		SendToUnsynced("ended_quit")
		return
	end
	if phase == 0 then
		setup(cases[index])
	end
	if phase == 30 then
		if active == "queued_removed" then
			Spring.GiveOrderToUnit(shooter, CMD.MOVE, { 3000, 200, 2048 }, {})
			Spring.GiveOrderToUnit(shooter, CMD.ATTACK, { target }, { "shift" })
			tag = Spring.GetUnitCommands(shooter, -1)[2].tag
		else
			Spring.GiveOrderToUnit(shooter, CMD.ATTACK, { target }, {})
			if active ~= "completed" then
				tag = select(3, Spring.GetUnitCurrentCommand(shooter))
			end
			if active:find("_move", 1, true) then
				Spring.GiveOrderToUnit(shooter, CMD.MOVE, { 3000, 200, 2048 }, { "shift" })
			end
			if active == "removed_wait" then
				Spring.GiveOrderToUnit(shooter, CMD.WAIT, {}, { "shift" })
			end
		end
	end
	if phase == 60 then
		if active:find("removed", 1, true) then
			Spring.GiveOrderToUnit(shooter, CMD.REMOVE, { tag }, {})
		elseif active:find("targetLost", 1, true) then
			Spring.DestroyUnit(target, false, true)
		elseif active == "replaced" then
			Spring.GiveOrderToUnit(shooter, CMD.MOVE, { 3000, 200, 2048 }, {})
		elseif active == "paused" then
			Spring.GiveOrderToUnit(shooter, CMD.WAIT, {}, {})
		elseif active == "inserted" then
			Spring.GiveOrderToUnit(shooter, CMD.INSERT, { 0, CMD.WAIT, 0 }, { "alt" })
		end
		local x, _, z = Spring.GetUnitPosition(shooter)
		stopX, stopZ = x, z
	end
	if phase == 140 then
		local expected = active:match("^(%w+)_") or active
		if active == "replaced" or active == "inserted" or active == "paused" then
			expected = "interrupted"
		end
		if active == "queued_removed" then
			assert(#events == 0)
		else
			assert(#events == 1, active .. ": expected one event, got " .. #events)
			assert(events[1].reason == expected, active .. ": " .. events[1].reason)
		end
		if active:find("_empty", 1, true) or active == "removed_wait" then
			local x, _, z = Spring.GetUnitPosition(shooter)
			assert((x - stopX) ^ 2 + (z - stopZ) ^ 2 < 100, "old attack movement continued")
		end
		if active:find("_move", 1, true) or active == "replaced" or active == "queued_removed" then
			assert(Spring.GetUnitCurrentCommand(shooter) == CMD.MOVE, "next move was lost")
		end
		Spring.Echo("[CommandEndedCase] PASS " .. active)
	end
end
for _, name in ipairs({ "GameFrame", "UnitCommandEnded", "AttackCommandMovement" }) do
	local run = gadget[name]
	gadget[name] = function(...)
		local ok, result = pcall(run, ...)
		if not ok then
			finished = true
			Spring.Echo("[AttackValidation] FAIL " .. tostring(result))
			SendToUnsynced("ended_quit")
			return true
		end
		return result
	end
end
