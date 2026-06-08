# Road / River AI Style Tiles

These tiles were generated to match the existing SCTD low-poly/cartoon terrain render style.

## Output

- 20 high-style road/river tile images are in this folder.
- 20 corresponding topology reference masks are in `TopologyMasks`.
- `_AIStyle_ContactSheet.png` is a visual review sheet.

## Review Notes

- Visual quality now matches the existing road/water terrain references much more closely than the procedural topology-locked draft set.
- Road and river openings were prompted to use S0-S5 side-center ports only.
- The images are AI-generated, so their topology cannot be proven at the same pixel-perfect level as the deterministic masks.
- The deterministic masks should remain the source of truth for 3D modeling or mesh snapping.

## Caution

The following images are visually usable but should be checked carefully if exact side-center topology is critical:

- `TILE_RiverTurnA_02_S4_S5_Elbow.png`
- `TILE_RiverTurnB_02_S4_S5_Dogleg.png`

Reason: adjacent left-side river ports are difficult for the image model to preserve perfectly under isometric perspective. The rendered results communicate the intended connection, but the side-center alignment is less mathematically strict than the reference masks.
