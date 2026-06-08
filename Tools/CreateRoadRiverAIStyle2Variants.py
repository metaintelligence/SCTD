from __future__ import annotations

import hashlib
import math
import random
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageEnhance, ImageFilter, ImageFont, ImageOps


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "Content" / "Images" / "Terrain" / "RoadRiver_AIStyle"
DST = ROOT / "Content" / "Images" / "Terrain" / "RoadRiver_AIStyle2"
MASK_SRC = SRC / "TopologyMasks"
MASK_DST = DST / "TopologyMasks"


def seed_for(path: Path) -> int:
    return int(hashlib.sha256(path.name.encode("utf-8")).hexdigest()[:8], 16)


def tile_mask(img: Image.Image) -> Image.Image:
    gray = ImageOps.grayscale(img)
    # Keep the actual slab/tile and ignore most of the dark studio background.
    mask = gray.point(lambda p: 255 if p > 38 else 0)
    return mask.filter(ImageFilter.MaxFilter(5)).filter(ImageFilter.GaussianBlur(0.6))


def random_point_in_mask(mask: Image.Image, rng: random.Random) -> tuple[int, int]:
    w, h = mask.size
    for _ in range(200):
        x = rng.randrange(24, w - 24)
        y = rng.randrange(24, h - 24)
        if mask.getpixel((x, y)) > 180:
            return x, y
    return w // 2, h // 2


def add_radial_grade(img: Image.Image, rng: random.Random) -> Image.Image:
    w, h = img.size
    cx = w * rng.uniform(0.42, 0.58)
    cy = h * rng.uniform(0.30, 0.46)
    max_d = math.hypot(max(cx, w - cx), max(cy, h - cy))
    grade = Image.new("L", (w, h), 0)
    px = grade.load()
    for y in range(h):
        for x in range(w):
            d = math.hypot(x - cx, y - cy) / max_d
            px[x, y] = max(0, min(255, int((d ** 1.55) * 70)))
    shadow = Image.new("RGB", (w, h), (0, 0, 0))
    return Image.composite(shadow, img, grade)


def blend_tint(img: Image.Image, rng: random.Random, is_water: bool) -> Image.Image:
    tint = (154, 126, 82) if not is_water else rng.choice([(132, 121, 90), (104, 126, 123), (142, 116, 82)])
    layer = Image.new("RGB", img.size, tint)
    return Image.blend(img, layer, rng.uniform(0.025, 0.055))


def add_weathering(img: Image.Image, rng: random.Random, is_water: bool) -> Image.Image:
    mask = tile_mask(img)
    overlay = Image.new("RGBA", img.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    # Fine dust and chips. Kept subtle so the accepted AI style remains intact.
    for _ in range(220):
        x, y = random_point_in_mask(mask, rng)
        r = rng.randint(1, 4)
        color = rng.choice(
            [
                (228, 202, 145, rng.randint(10, 28)),
                (55, 43, 31, rng.randint(10, 24)),
                (180, 142, 88, rng.randint(8, 20)),
            ]
        )
        draw.ellipse((x - r, y - r, x + r, y + r), fill=color)

    # Hairline cracks and scratches.
    for _ in range(38):
        x, y = random_point_in_mask(mask, rng)
        length = rng.randint(20, 95)
        angle = rng.choice([0, math.pi / 3, -math.pi / 3, math.pi / 6, -math.pi / 6]) + rng.uniform(-0.18, 0.18)
        x2 = int(x + math.cos(angle) * length)
        y2 = int(y + math.sin(angle) * length)
        alpha = rng.randint(22, 55) if not is_water else rng.randint(12, 36)
        draw.line((x, y, x2, y2), fill=(22, 17, 12, alpha), width=rng.choice([1, 1, 2]))

    # A few warm low-poly dust facets.
    for _ in range(18):
        x, y = random_point_in_mask(mask, rng)
        s = rng.randint(10, 34)
        pts = [
            (x + rng.randint(-s, s), y + rng.randint(-s, s)),
            (x + rng.randint(-s, s), y + rng.randint(-s, s)),
            (x + rng.randint(-s, s), y + rng.randint(-s, s)),
        ]
        draw.polygon(pts, fill=rng.choice([(205, 178, 120, 18), (80, 60, 35, 16), (240, 220, 165, 12)]))

    # Softly clip weathering to the tile body, not the outside background.
    alpha = overlay.getchannel("A")
    overlay.putalpha(ImageChops.multiply(alpha, mask))
    return Image.alpha_composite(img.convert("RGBA"), overlay).convert("RGB")


def variant_image(path: Path) -> Image.Image:
    rng = random.Random(seed_for(path))
    img = Image.open(path).convert("RGB")
    is_water = "River" in path.name

    img = ImageEnhance.Color(img).enhance(rng.uniform(0.94, 1.08))
    img = ImageEnhance.Contrast(img).enhance(rng.uniform(1.035, 1.095))
    img = ImageEnhance.Brightness(img).enhance(rng.uniform(0.965, 1.035))
    img = blend_tint(img, rng, is_water)
    img = add_radial_grade(img, rng)
    img = add_weathering(img, rng, is_water)
    img = ImageEnhance.Sharpness(img).enhance(1.05)
    return img


def make_contact_sheet(files: list[Path]) -> None:
    thumb_w, thumb_h = 360, 360
    label_h = 54
    cols = 5
    rows = (len(files) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * thumb_w, rows * (thumb_h + label_h)), (30, 30, 32))
    draw = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype("C:/Windows/Fonts/segoeui.ttf", 18)
    except Exception:
        font = ImageFont.load_default()
    for idx, path in enumerate(files):
        img = Image.open(path).convert("RGB")
        img.thumbnail((thumb_w, thumb_h), Image.Resampling.LANCZOS)
        x = (idx % cols) * thumb_w + (thumb_w - img.width) // 2
        y = (idx // cols) * (thumb_h + label_h)
        sheet.paste(img, (x, y))
        label = path.stem.replace("TILE_", "")
        draw.text(((idx % cols) * thumb_w + 8, y + thumb_h + 4), label[:42], fill=(235, 232, 220), font=font)
    sheet.save(DST / "_AIStyle2_ContactSheet.png", compress_level=1)


def main() -> None:
    DST.mkdir(parents=True, exist_ok=True)
    MASK_DST.mkdir(parents=True, exist_ok=True)

    for old in DST.glob("TILE_*.png"):
        old.unlink()
    for old in DST.glob("_AIStyle2_ContactSheet.png"):
        old.unlink()

    for mask in MASK_SRC.glob("*.png"):
        (MASK_DST / mask.name).write_bytes(mask.read_bytes())

    outputs: list[Path] = []
    for src in sorted(SRC.glob("TILE_*.png")):
        out = DST / src.name
        variant_image(src).save(out, compress_level=1)
        outputs.append(out)

    make_contact_sheet(outputs)
    (DST / "README.md").write_text(
        "# Road / River AI Style 2 Tiles\n\n"
        "This folder is a second sample set derived from the approved `RoadRiver_AIStyle` renders.\n"
        "The built-in image generator returned unrelated infographic/medical images during the new live generation attempt, so this set preserves the accepted style and topology by applying local art-direction variations to the approved images.\n\n"
        "- 20 terrain tile variants are in this folder.\n"
        "- 20 matching topology masks are in `TopologyMasks`.\n"
        "- `_AIStyle2_ContactSheet.png` is the visual review sheet.\n\n"
        "The deterministic topology masks remain the source of truth for exact 3D modeling ports.\n",
        encoding="utf-8",
    )
    print(f"Created {len(outputs)} RoadRiver_AIStyle2 variants")
    print(DST)


if __name__ == "__main__":
    main()
