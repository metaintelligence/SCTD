from __future__ import annotations

import json
import math
import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

import GenerateRoadRiverTopologyLockedTiles as tiles


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "Content" / "Images" / "Terrain" / "RoadRiver_TopologyLocked" / "road_river_topology_locked_manifest.json"
OPPOSITE_PAIRS = {tuple(sorted(pair)) for pair in (("S0", "S3"), ("S1", "S4"), ("S2", "S5"))}


def fail(message: str) -> None:
    raise AssertionError(message)


def sorted_pair(ports: list[str]) -> tuple[str, str]:
    return tuple(sorted(ports))  # type: ignore[return-value]


def side_distance(a: str, b: str) -> int:
    ia = int(a[1])
    ib = int(b[1])
    diff = abs(ia - ib)
    return min(diff, 6 - diff)


def no_go_mask() -> Image.Image:
    mask = Image.new("L", (tiles.SIZE, tiles.SIZE), 0)
    draw = ImageDraw.Draw(mask)
    for vertex in tiles.HEX:
        x, y = vertex
        r = tiles.VERTEX_NO_GO
        draw.ellipse((x - r, y - r, x + r, y + r), fill=255)
    return mask


def expected_path_mask(asset: str, kind: str, ports: list[str]) -> Image.Image:
    points = tiles.path_points(kind, ports[0], ports[1] if len(ports) == 2 else None)
    width = tiles.ROAD_WIDTH if asset == "Road" else tiles.RIVER_WIDTH
    return tiles.path_mask(points, width + 36)


def actual_path_mask(path: Path) -> Image.Image:
    with Image.open(path).convert("RGBA") as img:
        r, g, b, a = img.split()
        whiteish = Image.eval(ImageChops.lighter(ImageChops.lighter(r, g), b), lambda p: 255 if p > 150 else 0)
        return ImageChops.multiply(whiteish, a.point(lambda p: 255 if p > 0 else 0))


def validate_tile(item: dict, vertex_mask: Image.Image) -> None:
    filename = ROOT / item["filename"]
    topology_mask = ROOT / item["topology_mask"]
    for path in (filename, topology_mask):
        if not path.exists():
            fail(f"Missing file: {path}")
        with Image.open(path) as img:
            if img.size != (2048, 2048):
                fail(f"Wrong size for {path.name}: {img.size}")

    asset = item["asset"]
    kind = item["kind"]
    ports = item["ports"]
    if kind == "through" and sorted_pair(ports) not in OPPOSITE_PAIRS:
        fail(f"{filename.name}: through tile is not opposite-side: {ports}")
    if kind in {"turn_a", "turn_b"}:
        if sorted_pair(ports) in OPPOSITE_PAIRS or side_distance(ports[0], ports[1]) == 3:
            fail(f"{filename.name}: A/B tile uses pass-through opposite ports: {ports}")
    if kind == "end" and len(ports) != 1:
        fail(f"{filename.name}: end cap must have exactly one open port")

    expected = expected_path_mask(asset, kind, ports)
    actual = actual_path_mask(topology_mask)
    diff = ImageChops.difference(expected, actual)
    bbox = diff.getbbox()
    if bbox:
        # Allow a tiny difference from antialiasing and PNG roundtrips.
        nonzero = sum(1 for px in diff.getdata() if px > 0)
        if nonzero > 2500:
            fail(f"{filename.name}: saved topology mask does not match expected path mask")

    vertex_hit = ImageChops.multiply(actual, vertex_mask)
    if vertex_hit.getbbox():
        fail(f"{filename.name}: path touches forbidden vertex zone")


def main() -> int:
    if not MANIFEST.exists():
        fail(f"Missing manifest: {MANIFEST}")
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    tile_list = data.get("tiles", [])
    if len(tile_list) != 20:
        fail(f"Expected 20 tiles, got {len(tile_list)}")

    counts: dict[tuple[str, str], int] = {}
    for item in tile_list:
        key = (item["asset"], item["kind"])
        counts[key] = counts.get(key, 0) + 1
    expected_counts = {
        ("Road", "through"): 3,
        ("Road", "end"): 3,
        ("Road", "turn_a"): 2,
        ("Road", "turn_b"): 2,
        ("River", "through"): 3,
        ("River", "end"): 3,
        ("River", "turn_a"): 2,
        ("River", "turn_b"): 2,
    }
    if counts != expected_counts:
        fail(f"Unexpected tile counts: {counts}")

    vertex_mask = no_go_mask()
    for item in tile_list:
        validate_tile(item, vertex_mask)

    print("Road/river topology-locked tile validation passed.")
    print("Counts: Road 10, River 10")
    print("All through tiles use opposite side-center ports.")
    print("All A/B turn tiles use non-opposite side-center ports.")
    print("All road/river topology masks avoid forbidden vertex zones.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"Validation failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
