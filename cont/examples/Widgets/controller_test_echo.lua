function widget:GetInfo()
	return {
		name      = "Controller Test Echo",
		desc      = "Echoes all controller events and state for debugging controller input implementation",
		author    = "beherith",
		date      = "Jul 2026",
		license   = "GPL v2+",
		layer     = 0,
		enabled   = false,  -- set to true to enable
	}
end

local connectedControllers = {}
local statePollInterval    = 5  -- frames between state polls
local statePollCounter     = 0

function widget:Initialize()
	Spring.Echo("[ControllerTest] Widget initialized")
	Spring.Echo("[ControllerTest] Available controllers:")

	local controllers = Spring.GetAvailableControllers()
	if controllers and next(controllers) then
		for i, info in ipairs(controllers) do
			Spring.Echo(string.format("  [%d] joystickId=%d  name=%s",
				i, info.instanceId, info.name or "unknown"))
		end

		-- Auto-connect all available controllers
		for i, info in ipairs(controllers) do
			local instanceId = Spring.ConnectController(info.instanceId)
			if instanceId then
				connectedControllers[instanceId] = true
				Spring.Echo(string.format("[ControllerTest] Auto-connected joystickId %d -> instanceId %d", info.instanceId, instanceId))
			else
				Spring.Echo(string.format("[ControllerTest] FAILED to connect joystickId %d", info.instanceId))
			end
		end
	else
		Spring.Echo("  None found")
	end
end

function widget:Shutdown()
	for instanceId in pairs(connectedControllers) do
		Spring.DisconnectController(instanceId)
	end
	connectedControllers = {}
	Spring.Echo("[ControllerTest] Widget shut down, all controllers disconnected")
end

function ControllerDevice(eventName, instanceId)
	Spring.Echo(string.format("[ControllerTest] DEVICE EVENT: %s | instanceId=%d", eventName, instanceId))

	if eventName == "Connected" then
		connectedControllers[instanceId] = true
	elseif eventName == "Disconnected" then
		connectedControllers[instanceId] = nil
	elseif eventName == "Added" then
		Spring.Echo(string.format("[ControllerTest]   -> Device added (SDL joystick index=%d). Call Spring.ConnectController(%d) to use it.", instanceId, instanceId))
		-- Auto-connect newly added controllers
		local iid = Spring.ConnectController(instanceId)
		if iid then
			connectedControllers[iid] = true
			Spring.Echo(string.format("[ControllerTest]   -> Auto-connected -> instanceId=%d", iid))
		end
	elseif eventName == "Removed" then
		Spring.Echo(string.format("[ControllerTest]   -> Device removed (SDL joystick index=%d)", instanceId))
	end

	return true
end

function ControllerState(eventName, instanceId, statefulId, value, statefulName)
	if eventName == "AxisMotion" then
		Spring.Echo(string.format("[ControllerTest] AXIS: instanceId=%d | axis=%d (%s) | value=%d",
			instanceId, statefulId, statefulName, value))
	elseif eventName == "ButtonDown" then
		Spring.Echo(string.format("[ControllerTest] BTN_DOWN: instanceId=%d | button=%d (%s)",
			instanceId, statefulId, statefulName))
	elseif eventName == "ButtonUp" then
		Spring.Echo(string.format("[ControllerTest] BTN_UP:   instanceId=%d | button=%d (%s)",
			instanceId, statefulId, statefulName))
	end

	return true
end

function widget:Update()
	statePollCounter = statePollCounter + 1
	if statePollCounter >= statePollInterval then
		statePollCounter = 0
		for instanceId in pairs(connectedControllers) do
			local state = Spring.GetControllerState(instanceId)
			Spring.Echo(string.format("[ControllerTest] POLL %d: state=%s", instanceId, state and "valid" or "nil"))
			if state then
				local axisStr  = ""
				local btnStr   = ""

				if state.axis then
					local parts = {}
					for name, val in pairs(state.axis) do
						table.insert(parts, string.format("%s=%d", name, val))
					end
					axisStr = table.concat(parts, ", ")
				end

				if state.buttons then
					local parts = {}
					for name, val in pairs(state.buttons) do
						if val == 1 then
							table.insert(parts, name)
						end
					end
					btnStr = table.concat(parts, ", ")
				end

				if axisStr ~= "" or btnStr ~= "" then
					Spring.Echo(string.format("[ControllerTest] POLL %d: axis=[%s] buttons=[%s]",
						instanceId, axisStr, btnStr))
				end
			else
				Spring.Echo(string.format("[ControllerTest] POLL %d: state=nil (disconnected?)", instanceId))
				connectedControllers[instanceId] = nil
			end
		end
	end
end
