+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2026.07, for **pre-release 2026.08**.

## Weapons

- Check turret `maxangledif` against the launch direction relative to the unit,
  matching `AimWeapon` on slopes and for ballistic or arcing shots. This can
  change which targets are inside a limited firing arc, including on flat ground.
  The `AimWeapon` heading and pitch arguments are unchanged.
