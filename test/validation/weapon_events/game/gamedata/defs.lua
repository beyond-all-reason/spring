-- SPDX-License-Identifier: GPL-2.0-or-later
local units, weapons = {}, {}
for _, name in ipairs({ "single", "multi", "cancel", "blocked", "silent", "onlyfired", "beam" }) do
	weapons[name] = {
		name = name,
		weaponType = name == "beam" and "BeamLaser" or "Cannon",
		turret = true,
		firetolerance = 32768,
		range = 1000,
		reloadtime = 60,
		weaponVelocity = 600,
		burst = (name == "multi" or name == "cancel") and 3 or 1,
		burstrate = 0.2,
		projectiles = name == "multi" and 4 or 1,
		beamtime = 0.1,
		weaponTimer = 5,
		avoidFriendly = false,
		avoidFeature = false,
		avoidGround = false,
		collideFriendly = false,
		fireSubmersed = true,
		damage = { default = 1 },
	}
	local slot = { name = name, burstControlWhenOutOfArc = 1, fastQueryPointUpdate = true }
	units[name] = {
		name = name,
		objectName = "fir_tree_small.s3o",
		script = "weapon_events.lua", -- attached by the validation gadget
		maxDamage = 10000,
		explodeAs = name,
		selfDestructAs = name,
		footprintX = 1,
		footprintZ = 1,
		maxWaterDepth = 10000,
		canAttack = true,
		canCloak = true,
		decloakOnFire = true,
		cloakCost = 0,
		sightDistance = 1000,
		weapons = name == "multi" and { slot, slot } or { slot },
	}
end
return { unitdefs = units, weapondefs = weapons, featuredefs = {}, armordefs = {}, movedefs = {} }
