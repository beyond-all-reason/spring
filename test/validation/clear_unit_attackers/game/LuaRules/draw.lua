-- This file is part of the Spring engine (GPL v2 or later), see LICENSE.html.
function Update()
	if Spring.GetGameFrame() < 0 then
		Spring.SendCommands("forcestart")
	end
end
function RecvFromSynced(message)
	if message == "clear_attackers_done" then
		Spring.Echo(Spring.ClearUnitAttackers == nil and "CLEAR_ATTACKERS UNSYNCED PASS" or "CLEAR_ATTACKERS FAIL: exposed to unsynced")
		Spring.SendCommands("quitforce")
	end
end
