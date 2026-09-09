+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2026.07, for **pre-release 2026.08**.

## Fixes

### GLTF/GLB animation axes

- GLTF/GLB vertices, normals, tangents, piece offsets, and node rest rotations are
  now converted into the engine model coordinate frame while loading. Equivalent
  S3O and GLTF models consequently use the same BOS/LUS piece animation axes.
- `s3ocompat=true` remains available for models exported through the established
  S3O Blender workflow, but no longer requires compiler-side axis remapping.
