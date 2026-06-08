from __future__ import annotations

import json
import math
import random
import hashlib
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

import GenerateHexTopologyTemplates as topo


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "Content" / "Images" / "Terrain" / "RoadRiver_TopologyLocked"
MASK_DIR = OUT_DIR / "TopologyMasks"
MANIFEST = OUT_DIR / "road_river_topology_locked_manifest.json"

SIZE = topo.SIZE
CANVAS = topo.CANVAS
SCALE = 1
SIZE = CANVAS
CX = CY = CANVAS // 2
R = 820
ROAD_WIDTH = 178
RIVER_WIDTH = 222
VERTEX_NO_GO = 92


def scale_point(point: tuple[int, int]) -> tuple[int, int]:
    return (int(round(point[0] / topo.SCALE)), int(round(point[1] / topo.SCALE)))


HEX = [scale_point(point) for point in topo.VERTICES]
PORTS = {name: scale_point(point) for name, point in topo.PORTS.items()}


def hex_clip_mask() -> Image.Image:
    mask = Image.new("L", (SIZE, SIZE), 0)
    draw = ImageDraw.Draw(mask)
    draw.polygon(HEX, fill=255)
    return mask


HEX_CLIP = hex_clip_mask()

DEPTH = 86 * SCALE


def clamp(v: int) -> int:
    return max(0, min(255, v))


def shade(color: tuple[int, int, int], delta: int) -> tuple[int, int, int, int]:
    return tuple(clamp(c + delta) for c in color) + (255,)


def downsample(img: Image.Image) -> Image.Image:
    return img


def save(img: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    downsample(img).save(path, compress_level=1)


def make_clip_layer() -> Image.Image:
    return Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))


def hex_mask_rgba(layer: Image.Image) -> Image.Image:
    alpha = layer.getchannel("A")
    layer = layer.copy()
    layer.putalpha(ImageChops.multiply(alpha, HEX_CLIP))
    return layer


def draw_polyline(
    draw: ImageDraw.ImageDraw,
    points: list[tuple[int, int]],
    width: int,
    fill: tuple[int, int, int, int],
    outline: tuple[int, int, int, int] | None = None,
    outline_width: int = 0,
) -> None:
    if outline and outline_width:
        draw.line(points, fill=outline, width=width + outline_width * 2, joint="curve")
    draw.line(points, fill=fill, width=width, joint="curve")


def lerp(a: tuple[int, int], b: tuple[int, int], t: float) -> tuple[int, int]:
    return (int(a[0] + (b[0] - a[0]) * t), int(a[1] + (b[1] - a[1]) * t))


def path_points(kind: str, a: str, b: str | None = None) -> list[tuple[int, int]]:
    if kind == "through" and b:
        return [PORTS[a], PORTS[b]]
    if kind == "end":
        return [PORTS[a], lerp(PORTS[a], (CX, CY), 0.70)]
    if kind == "turn_a" and b:
        return [PORTS[a], (CX, CY), PORTS[b]]
    if kind == "turn_b" and b:
        pa, pb = PORTS[a], PORTS[b]
        return [pa, lerp(pa, (CX, CY), 0.60), lerp(pb, (CX, CY), 0.60), pb]
    raise ValueError(f"Unsupported path: {kind}, {a}, {b}")


def path_mask(points: list[tuple[int, int]], width: int) -> Image.Image:
    mask = Image.new("L", (SIZE, SIZE), 0)
    draw = ImageDraw.Draw(mask)
    draw.line(points, fill=255, width=width, joint="curve")
    clipped = Image.new("L", (SIZE, SIZE), 0)
    clipped.paste(mask, (0, 0), HEX_CLIP)
    return clipped


def point_in_hex(point: tuple[int, int]) -> bool:
    x, y = point
    inside = False
    pts = HEX
    j = len(pts) - 1
    for i in range(len(pts)):
        xi, yi = pts[i]
        xj, yj = pts[j]
        if (yi > y) != (yj > y):
            x_cross = (xj - xi) * (y - yi) / max(yj - yi, 1e-9) + xi
            if x < x_cross:
                inside = not inside
        j = i
    return inside


def random_hex_point(rng: random.Random, margin: int = 0) -> tuple[int, int]:
    xs = [p[0] for p in HEX]
    ys = [p[1] for p in HEX]
    while True:
        p = (
            rng.randint(min(xs) + margin, max(xs) - margin),
            rng.randint(min(ys) + margin, max(ys) - margin),
        )
        if point_in_hex(p):
            return p


def mask_near(mask: Image.Image, point: tuple[int, int], radius: int) -> bool:
    x, y = point
    for dx, dy in (
        (0, 0),
        (-radius, 0),
        (radius, 0),
        (0, -radius),
        (0, radius),
        (-radius, -radius),
        (-radius, radius),
        (radius, -radius),
        (radius, radius),
    ):
        sx = max(0, min(SIZE - 1, x + dx))
        sy = max(0, min(SIZE - 1, y + dy))
        if mask.getpixel((sx, sy)) > 0:
            return True
    return False


def draw_background(img: Image.Image) -> None:
    draw = ImageDraw.Draw(img)
    draw.rectangle((0, 0, SIZE, SIZE), fill=(34, 35, 36, 255))
    for _ in range(45):
        x = random.randint(0, SIZE)
        y = random.randint(0, SIZE)
        radius = random.randint(2 * SCALE, 7 * SCALE)
        c = random.choice([(43, 43, 42, 40), (15, 15, 15, 36), (68, 63, 54, 26)])
        draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=c)


def draw_slab(img: Image.Image, rng: random.Random, base: tuple[int, int, int]) -> None:
    draw = ImageDraw.Draw(img)
    shadow = [(x + 110 * SCALE, y + DEPTH + 90 * SCALE) for x, y in HEX]
    draw.polygon(shadow, fill=(8, 8, 9, 150))
    draw.polygon([(x, y + DEPTH) for x, y in HEX], fill=shade(base, -68))

    bottom = [(x, y + DEPTH) for x, y in HEX]
    for i in range(len(HEX)):
        j = (i + 1) % len(HEX)
        y_avg = (HEX[i][1] + HEX[j][1]) / 2
        if y_avg < CY - R * 0.15:
            continue
        delta = -58 if y_avg > CY else -42
        draw.polygon([HEX[i], HEX[j], bottom[j], bottom[i]], fill=shade(base, delta))
        draw.line([bottom[i], bottom[j]], fill=shade(base, -86), width=3 * SCALE)

    top = make_clip_layer()
    tdraw = ImageDraw.Draw(top)
    tdraw.polygon(HEX, fill=shade(base, 0))

    for _ in range(96):
        cx, cy = random_hex_point(rng)
        radius = rng.randint(60 * SCALE, 170 * SCALE)
        sides = rng.choice([3, 4])
        angle0 = rng.random() * math.tau
        pts = []
        for k in range(sides):
            angle = angle0 + math.tau * k / sides + rng.uniform(-0.22, 0.22)
            rr = radius * rng.uniform(0.55, 1.05)
            pts.append((int(cx + math.cos(angle) * rr), int(cy + math.sin(angle) * rr)))
        tdraw.polygon(pts, fill=shade(base, rng.randint(-22, 24)))

    for _ in range(18):
        a = random_hex_point(rng, 50 * SCALE)
        length = rng.randint(70 * SCALE, 260 * SCALE)
        angle = rng.uniform(-0.3, 0.3) + rng.choice([0.0, math.pi / 3, -math.pi / 3])
        b = (int(a[0] + math.cos(angle) * length), int(a[1] + math.sin(angle) * length))
        tdraw.line([a, b], fill=(72, 61, 50, rng.randint(90, 150)), width=rng.randint(2 * SCALE, 5 * SCALE))

    img.alpha_composite(hex_mask_rgba(top))
    draw.line(HEX + [HEX[0]], fill=(220, 200, 165, 255), width=6 * SCALE, joint="curve")
    draw.line([(x, y + DEPTH) for x, y in HEX] + [(HEX[0][0], HEX[0][1] + DEPTH)], fill=(35, 30, 25, 160), width=4 * SCALE, joint="curve")


def draw_road(img: Image.Image, points: list[tuple[int, int]], rng: random.Random, variant: int) -> Image.Image:
    body_width = ROAD_WIDTH
    outline_width = 22 * SCALE
    road_mask = path_mask(points, body_width + outline_width * 2)

    layer = make_clip_layer()
    draw = ImageDraw.Draw(layer)
    draw_polyline(draw, points, body_width, (44, 48, 48, 255), (19, 18, 17, 255), outline_width)

    for _ in range(48):
        p = random_hex_point(rng)
        if road_mask.getpixel(p) == 0:
            continue
        rr = rng.randint(10 * SCALE, 34 * SCALE)
        col = rng.choice([(55, 57, 54, 70), (27, 27, 26, 72), (66, 64, 58, 58)])
        draw.polygon(
            [
                (p[0] + rng.randint(-rr, rr), p[1] + rng.randint(-rr, rr)),
                (p[0] + rng.randint(-rr, rr), p[1] + rng.randint(-rr, rr)),
                (p[0] + rng.randint(-rr, rr), p[1] + rng.randint(-rr, rr)),
            ],
            fill=col,
        )

    dash_color = rng.choice([(210, 178, 78, 230), (188, 166, 98, 220)])
    for a, b in zip(points, points[1:]):
        dx, dy = b[0] - a[0], b[1] - a[1]
        length = max(math.hypot(dx, dy), 1)
        ux, uy = dx / length, dy / length
        spacing = 245 * SCALE
        dash = 82 * SCALE
        count = max(1, int(length // spacing))
        for i in range(1, count + 1):
            t = (i - 0.5) / count
            if t < 0.12 or t > 0.88:
                continue
            c = (int(a[0] + dx * t), int(a[1] + dy * t))
            jitter = rng.randint(-8 * SCALE, 8 * SCALE)
            p1 = (int(c[0] - ux * dash / 2 - uy * jitter), int(c[1] - uy * dash / 2 + ux * jitter))
            p2 = (int(c[0] + ux * dash / 2 - uy * jitter), int(c[1] + uy * dash / 2 + ux * jitter))
            draw.line([p1, p2], fill=dash_color, width=10 * SCALE)

    for _ in range(12):
        p = random_hex_point(rng)
        if road_mask.getpixel(p) == 0:
            continue
        length = rng.randint(45 * SCALE, 130 * SCALE)
        angle = rng.choice([0.0, math.pi / 3, -math.pi / 3]) + rng.uniform(-0.25, 0.25)
        b = (int(p[0] + math.cos(angle) * length), int(p[1] + math.sin(angle) * length))
        draw.line([p, b], fill=(12, 12, 12, 150), width=rng.randint(2 * SCALE, 5 * SCALE))

    img.alpha_composite(hex_mask_rgba(layer))
    return road_mask


def draw_river(img: Image.Image, points: list[tuple[int, int]], rng: random.Random, variant: int) -> Image.Image:
    water_width = RIVER_WIDTH
    bank_width = 34 * SCALE
    river_mask = path_mask(points, water_width + bank_width * 2)

    layer = make_clip_layer()
    draw = ImageDraw.Draw(layer)
    draw_polyline(draw, points, water_width, (32, 156, 174, 255), (54, 65, 50, 255), bank_width)

    for _ in range(62):
        p = random_hex_point(rng)
        if river_mask.getpixel(p) == 0:
            continue
        rr = rng.randint(30 * SCALE, 90 * SCALE)
        col = rng.choice([(46, 179, 190, 72), (18, 117, 143, 66), (82, 198, 202, 55)])
        draw.polygon(
            [
                (p[0] + rng.randint(-rr, rr), p[1] + rng.randint(-rr, rr)),
                (p[0] + rng.randint(-rr, rr), p[1] + rng.randint(-rr, rr)),
                (p[0] + rng.randint(-rr, rr), p[1] + rng.randint(-rr, rr)),
                (p[0] + rng.randint(-rr, rr), p[1] + rng.randint(-rr, rr)),
            ],
            fill=col,
        )

    for a, b in zip(points, points[1:]):
        dx, dy = b[0] - a[0], b[1] - a[1]
        length = max(math.hypot(dx, dy), 1)
        ux, uy = dx / length, dy / length
        px, py = -uy, ux
        count = max(1, int(length // (240 * SCALE)))
        for i in range(count):
            t = (i + 0.5) / count
            c = (int(a[0] + dx * t), int(a[1] + dy * t))
            offset = rng.randint(-65 * SCALE, 65 * SCALE)
            p1 = (int(c[0] + px * offset - ux * 42 * SCALE), int(c[1] + py * offset - uy * 42 * SCALE))
            p2 = (int(c[0] + px * offset + ux * 42 * SCALE), int(c[1] + py * offset + uy * 42 * SCALE))
            draw.line([p1, p2], fill=(142, 228, 222, 95), width=5 * SCALE)

    img.alpha_composite(hex_mask_rgba(layer))
    return river_mask


def draw_rocks_and_scrap(
    img: Image.Image,
    rng: random.Random,
    avoid_mask: Image.Image,
    density: int,
    theme: str,
) -> None:
    layer = make_clip_layer()
    draw = ImageDraw.Draw(layer)
    for _ in range(density):
        p = random_hex_point(rng, 95 * SCALE)
        kind = rng.choice(["rock", "scrap", "deadgrass", "plate"])
        if theme == "river" and kind == "deadgrass":
            kind = "reed"
        size = rng.randint(18 * SCALE, 58 * SCALE)
        if mask_near(avoid_mask, p, size + 72 * SCALE):
            continue
        if kind == "rock":
            pts = []
            for k in range(rng.randint(5, 7)):
                a = math.tau * k / 6 + rng.uniform(-0.22, 0.22)
                rr = size * rng.uniform(0.55, 1.0)
                pts.append((int(p[0] + math.cos(a) * rr), int(p[1] + math.sin(a) * rr)))
            draw.polygon(pts, fill=rng.choice([(93, 87, 76, 255), (118, 105, 86, 255), (71, 72, 68, 255)]))
            draw.line(pts + [pts[0]], fill=(38, 36, 32, 130), width=2 * SCALE)
        elif kind == "scrap":
            w = size
            h = int(size * rng.uniform(0.35, 0.65))
            col = rng.choice([(119, 74, 47, 255), (83, 86, 80, 255), (128, 103, 66, 255)])
            draw.rounded_rectangle((p[0] - w, p[1] - h, p[0] + w, p[1] + h), radius=3 * SCALE, fill=col, outline=(38, 29, 25, 150), width=2 * SCALE)
            draw.line([(p[0] - w // 2, p[1] - h), (p[0] - w // 2, p[1] + h)], fill=(180, 140, 96, 80), width=2 * SCALE)
        elif kind == "plate":
            pts = [
                (p[0] - size, p[1] - size // 3),
                (p[0] + size // 2, p[1] - size // 2),
                (p[0] + size, p[1] + size // 4),
                (p[0] - size // 2, p[1] + size // 2),
            ]
            draw.polygon(pts, fill=(100, 72, 49, 235), outline=(45, 35, 28, 160))
        elif kind == "reed":
            for _ in range(4):
                angle = rng.uniform(-1.25, -0.55)
                end = (int(p[0] + math.cos(angle) * size), int(p[1] + math.sin(angle) * size))
                draw.line([p, end], fill=(55, 79, 47, 210), width=4 * SCALE)
        else:
            for _ in range(5):
                angle = rng.uniform(-1.3, -0.2)
                end = (int(p[0] + math.cos(angle) * size), int(p[1] + math.sin(angle) * size))
                draw.line([p, end], fill=(83, 70, 43, 180), width=3 * SCALE)

    img.alpha_composite(hex_mask_rgba(layer))


def draw_end_cap_detail(
    img: Image.Image,
    spec: dict,
    points: list[tuple[int, int]],
    rng: random.Random,
    avoid_mask: Image.Image,
) -> None:
    if spec["kind"] != "end":
        return
    end = points[-1]
    layer = make_clip_layer()
    draw = ImageDraw.Draw(layer)
    if spec["asset"] == "Road":
        for idx in range(5):
            ox = rng.randint(-70 * SCALE, 70 * SCALE)
            oy = rng.randint(-55 * SCALE, 55 * SCALE)
            w = rng.randint(40 * SCALE, 75 * SCALE)
            h = rng.randint(22 * SCALE, 40 * SCALE)
            col = rng.choice([(92, 58, 38, 255), (117, 75, 45, 255), (75, 77, 73, 255)])
            draw.rectangle((end[0] + ox - w, end[1] + oy - h, end[0] + ox + w, end[1] + oy + h), fill=col, outline=(29, 22, 18, 170), width=2 * SCALE)
        draw.line(
            [(end[0] - 150 * SCALE, end[1] - 75 * SCALE), (end[0] + 145 * SCALE, end[1] + 75 * SCALE)],
            fill=(178, 112, 56, 230),
            width=12 * SCALE,
        )
    else:
        for _ in range(8):
            p = (end[0] + rng.randint(-135 * SCALE, 135 * SCALE), end[1] + rng.randint(-95 * SCALE, 95 * SCALE))
            radius = rng.randint(18 * SCALE, 48 * SCALE)
            draw.ellipse((p[0] - radius, p[1] - radius, p[0] + radius, p[1] + radius), fill=(74, 72, 60, 220))
        draw.arc((end[0] - 155 * SCALE, end[1] - 120 * SCALE, end[0] + 155 * SCALE, end[1] + 120 * SCALE), 190, 520, fill=(34, 87, 78, 200), width=12 * SCALE)
    img.alpha_composite(hex_mask_rgba(layer))


def make_output_mask(spec: dict, points: list[tuple[int, int]]) -> Image.Image:
    width = ROAD_WIDTH if spec["asset"] == "Road" else RIVER_WIDTH
    mask = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(mask)
    draw.polygon(HEX, fill=(40, 40, 40, 255))
    body = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    bdraw = ImageDraw.Draw(body)
    fill = (255, 255, 255, 255)
    outline = (180, 180, 180, 255)
    draw_polyline(bdraw, points, width, fill, outline, 18 * SCALE)
    mask.alpha_composite(hex_mask_rgba(body))
    return mask


def render_tile(spec: dict) -> dict:
    rng = random.Random(spec["seed"])
    points = path_points(spec["kind"], spec["ports"][0], spec["ports"][1] if len(spec["ports"]) == 2 else None)

    img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    draw_background(img)
    base_color = rng.choice([(136, 122, 96), (125, 112, 91), (145, 126, 94)])
    draw_slab(img, rng, base_color)

    if spec["asset"] == "Road":
        avoid = draw_road(img, points, rng, spec["variant"])
        draw_end_cap_detail(img, spec, points, rng, avoid)
        draw_rocks_and_scrap(img, rng, avoid, 16 + spec["variant"], "road")
    else:
        avoid = draw_river(img, points, rng, spec["variant"])
        draw_end_cap_detail(img, spec, points, rng, avoid)
        draw_rocks_and_scrap(img, rng, avoid, 15 + spec["variant"], "river")

    out_path = OUT_DIR / spec["filename"]
    mask_path = MASK_DIR / spec["filename"].replace(".png", "_TopologyMask.png")
    save(img, out_path)
    save(make_output_mask(spec, points), mask_path)
    return {
        "filename": str(out_path.relative_to(ROOT)).replace("\\", "/"),
        "topology_mask": str(mask_path.relative_to(ROOT)).replace("\\", "/"),
        "asset": spec["asset"],
        "kind": spec["kind"],
        "ports": spec["ports"],
        "variant": spec["variant"],
        "description": spec["description"],
    }


def specs() -> list[dict]:
    items = [
        # Road pass-through tiles.
        ("Road", "through", ["S0", "S3"], 1, "TILE_RoadThrough_01_S0_S3_CrackedAsphalt.png", "straight north-south cracked asphalt"),
        ("Road", "through", ["S1", "S4"], 2, "TILE_RoadThrough_02_S1_S4_DustyDiagonal.png", "upper-right to lower-left dusty asphalt"),
        ("Road", "through", ["S2", "S5"], 3, "TILE_RoadThrough_03_S2_S5_BrokenServiceRoad.png", "lower-right to upper-left broken service road"),
        # River pass-through tiles.
        ("River", "through", ["S0", "S3"], 1, "TILE_RiverThrough_01_S0_S3_StraightChannel.png", "straight vertical river channel"),
        ("River", "through", ["S1", "S4"], 2, "TILE_RiverThrough_02_S1_S4_RubbleBank.png", "diagonal river with rubble banks"),
        ("River", "through", ["S2", "S5"], 3, "TILE_RiverThrough_03_S2_S5_FloodCut.png", "opposite diagonal flood cut"),
        # End caps.
        ("Road", "end", ["S0"], 1, "TILE_RoadEnd_01_S0_Barricade.png", "road stops at interior barricade"),
        ("Road", "end", ["S2"], 2, "TILE_RoadEnd_02_S2_RubbleCollapse.png", "road blocked by rubble collapse"),
        ("Road", "end", ["S4"], 3, "TILE_RoadEnd_03_S4_SandFilled.png", "sand-filled road end"),
        ("River", "end", ["S1"], 1, "TILE_RiverEnd_01_S1_DryBasin.png", "river fades into dry basin"),
        ("River", "end", ["S3"], 2, "TILE_RiverEnd_02_S3_RockPool.png", "river ends in rock pool"),
        ("River", "end", ["S5"], 3, "TILE_RiverEnd_03_S5_SiltedDrain.png", "silted drainage end"),
        # A turns: one-bend elbows.
        ("Road", "turn_a", ["S0", "S2"], 1, "TILE_RoadTurnA_01_S0_S2_Elbow.png", "single-bend elbow road"),
        ("Road", "turn_a", ["S3", "S5"], 2, "TILE_RoadTurnA_02_S3_S5_Elbow.png", "single-bend elbow road"),
        ("River", "turn_a", ["S0", "S2"], 1, "TILE_RiverTurnA_01_S0_S2_Elbow.png", "single-bend elbow river"),
        ("River", "turn_a", ["S4", "S5"], 2, "TILE_RiverTurnA_02_S4_S5_Elbow.png", "single-bend elbow river"),
        # B turns: doglegs with an extra internal bend.
        ("Road", "turn_b", ["S0", "S2"], 1, "TILE_RoadTurnB_01_S0_S2_Dogleg.png", "dogleg road with extra internal bend"),
        ("Road", "turn_b", ["S3", "S5"], 2, "TILE_RoadTurnB_02_S3_S5_Dogleg.png", "dogleg road with extra internal bend"),
        ("River", "turn_b", ["S0", "S2"], 1, "TILE_RiverTurnB_01_S0_S2_Dogleg.png", "dogleg river with extra internal bend"),
        ("River", "turn_b", ["S4", "S5"], 2, "TILE_RiverTurnB_02_S4_S5_Dogleg.png", "dogleg river with extra internal bend"),
    ]
    return [
        {
            "asset": asset,
            "kind": kind,
            "ports": ports,
            "variant": variant,
            "filename": filename,
            "description": description,
            "seed": int(hashlib.sha256(f"{asset}|{kind}|{ports}|{variant}|{filename}".encode("utf-8")).hexdigest()[:8], 16),
        }
        for asset, kind, ports, variant, filename, description in items
    ]


def main() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    MASK_DIR.mkdir(parents=True, exist_ok=True)
    for path in OUT_DIR.glob("TILE_*.png"):
        path.unlink()
    for path in MASK_DIR.glob("*.png"):
        path.unlink()

    manifest = [render_tile(spec) for spec in specs()]
    MANIFEST.write_text(json.dumps({"tiles": manifest}, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Generated {len(manifest)} topology-locked road/river tiles")
    print(f"Output: {OUT_DIR}")


if __name__ == "__main__":
    main()
