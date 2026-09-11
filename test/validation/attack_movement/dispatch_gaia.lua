-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
-- Install as LuaGaia/main.lua in the isolated GAME archive. Quicksilver's
-- map-side main.lua enables LuaGaia; its mod-first loader picks this fixture.
if Spring.GetModOptions().attackmovementvalidation ~= "dispatch" then
	return VFS.Include("LuaGaia/main.lua", nil, VFS.MAP)
end
Spring.SetGameRulesParam("attack_dispatch_gaia_ready", 1)
function AttackCommandMovement(unitID)
	Spring.SetGameRulesParam("attack_dispatch_gaia", (Spring.GetGameRulesParam("attack_dispatch_gaia") or 0) + 1)
	return true
end
