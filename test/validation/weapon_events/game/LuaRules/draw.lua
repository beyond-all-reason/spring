-- SPDX-License-Identifier: GPL-2.0-or-later
function Update()
	if Spring.GetGameFrame() < 0 then
		Spring.SendCommands("forcestart")
	end
end
function RecvFromSynced(message)
	if message == "weapon_events_done" then
		Spring.SendCommands("quitforce")
	end
end
