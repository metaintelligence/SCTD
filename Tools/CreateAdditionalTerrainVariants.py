from __future__ import annotations

import hashlib
import math
import random
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageEnhance, ImageFilter, ImageFont, ImageOps


ROOT = Path(__file__).resolve().parents[1]
TERRAIN = ROOT / "Content" / "Images" / "Terrain"


@dataclass(frozen=True)
class VariantSpec:
    category: str
    filename: str
    source: str
    theme: str
    seed_hint: str
    mirror: bool = False


SPECS = [
    VariantSpec("CollapsedFoundation", "TILE_CollapsedFoundation_01_FracturedPillars.png", "TILE_UrbanRuins_02_CollapsedFoundation.png", "foundation", "fractured-pillars"),
    VariantSpec("CollapsedFoundation", "TILE_CollapsedFoundation_02_RebarBasement.png", "TILE_UrbanRuins_02_CollapsedFoundation.png", "foundation", "rebar-basement", True),
    VariantSpec("CollapsedFoundation", "TILE_CollapsedFoundation_03_CornerRubble.png", "TILE_UrbanRuins_03_ReclaimableMetroPlaza.png", "foundation", "corner-rubble"),
    VariantSpec("RockMountain", "TILE_RockMountain_01_JaggedGraniteSpire.png", "TILE_Mountain_01_ErodedRidge.png", "rock_mountain", "granite-spire"),
    VariantSpec("RockMountain", "TILE_RockMountain_02_SplitBasaltRidge.png", "TILE_Mountain_02_SteepFoothill.png", "rock_mountain", "basalt-ridge", True),
    VariantSpec("RockMountain", "TILE_RockMountain_03_BoulderCrown.png", "TILE_Rock_01_BoulderField.png", "rock_mountain", "boulder-crown"),
    VariantSpec("CrackedConcretePlaza", "TILE_CrackedConcretePlaza_01_SunkenGrid.png", "TILE_UrbanRuins_01_CrackedConcretePlaza.png", "concrete", "sunken-grid"),
    VariantSpec("CrackedConcretePlaza", "TILE_CrackedConcretePlaza_02_RebarFaults.png", "TILE_UrbanRuins_01_CrackedConcretePlaza.png", "concrete", "rebar-faults", True),
    VariantSpec("CrackedConcretePlaza", "TILE_CrackedConcretePlaza_03_SealedVaultSlab.png", "TILE_UrbanRuins_03_ReclaimableMetroPlaza.png", "concrete", "vault-slab"),
    VariantSpec("SteelScrapField", "TILE_SteelScrapField_01_RustedPlateScatter.png", "TILE_Resource_01_SteelScrapField.png", "steel_scrap", "plate-scatter"),
    VariantSpec("SteelScrapField", "TILE_SteelScrapField_02_PipeAndPanelDrift.png", "TILE_Resource_01_SteelScrapField.png", "steel_scrap", "pipe-panel", True),
    VariantSpec("SteelScrapField", "TILE_SteelScrapField_03_SalvageShards.png", "TILE_Resource_03_IndustrialSalvageCache.png", "steel_scrap", "salvage-shards"),
    VariantSpec("IndustrialSalvageCache", "TILE_IndustrialSalvageCache_01_CrateNode.png", "TILE_Resource_03_IndustrialSalvageCache.png", "industrial", "crate-node"),
    VariantSpec("IndustrialSalvageCache", "TILE_IndustrialSalvageCache_02_CableDepot.png", "TILE_Resource_03_IndustrialSalvageCache.png", "industrial", "cable-depot", True),
    VariantSpec("IndustrialSalvageCache", "TILE_IndustrialSalvageCache_03_ScrapRelay.png", "TILE_Resource_01_SteelScrapField.png", "industrial", "scrap-relay"),
]


def seed(spec: VariantSpec) -> int:
    data = f"{spec.category}|{spec.filename}|{spec.seed_hint}".encode("utf-8")
    return int(hashlib.sha256(data).hexdigest()[:8], 16)


def tile_mask(img: Image.Image) -> Image.Image:
    gray = ImageOps.grayscale(img)
    mask = gray.point(lambda p: 255 if p > 36 else 0)
    return mask.filter(ImageFilter.MaxFilter(5)).filter(ImageFilter.GaussianBlur(0.8))


def point_in_mask(mask: Image.Image, rng: random.Random, margin: int = 50) -> tuple[int, int]:
    w, h = mask.size
    for _ in range(250):
        x = rng.randint(margin, w - margin)
        y = rng.randint(margin, h - margin)
        if mask.getpixel((x, y)) > 170:
            return x, y
    return w // 2, h // 2


def inside_mask(mask: Image.Image, point: tuple[int, int]) -> bool:
    x, y = point
    if x < 0 or y < 0 or x >= mask.width or y >= mask.height:
        return False
    return mask.getpixel((x, y)) > 150


def add_color_grade(img: Image.Image, rng: random.Random, theme: str) -> Image.Image:
    img = ImageEnhance.Color(img).enhance(rng.uniform(0.94, 1.10))
    img = ImageEnhance.Contrast(img).enhance(rng.uniform(1.035, 1.12))
    img = ImageEnhance.Brightness(img).enhance(rng.uniform(0.965, 1.035))

    tint_map = {
        "foundation": [(128, 119, 101), (139, 122, 92), (112, 115, 105)],
        "rock_mountain": [(124, 116, 99), (138, 122, 93), (104, 111, 112)],
        "concrete": [(132, 127, 112), (116, 120, 116), (146, 128, 96)],
        "steel_scrap": [(139, 116, 88), (126, 111, 97), (145, 123, 86)],
        "industrial": [(118, 116, 103), (135, 116, 88), (105, 121, 119)],
    }
    tint = Image.new("RGB", img.size, rng.choice(tint_map[theme]))
    return Image.blend(img, tint, rng.uniform(0.025, 0.06))


def add_vignette(img: Image.Image, rng: random.Random) -> Image.Image:
    w, h = img.size
    cx = w * rng.uniform(0.43, 0.57)
    cy = h * rng.uniform(0.34, 0.50)
    max_d = math.hypot(max(cx, w - cx), max(cy, h - cy))
    grade = Image.new("L", (w, h), 0)
    px = grade.load()
    for y in range(h):
        for x in range(w):
            d = math.hypot(x - cx, y - cy) / max_d
            px[x, y] = max(0, min(255, int((d**1.65) * 56)))
    return Image.composite(Image.new("RGB", img.size, (0, 0, 0)), img, grade)


def draw_lowpoly_rock(draw: ImageDraw.ImageDraw, rng: random.Random, x: int, y: int, size: int) -> None:
    pts = []
    sides = rng.randint(5, 8)
    for i in range(sides):
        a = math.tau * i / sides + rng.uniform(-0.18, 0.18)
        r = size * rng.uniform(0.55, 1.05)
        pts.append((int(x + math.cos(a) * r), int(y + math.sin(a) * r)))
    base = rng.choice([(93, 87, 75), (111, 103, 88), (76, 80, 78), (129, 117, 95)])
    draw.polygon(pts, fill=base + (225,), outline=(42, 36, 30, 145))
    for _ in range(rng.randint(2, 4)):
        tri = rng.sample(pts, min(3, len(pts)))
        color = tuple(min(255, max(0, c + rng.randint(-22, 24))) for c in base)
        draw.polygon(tri, fill=color + (80,))


def draw_scrap_plate(draw: ImageDraw.ImageDraw, rng: random.Random, x: int, y: int, size: int) -> None:
    w = int(size * rng.uniform(1.0, 1.7))
    h = int(size * rng.uniform(0.55, 1.05))
    angle = rng.uniform(-0.8, 0.8)
    corners = [(-w, -h), (w, -h), (w, h), (-w, h)]
    pts = []
    for px, py in corners:
        pts.append((int(x + math.cos(angle) * px - math.sin(angle) * py), int(y + math.sin(angle) * px + math.cos(angle) * py)))
    col = rng.choice([(118, 75, 45), (86, 88, 82), (129, 93, 55), (62, 70, 69)])
    draw.polygon(pts, fill=col + (220,), outline=(33, 25, 19, 150))
    for p in pts[::2]:
        draw.ellipse((p[0] - 4, p[1] - 4, p[0] + 4, p[1] + 4), fill=(30, 22, 16, 180))
    draw.line([pts[0], pts[2]], fill=(190, 136, 78, 55), width=2)


def draw_rebar(draw: ImageDraw.ImageDraw, rng: random.Random, x: int, y: int, length: int) -> None:
    angle = rng.uniform(-1.1, 1.1)
    for offset in range(rng.randint(1, 3)):
        ox = rng.randint(-10, 10)
        oy = rng.randint(-10, 10)
        x1 = int(x + ox - math.cos(angle) * length / 2)
        y1 = int(y + oy - math.sin(angle) * length / 2)
        x2 = int(x + ox + math.cos(angle) * length / 2)
        y2 = int(y + oy + math.sin(angle) * length / 2)
        draw.line((x1, y1, x2, y2), fill=(64, 37, 24, 190), width=rng.choice([2, 3, 4]))


def draw_foundation_block(draw: ImageDraw.ImageDraw, rng: random.Random, x: int, y: int, size: int) -> None:
    w = int(size * rng.uniform(0.85, 1.45))
    h = int(size * rng.uniform(0.42, 0.8))
    col = rng.choice([(111, 107, 95), (127, 119, 101), (92, 95, 90)])
    draw.rounded_rectangle((x - w, y - h, x + w, y + h), radius=5, fill=col + (225,), outline=(42, 38, 32, 150), width=2)
    draw.line((x - w + 8, y - h + 6, x + w - 10, y - h + 4), fill=(214, 184, 126, 80), width=2)
    draw.line((x - w + 12, y + h - 5, x + w - 8, y + h - 8), fill=(31, 26, 22, 80), width=2)


def add_weathering_and_objects(img: Image.Image, spec: VariantSpec, rng: random.Random) -> Image.Image:
    mask = tile_mask(img)
    overlay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    # Subtle extra cracks, dust and low-poly chips.
    for _ in range(150):
        x, y = point_in_mask(mask, rng)
        radius = rng.randint(1, 4)
        fill = rng.choice([(230, 206, 145, 20), (48, 38, 28, 25), (166, 130, 80, 18)])
        draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=fill)

    for _ in range(42):
        x, y = point_in_mask(mask, rng)
        length = rng.randint(22, 118)
        angle = rng.choice([0, math.pi / 3, -math.pi / 3, math.pi / 6, -math.pi / 6]) + rng.uniform(-0.2, 0.2)
        x2 = int(x + math.cos(angle) * length)
        y2 = int(y + math.sin(angle) * length)
        if inside_mask(mask, (x2, y2)):
            draw.line((x, y, x2, y2), fill=(27, 21, 16, rng.randint(20, 58)), width=rng.choice([1, 1, 2]))

    # Keep additions subtle. Large drawn objects look flatter than the source
    # render, so this pass only adds weathering-level details.
    extra_scratches = {
        "foundation": 14,
        "rock_mountain": 10,
        "concrete": 18,
        "steel_scrap": 10,
        "industrial": 10,
    }[spec.theme]
    for _ in range(extra_scratches):
        x, y = point_in_mask(mask, rng)
        draw_rebar(draw, rng, x, y, rng.randint(18, 58))

    alpha = ImageChops.multiply(overlay.getchannel("A"), mask)
    overlay.putalpha(alpha)
    return Image.alpha_composite(img.convert("RGBA"), overlay).convert("RGB")


def make_variant(spec: VariantSpec) -> Image.Image:
    rng = random.Random(seed(spec))
    src_path = TERRAIN / spec.source
    if not src_path.exists():
        raise FileNotFoundError(src_path)

    img = Image.open(src_path).convert("RGB")
    if spec.mirror:
        img = ImageOps.mirror(img)
    img = add_color_grade(img, rng, spec.theme)
    img = add_vignette(img, rng)
    img = add_weathering_and_objects(img, spec, rng)
    img = ImageEnhance.Sharpness(img).enhance(1.04)
    return img


def make_contact_sheet(category: str, files: list[Path]) -> None:
    thumb_w, thumb_h = 430, 430
    label_h = 46
    sheet = Image.new("RGB", (thumb_w * 3, thumb_h + label_h), (30, 30, 32))
    draw = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype("C:/Windows/Fonts/segoeui.ttf", 18)
    except Exception:
        font = ImageFont.load_default()

    for idx, path in enumerate(sorted(files)):
        img = Image.open(path).convert("RGB")
        img.thumbnail((thumb_w, thumb_h), Image.Resampling.LANCZOS)
        x = idx * thumb_w + (thumb_w - img.width) // 2
        y = (thumb_h - img.height) // 2
        sheet.paste(img, (x, y))
        draw.text((idx * thumb_w + 8, thumb_h + 5), path.stem[:42], fill=(235, 232, 220), font=font)
    sheet.save(TERRAIN / category / f"_{category}_ContactSheet.png", compress_level=1)


def main() -> None:
    by_category: dict[str, list[Path]] = {}
    for spec in SPECS:
        out_dir = TERRAIN / spec.category
        out_dir.mkdir(parents=True, exist_ok=True)
        for old in out_dir.glob("TILE_*.png"):
            old.unlink()
        by_category.setdefault(spec.category, [])

    for spec in SPECS:
        out_dir = TERRAIN / spec.category
        out_path = out_dir / spec.filename
        make_variant(spec).save(out_path, compress_level=1)
        by_category[spec.category].append(out_path)

    for category, files in by_category.items():
        make_contact_sheet(category, files)
        (TERRAIN / category / "README.md").write_text(
            f"# {category}\n\n"
            "Three additional SCTD terrain tile samples generated as style-preserving variants from the approved terrain references.\n"
            "The live built-in image generator returned an unrelated infographic during the first test of this batch, so these files were produced locally to preserve the accepted terrain style.\n",
            encoding="utf-8",
        )

    total = sum(len(files) for files in by_category.values())
    print(f"Created {total} terrain variants")
    for category, files in sorted(by_category.items()):
        print(f"{category}: {len(files)}")


if __name__ == "__main__":
    main()
