from __future__ import annotations

import json
import math
from pathlib import Path
from itertools import combinations
from typing import Iterable

from PIL import Image, ImageChops, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "Content" / "Images" / "Terrain" / "Templates"
GUIDE_DIR = OUT_DIR / "Guides"
MASK_DIR = OUT_DIR / "Masks"

CANVAS = 2048
SCALE = 3
SIZE = CANVAS * SCALE
CX = CY = SIZE // 2
R = int(820 * SCALE)

ROAD_WIDTH = int(178 * SCALE)
RIVER_WIDTH = int(222 * SCALE)
PORT_DOT = int(20 * SCALE)
VERTEX_NO_GO = int(92 * SCALE)


def load_font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    candidates = [
        Path("C:/Windows/Fonts/arialbd.ttf" if bold else "C:/Windows/Fonts/arial.ttf"),
        Path("C:/Windows/Fonts/segoeuib.ttf" if bold else "C:/Windows/Fonts/segoeui.ttf"),
    ]
    for path in candidates:
        if path.exists():
            return ImageFont.truetype(str(path), size)
    return ImageFont.load_default()


FONT_L = load_font(44 * SCALE, True)
FONT_M = load_font(27 * SCALE, True)
FONT_S = load_font(20 * SCALE, False)


def hex_vertices() -> list[tuple[int, int]]:
    # Flat-top regular hexagon, clockwise from right vertex.
    points = []
    for angle_deg in [0, 60, 120, 180, 240, 300]:
        a = math.radians(angle_deg)
        points.append((int(CX + R * math.cos(a)), int(CY + R * math.sin(a))))
    return points


VERTICES = hex_vertices()

# Side labels are clockwise starting at the top horizontal side.
SIDE_VERTEX_INDICES = {
    "S0": (4, 5),  # top
    "S1": (5, 0),  # upper-right
    "S2": (0, 1),  # lower-right
    "S3": (1, 2),  # bottom
    "S4": (2, 3),  # lower-left
    "S5": (3, 4),  # upper-left
}


def midpoint(a: tuple[int, int], b: tuple[int, int]) -> tuple[int, int]:
    return ((a[0] + b[0]) // 2, (a[1] + b[1]) // 2)


PORTS = {name: midpoint(VERTICES[i], VERTICES[j]) for name, (i, j) in SIDE_VERTEX_INDICES.items()}


def lerp(a: tuple[int, int], b: tuple[int, int], t: float) -> tuple[int, int]:
    return (int(a[0] + (b[0] - a[0]) * t), int(a[1] + (b[1] - a[1]) * t))


def fit_text_center(
    draw: ImageDraw.ImageDraw,
    xy: tuple[int, int],
    text: str,
    font: ImageFont.ImageFont,
    fill: tuple[int, int, int, int],
) -> None:
    bbox = draw.textbbox((0, 0), text, font=font)
    w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
    draw.text((xy[0] - w // 2, xy[1] - h // 2), text, font=font, fill=fill)


def draw_circle(
    draw: ImageDraw.ImageDraw,
    center: tuple[int, int],
    radius: int,
    fill: tuple[int, int, int, int],
    outline: tuple[int, int, int, int] | None = None,
    width: int = 1,
) -> None:
    x, y = center
    draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=fill, outline=outline, width=width)


def base_image(include_background: bool = True) -> Image.Image:
    bg = (18, 18, 20, 255) if include_background else (0, 0, 0, 0)
    return Image.new("RGBA", (SIZE, SIZE), bg)


def draw_hex_base(draw: ImageDraw.ImageDraw, fill: tuple[int, int, int, int] = (128, 116, 95, 255)) -> None:
    draw.polygon(VERTICES, fill=fill, outline=(24, 22, 20, 255))
    draw.line(VERTICES + [VERTICES[0]], fill=(245, 228, 190, 255), width=5 * SCALE, joint="curve")


def hex_clip_mask() -> Image.Image:
    mask = Image.new("L", (SIZE, SIZE), 0)
    draw = ImageDraw.Draw(mask)
    draw.polygon(VERTICES, fill=255)
    return mask


HEX_CLIP = hex_clip_mask()


def clip_layer_to_hex(layer: Image.Image) -> Image.Image:
    clipped = layer.copy()
    alpha = clipped.getchannel("A")
    clipped.putalpha(ImageChops.multiply(alpha, HEX_CLIP))
    return clipped


def draw_ports_and_vertices(draw: ImageDraw.ImageDraw, label: bool = True) -> None:
    for v in VERTICES:
        draw_circle(draw, v, VERTEX_NO_GO, (210, 50, 38, 64), (230, 60, 48, 200), 3 * SCALE)
    for name, p in PORTS.items():
        draw_circle(draw, p, PORT_DOT, (50, 220, 110, 255), (5, 65, 30, 255), 3 * SCALE)
        if label:
            offset = (p[0] - CX, p[1] - CY)
            mag = max(math.hypot(*offset), 1)
            label_pos = (int(p[0] + offset[0] / mag * 120 * SCALE), int(p[1] + offset[1] / mag * 120 * SCALE))
            fit_text_center(draw, label_pos, name, FONT_L, (245, 245, 235, 255))


def side_center_label(side: str) -> str:
    return {
        "S0": "S0 top edge center",
        "S1": "S1 upper-right edge center",
        "S2": "S2 lower-right edge center",
        "S3": "S3 bottom edge center",
        "S4": "S4 lower-left edge center",
        "S5": "S5 upper-left edge center",
    }[side]


def draw_label_box(draw: ImageDraw.ImageDraw, lines: list[str]) -> None:
    x, y = 90 * SCALE, 82 * SCALE
    pad = 20 * SCALE
    line_h = 33 * SCALE
    width = max(draw.textbbox((0, 0), line, font=FONT_S)[2] for line in lines) + pad * 2
    height = line_h * len(lines) + pad * 2
    draw.rounded_rectangle((x, y, x + width, y + height), radius=12 * SCALE, fill=(10, 10, 12, 210))
    for idx, line in enumerate(lines):
        draw.text((x + pad, y + pad + idx * line_h), line, font=FONT_S, fill=(245, 238, 220, 255))


def downsample(img: Image.Image) -> Image.Image:
    return img.resize((CANVAS, CANVAS), Image.Resampling.LANCZOS)


def save(img: Image.Image, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    downsample(img).save(path)


def draw_polyline(
    draw: ImageDraw.ImageDraw,
    points: Iterable[tuple[int, int]],
    width: int,
    fill: tuple[int, int, int, int],
    outline_fill: tuple[int, int, int, int] | None = None,
    outline_width: int = 0,
) -> None:
    pts = list(points)
    if outline_fill and outline_width > 0:
        draw.line(pts, fill=outline_fill, width=width + outline_width * 2, joint="curve")
    draw.line(pts, fill=fill, width=width, joint="curve")


def draw_road_markings(draw: ImageDraw.ImageDraw, points: list[tuple[int, int]]) -> None:
    # Lightweight center marks placed on each segment. Exact topology is carried by the road body.
    for a, b in zip(points, points[1:]):
        for t in (0.28, 0.50, 0.72):
            c = lerp(a, b, t)
            dx, dy = b[0] - a[0], b[1] - a[1]
            length = max(math.hypot(dx, dy), 1)
            ux, uy = dx / length, dy / length
            mark_len = 70 * SCALE
            p1 = (int(c[0] - ux * mark_len / 2), int(c[1] - uy * mark_len / 2))
            p2 = (int(c[0] + ux * mark_len / 2), int(c[1] + uy * mark_len / 2))
            draw.line([p1, p2], fill=(216, 178, 75, 230), width=10 * SCALE)


def path_points(kind: str, a: str, b: str | None = None) -> list[tuple[int, int]]:
    if kind == "through" and b:
        return [PORTS[a], PORTS[b]]
    if kind == "end":
        return [PORTS[a], lerp(PORTS[a], (CX, CY), 0.70)]
    if kind == "turn_a" and b:
        # Single-bend elbow. Turn templates are for non-opposite side-center ports only.
        return [PORTS[a], (CX, CY), PORTS[b]]
    if kind == "turn_b" and b:
        # Dogleg with two bends. Turn templates are for non-opposite side-center ports only.
        pa, pb = PORTS[a], PORTS[b]
        return [
            pa,
            lerp(pa, (CX, CY), 0.60),
            lerp(pb, (CX, CY), 0.60),
            pb,
        ]
    raise ValueError(f"Unsupported path: {kind}, {a}, {b}")


def side_distance(a: str, b: str) -> int:
    ia = int(a[1])
    ib = int(b[1])
    diff = abs(ia - ib)
    return min(diff, 6 - diff)


def non_opposite_pairs() -> list[tuple[str, str]]:
    sides = [f"S{i}" for i in range(6)]
    return [(a, b) for a, b in combinations(sides, 2) if side_distance(a, b) != 3]


def clear_old_generated_templates() -> None:
    for folder, patterns in (
        (GUIDE_DIR, ("HEX_GUIDE_*.png", "HEX_TOPOLOGY_*.png")),
        (MASK_DIR, ("HEX_MASK_*.png",)),
    ):
        if not folder.exists():
            continue
        for pattern in patterns:
            for path in folder.glob(pattern):
                path.unlink()


def render_topology(
    asset: str,
    kind: str,
    a: str,
    b: str | None,
    label: str,
    guide_path: Path,
    mask_path: Path,
) -> dict:
    width = ROAD_WIDTH if asset == "Road" else RIVER_WIDTH
    fill = (45, 48, 48, 255) if asset == "Road" else (26, 157, 179, 230)
    outline = (22, 20, 18, 255) if asset == "Road" else (8, 78, 90, 255)

    pts = path_points(kind, a, b)

    img = base_image(True)
    draw = ImageDraw.Draw(img)
    draw_hex_base(draw)

    path_layer = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    pdraw = ImageDraw.Draw(path_layer)
    draw_polyline(pdraw, pts, width, fill, outline, 16 * SCALE)
    if asset == "Road":
        draw_road_markings(pdraw, pts)
    img.alpha_composite(clip_layer_to_hex(path_layer))

    draw_ports_and_vertices(draw, True)
    for p in pts:
        draw_circle(draw, p, 14 * SCALE, (255, 245, 85, 255), (55, 35, 0, 255), 2 * SCALE)
    lines = [
        label,
        f"Open port A: {side_center_label(a)}",
    ]
    if b:
        lines.append(f"Open port B: {side_center_label(b)}")
    else:
        lines.append("End cap: no second edge opening")
    lines.extend(
        [
            "Only side-center ports are valid",
            "Red vertex zones are forbidden",
        ]
    )
    draw_label_box(draw, lines)
    save(img, guide_path)

    # Clean mask/reference without text. Transparent outside the hex.
    mask = base_image(False)
    mdraw = ImageDraw.Draw(mask)
    mdraw.polygon(VERTICES, fill=(118, 106, 88, 255))

    mask_path_layer = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    mpdraw = ImageDraw.Draw(mask_path_layer)
    draw_polyline(mpdraw, pts, width, fill, outline, 14 * SCALE)
    if asset == "Road":
        draw_road_markings(mpdraw, pts)
    mask.alpha_composite(clip_layer_to_hex(mask_path_layer))

    mdraw.line(VERTICES + [VERTICES[0]], fill=(245, 230, 194, 255), width=4 * SCALE, joint="curve")
    save(mask, mask_path)

    return {
        "asset": asset,
        "kind": kind,
        "ports": [a] if b is None else [a, b],
        "guide": str(guide_path.relative_to(ROOT)).replace("\\", "/"),
        "mask": str(mask_path.relative_to(ROOT)).replace("\\", "/"),
    }


def render_port_guides() -> None:
    img = base_image(True)
    draw = ImageDraw.Draw(img)
    draw_hex_base(draw)
    draw_ports_and_vertices(draw, True)
    draw_label_box(
        draw,
        [
            "Regular Hex Port Map",
            "S0-S5 are the only valid connection ports",
            "Every road/river must enter/exit at side center",
            "Vertices are not connection points",
        ],
    )
    save(img, GUIDE_DIR / "HEX_GUIDE_01_PortMap_S0_S5.png")

    img2 = base_image(True)
    draw2 = ImageDraw.Draw(img2)
    draw_hex_base(draw2)
    for v in VERTICES:
        draw_circle(draw2, v, VERTEX_NO_GO, (225, 25, 25, 96), (255, 70, 70, 230), 5 * SCALE)
        fit_text_center(draw2, v, "NO", FONT_M, (255, 238, 238, 255))
    for name, p in PORTS.items():
        draw_circle(draw2, p, 26 * SCALE, (50, 230, 120, 255), (5, 70, 35, 255), 4 * SCALE)
        fit_text_center(draw2, (p[0], p[1] - 58 * SCALE), name, FONT_L, (245, 245, 235, 255))
    draw_label_box(
        draw2,
        [
            "Vertex No-Go Guide",
            "Path width must never include red vertex zones",
            "Path centerline must hit green side-center ports",
        ],
    )
    save(img2, GUIDE_DIR / "HEX_GUIDE_02_VertexNoGo_Zones.png")


def main() -> None:
    GUIDE_DIR.mkdir(parents=True, exist_ok=True)
    MASK_DIR.mkdir(parents=True, exist_ok=True)
    clear_old_generated_templates()
    render_port_guides()

    manifest: list[dict] = []

    through_pairs = [("S0", "S3"), ("S1", "S4"), ("S2", "S5")]
    for asset in ("Road", "River"):
        for a, b in through_pairs:
            name = f"{asset}Through_{a}_{b}"
            manifest.append(
                render_topology(
                    asset,
                    "through",
                    a,
                    b,
                    f"{asset} through: {a} to {b}",
                    GUIDE_DIR / f"HEX_TOPOLOGY_{name}.png",
                    MASK_DIR / f"HEX_MASK_{name}.png",
                )
            )

    for asset in ("Road", "River"):
        for a in ("S0", "S1", "S2", "S3", "S4", "S5"):
            name = f"{asset}End_{a}"
            manifest.append(
                render_topology(
                    asset,
                    "end",
                    a,
                    None,
                    f"{asset} end cap: {a}",
                    GUIDE_DIR / f"HEX_TOPOLOGY_{name}.png",
                    MASK_DIR / f"HEX_MASK_{name}.png",
                )
            )

    # User sketch A/B examples as exact topology references.
    # A/B are not pass-through/opposite-side patterns. They are non-opposite
    # side-center-to-side-center turns; the port pair stays fixed and only the
    # internal path differs.
    turn_pairs = non_opposite_pairs()
    for asset in ("Road", "River"):
        for kind, tag in (("turn_a", "TurnA"), ("turn_b", "TurnB")):
            for a, b in turn_pairs:
                name = f"{asset}{tag}_{a}_{b}"
                manifest.append(
                    render_topology(
                        asset,
                        kind,
                        a,
                        b,
                        f"{asset} {tag}: {a} to {b}",
                        GUIDE_DIR / f"HEX_TOPOLOGY_{name}.png",
                        MASK_DIR / f"HEX_MASK_{name}.png",
                    )
                )

    metadata = {
        "side_labels": {
            "S0": "top horizontal edge center",
            "S1": "upper-right slanted edge center",
            "S2": "lower-right slanted edge center",
            "S3": "bottom horizontal edge center",
            "S4": "lower-left slanted edge center",
            "S5": "upper-left slanted edge center",
        },
        "rules": [
            "Road and river endpoints must be side-center ports only.",
            "No endpoint may touch or include a hex vertex.",
            "The full road/river width must leave visible dry-ground clearance from adjacent vertices.",
            "Use mask files as topology locks; use guide files for human-readable review.",
            "Final 3D models should snap port centers to exact mesh-side midpoint coordinates.",
        ],
        "templates": manifest,
    }
    with (OUT_DIR / "hex_topology_manifest.json").open("w", encoding="utf-8") as fp:
        json.dump(metadata, fp, indent=2, ensure_ascii=False)

    print(f"Generated {len(manifest)} topology templates")
    print(f"Guides: {GUIDE_DIR}")
    print(f"Masks:  {MASK_DIR}")


if __name__ == "__main__":
    main()
