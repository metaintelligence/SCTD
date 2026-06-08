# Hex Terrain Topology Templates

These files are preparation assets for SCTD hex terrain generation.

## Side Labels

- `S0`: top horizontal edge center
- `S1`: upper-right slanted edge center
- `S2`: lower-right slanted edge center
- `S3`: bottom horizontal edge center
- `S4`: lower-left slanted edge center
- `S5`: upper-left slanted edge center

## Rules

- Road and river endpoints must use only `S0` to `S5` side-center ports.
- Vertices are not valid connection points.
- The full road or river width must not touch or include any hex vertex.
- Use `Guides` for human review.
- Use `Masks` as topology-lock references for image generation, 3D modeling AI, or mesh layout.
- Final 3D meshes should snap road/river openings to exact edge midpoint coordinates.

## Generated Sets

- `RoadThrough` / `RiverThrough`: opposite-edge pass-through templates.
- `RoadEnd` / `RiverEnd`: one-edge end-cap templates.
- `RoadTurnA` / `RiverTurnA`: single-bend side-center turn templates.
- `RoadTurnB` / `RiverTurnB`: dogleg side-center turn templates.

The manifest file `hex_topology_manifest.json` lists every generated template and its ports.
