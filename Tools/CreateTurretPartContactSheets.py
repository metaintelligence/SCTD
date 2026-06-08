from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "Content" / "Images" / "TurretParts"


def load_font(size: int) -> ImageFont.ImageFont:
    for path in ("C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf"):
        try:
            return ImageFont.truetype(path, size)
        except Exception:
            pass
    return ImageFont.load_default()


def make_sheet(folder: Path, files: list[Path], columns: int = 2, thumb_size: int = 360) -> Path:
    title_h = 46
    label_h = 48
    rows = (len(files) + columns - 1) // columns
    width = columns * thumb_size
    height = title_h + rows * (thumb_size + label_h)
    sheet = Image.new("RGB", (width, height), (28, 28, 30))
    draw = ImageDraw.Draw(sheet)
    title_font = load_font(22)
    label_font = load_font(15)

    title = folder.relative_to(BASE).as_posix()
    draw.rectangle((0, 0, width, title_h), fill=(20, 20, 22))
    draw.text((12, 8), title, fill=(235, 232, 220), font=title_font)

    for idx, path in enumerate(files):
        col = idx % columns
        row = idx // columns
        x0 = col * thumb_size
        y0 = title_h + row * (thumb_size + label_h)
        img = Image.open(path).convert("RGB")
        img.thumbnail((thumb_size, thumb_size), Image.Resampling.LANCZOS)
        x = x0 + (thumb_size - img.width) // 2
        y = y0 + (thumb_size - img.height) // 2
        sheet.paste(img, (x, y))
        label = path.stem.replace("TILE_", "")
        draw.text((x0 + 8, y0 + thumb_size + 8), label[:42], fill=(235, 232, 220), font=label_font)

    out = folder / f"_{folder.parent.name}_{folder.name}_ContactSheet.png"
    sheet.save(out, compress_level=1)
    return out


def make_index(files: list[Path]) -> Path:
    columns = 4
    thumb_size = 300
    label_h = 54
    title_h = 56
    rows = (len(files) + columns - 1) // columns
    width = columns * thumb_size
    height = title_h + rows * (thumb_size + label_h)
    sheet = Image.new("RGB", (width, height), (27, 27, 29))
    draw = ImageDraw.Draw(sheet)
    title_font = load_font(24)
    label_font = load_font(12)
    draw.rectangle((0, 0, width, title_h), fill=(20, 20, 22))
    draw.text((12, 12), f"SCTD Turret Parts ({len(files)} assets)", fill=(235, 232, 220), font=title_font)

    for idx, path in enumerate(files):
        col = idx % columns
        row = idx // columns
        x0 = col * thumb_size
        y0 = title_h + row * (thumb_size + label_h)
        img = Image.open(path).convert("RGB")
        img.thumbnail((thumb_size, thumb_size), Image.Resampling.LANCZOS)
        x = x0 + (thumb_size - img.width) // 2
        y = y0 + (thumb_size - img.height) // 2
        sheet.paste(img, (x, y))
        label = path.relative_to(BASE).as_posix().replace("TILE_", "").replace(".png", "")
        draw.text((x0 + 8, y0 + thumb_size + 8), label[:44], fill=(235, 232, 220), font=label_font)

    out = BASE / "_TurretParts_Index.png"
    sheet.save(out, compress_level=1)
    return out


def main() -> None:
    leaf_folders = sorted({path.parent for path in BASE.rglob("TILE_*.png")})
    all_files: list[Path] = []
    for folder in leaf_folders:
        files = sorted(folder.glob("TILE_*.png"))
        if not files:
            continue
        make_sheet(folder, files)
        all_files.extend(files)
    make_index(sorted(all_files))
    print(f"Created contact sheets for {len(leaf_folders)} folders")
    print(f"Indexed {len(all_files)} turret part assets")


if __name__ == "__main__":
    main()
