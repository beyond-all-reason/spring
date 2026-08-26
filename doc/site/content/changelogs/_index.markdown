+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2026.07, for **pre-release 2026.08**.

## Lua API

- Add the synced, watch-gated `UnitWeaponBurstEnd` call-in. It is delivered at
  the end of the simulation frame with the unit and weapon identifiers plus a
  snapshot of the active command ID and tag from when the burst ended. Add
  `Script.GetWatchWeaponBurst` and `Script.SetWatchWeaponBurst` to control it.
