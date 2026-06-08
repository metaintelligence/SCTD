from __future__ import annotations

import json
import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw

import GenerateHexTopologyTemplates as topo


ROOT = Path(__file__).resolve().parents[1]
TEMPLATE_DIR = ROOT / "Content" / "Images" / "Terrain" / "Templates"
GUIDE_DIR = TEMPLATE_DIR / "Guides"
MASK_DIR = TEMPLATE_DIR / "Masks"
MANIFEST = TEMPLATE_DIR / "hex_topology_manifest.json"

OPPOSITE_PAIRS = {tuple(sorted(pair)) for pair in (("S0", "S3"), ("S1", "S4"), ("S2", "S5"))}
SIDES = {f"S{i}" for i in range(6)}


def fail(message: str) -> None:
    raise AssertionError(message)


def sorted_pair(ports: list[str]) -> tuple[str, str]:
    if len(ports) != 2:
        fail(f"Expected two ports, got {ports}")
    return tuple(sorted(ports))  # type: ignore[return-value]


def distance(a: tuple[int, int], b: tuple[int, int]) -> float:
    return math.hypot(a[0] - b[0], a[1] - b[1])


def polyline_length(points: list[tuple[int, int]]) -> float:
    return sum(distance(a, b) for a, b in zip(points, points[1:]))


def raster_path(points: list[tuple[int, int]], width: int) -> Image.Image:
    img = Image.new("L", (topo.SIZE, topo.SIZE), 0)
    draw = ImageDraw.Draw(img)
    draw.line(points, fill=255, width=width, joint="curve")
    alpha = img.point(lambda p: 255 if p > 0 else 0)
    clipped = Image.new("L", (topo.SIZE, topo.SIZE), 0)
    clipped.paste(alpha, (0, 0), topo.HEX_CLIP)
    return clipped


def assert_no_vertex_intrusion(points: list[tuple[int, int]], width: int, name: str) -> None:
    path = raster_path(points, width)
    check_radius = topo.VERTEX_NO_GO
    for idx, vertex in enumerate(topo.VERTICES):
        zone = Image.new("L", (topo.SIZE, topo.SIZE), 0)
        draw = ImageDraw.Draw(zone)
        x, y = vertex
        draw.ellipse((x - check_radius, y - check_radius, x + check_radius, y + check_radius), fill=255)
        if Image.composite(path, Image.new("L", path.size, 0), zone).getbbox():
            fail(f"{name}: path intrudes into forbidden vertex zone V{idx}")


def assert_port_center(points: list[tuple[int, int]], ports: list[str], name: str) -> None:
    start = topo.PORTS[ports[0]]
    if distance(points[0], start) > 1:
        fail(f"{name}: start point is not at {ports[0]} side center")
    if len(ports) == 2:
        end = topo.PORTS[ports[1]]
        if distance(points[-1], end) > 1:
            fail(f"{name}: end point is not at {ports[1]} side center")


def validate_manifest() -> list[dict]:
    if not MANIFEST.exists():
        fail(f"Missing manifest: {MANIFEST}")
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    templates = data.get("templates", [])
    if len(templates) != 66:
        fail(f"Expected 66 topology templates, got {len(templates)}")

    counts: dict[str, int] = {}
    for item in templates:
        counts[item["kind"]] = counts.get(item["kind"], 0) + 1
    expected_counts = {"through": 6, "end": 12, "turn_a": 24, "turn_b": 24}
    if counts != expected_counts:
        fail(f"Unexpected kind counts: {counts}, expected {expected_counts}")

    return templates


def validate_item(item: dict) -> None:
    name = Path(item["mask"]).stem
    asset = item["asset"]
    kind = item["kind"]
    ports = item["ports"]

    if asset not in {"Road", "River"}:
        fail(f"{name}: unknown asset type {asset}")
    if any(port not in SIDES for port in ports):
        fail(f"{name}: unknown port list {ports}")

    guide = ROOT / item["guide"]
    mask = ROOT / item["mask"]
    if not guide.exists():
        fail(f"{name}: missing guide file {guide}")
    if not mask.exists():
        fail(f"{name}: missing mask file {mask}")

    for path in (guide, mask):
        with Image.open(path) as img:
            if img.size != (topo.CANVAS, topo.CANVAS):
                fail(f"{name}: wrong image size for {path.name}: {img.size}")

    if kind == "through":
        if sorted_pair(ports) not in OPPOSITE_PAIRS:
            fail(f"{name}: through templates must use opposite side centers only, got {ports}")
        points = topo.path_points(kind, ports[0], ports[1])
        if len(points) != 2:
            fail(f"{name}: through path must be a straight two-point line")
    elif kind == "end":
        if len(ports) != 1:
            fail(f"{name}: end templates must have one port, got {ports}")
        points = topo.path_points(kind, ports[0], None)
        if len(points) != 2:
            fail(f"{name}: end path must have one open edge and one interior endpoint")
        if distance(points[-1], topo.PORTS[ports[0]]) < topo.R * 0.25:
            fail(f"{name}: end cap interior endpoint is too close to the edge")
    elif kind in {"turn_a", "turn_b"}:
        if sorted_pair(ports) in OPPOSITE_PAIRS:
            fail(f"{name}: A/B turn must not use opposite pass-through ports, got {ports}")
        if topo.side_distance(ports[0], ports[1]) == 3:
            fail(f"{name}: A/B turn is opposite-side pass-through, got {ports}")
        points = topo.path_points(kind, ports[0], ports[1])
        if kind == "turn_a":
            if len(points) != 3:
                fail(f"{name}: TurnA must be one-bend elbow with three points")
            if distance(points[1], (topo.CX, topo.CY)) > 1:
                fail(f"{name}: TurnA elbow bend must pass through tile center")
        else:
            if len(points) != 4:
                fail(f"{name}: TurnB must be dogleg with four points")
            if distance(points[1], (topo.CX, topo.CY)) < 2 or distance(points[2], (topo.CX, topo.CY)) < 2:
                fail(f"{name}: TurnB dogleg bends must not collapse into TurnA center bend")
            turn_a_points = topo.path_points("turn_a", ports[0], ports[1])
            if abs(polyline_length(points) - polyline_length(turn_a_points)) < topo.R * 0.05:
                fail(f"{name}: TurnB path is too similar to TurnA")
    else:
        fail(f"{name}: unknown kind {kind}")

    assert_port_center(points, ports, name)
    width = topo.ROAD_WIDTH if asset == "Road" else topo.RIVER_WIDTH
    assert_no_vertex_intrusion(points, width + 32 * topo.SCALE, name)


def main() -> int:
    templates = validate_manifest()
    for item in templates:
        validate_item(item)

    guide_files = list(GUIDE_DIR.glob("*.png"))
    mask_files = list(MASK_DIR.glob("*.png"))
    if len(guide_files) != 68:
        fail(f"Expected 68 guide PNGs including the two base guides, got {len(guide_files)}")
    if len(mask_files) != 66:
        fail(f"Expected 66 mask PNGs, got {len(mask_files)}")

    print("Hex topology template validation passed.")
    print("Counts: through=6, end=12, turn_a=24, turn_b=24")
    print("All A/B turn templates use non-opposite side-center ports.")
    print("All rendered path bodies avoid forbidden vertex zones.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"Validation failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
