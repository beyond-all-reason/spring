+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2026.07, for **pre-release 2026.08**.

# Caveats
These are the entries which may require special attention when migrating:
* Cannon, MissileLauncher and StarburstLauncher weapons no longer test line of fire from
the current muzzle position before aiming. Like every other weapon type they now use the
`AimFromWeapon` piece, or the new `aimFromEstimate` unit def tag when the unit provides one.
* `Spring.GetUnitWeaponTryTarget(unitID, weaponNum, x, y, z)` used to test position (0, 0, 0)
regardless of the given coordinates and therefore always returned `false`. It now tests the
given position.

# Fixes
* Line-of-fire and other synced ground traces (`TraceRay`, `CWeapon::HaveFreeLineOfFire`,
`Spring.GetUnitWeaponHaveFreeLineOfFire`) no longer report a free line when the ray starts
below the terrain. `LineGroundCol` returns a hit distance of 0 for such a ray and `TraceRay`
discarded that as "no hit", so a weapon whose muzzle or aim-from piece was inside a cliff
believed it could shoot through it, stopped, and never fired. Cannons had the same problem
with their trajectory trace and now reject a source below the interpolated terrain height
outright, the test the fire-time check already applies to the muzzle.
* The underground test of `LineGroundCol` (also behind `Spring.TraceRayGround*`) compares the
ray origin against the interpolated terrain height instead of the corner vertex of its
heightmap square. Next to a steep cliff that vertex could sit far above an origin that was
well clear of the ground, so the whole ground trace was skipped. A ray that starts exactly
on the surface is no longer treated as underground.
* The heading passed to `AimWeapon` is now derived from the horizontal part of the
shot direction. Since 2026.06 it was the unit-frame azimuth of the full launch vector,
which for steep (high-trajectory) shots on tilted ground swung by tens of degrees, so
scripts with a yaw arc check rejected targets the engine considered in arc and the
unit never fired (e.g. a Wolverine stopping on a slope during a ground attack).

# Additions
* `AimWeapon` additionally receives the unit-frame heading of the full shot direction,
the value 2026.06 passed as `heading`. It is the exact turret yaw that points the barrel
along a steep shot on tilted ground, but it depends on hull tilt and launch pitch, so
use it for turning the turret and keep arc checks on `heading`. LUS scripts get it as
an optional fourth argument, `AimWeapon(weaponNum, heading, pitch, launchHeading)`
(radians); COB scripts via `get WEAPON_LAUNCH_HEADING(weaponNum)` (141, COB angle
units). Existing scripts need no change.

# Weapons
* Added the `aimFromEstimate` tag to unit def weapon entries. It describes where the muzzle
ends up once the script has aimed, so line-of-fire tests that run before the turret has
turned (target selection and target re-validation) trace from the predicted muzzle instead of
the `AimFromWeapon` piece. The value is a list of 7 numbers
`{pivotX, pivotY, pivotZ, lateral, forward, barrelForward, barrelUp}` in unit space (yaw pivot,
offsets turning with yaw only, offsets turning with yaw and pitch). Weapons that alternate
between barrels use one estimate for the mean muzzle. Units without the tag behave as before.
* Added `Spring.GetUnitWeaponAimFromPos(unitID, weaponNum, x, y, z) -> posX, posY, posZ, isEstimate`
returning the position a weapon's pre-aim line-of-fire test towards the given target is traced from.
