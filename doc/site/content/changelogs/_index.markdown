+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2026.07, for **pre-release 2026.08**.

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
shot direction. Since 2026.02 it was the unit-frame azimuth of the full launch vector,
which for steep (high-trajectory) shots on tilted ground swung by tens of degrees, so
scripts with a yaw arc check rejected targets the engine considered in arc and the
unit never fired (e.g. a Wolverine stopping on a slope during a ground attack).
