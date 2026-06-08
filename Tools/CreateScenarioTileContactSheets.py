from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "Content" / "Images" / "Terrain" / "ScenarioTiles"


def font(size: int) -> ImageFont.ImageFont:
    for path in (
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
    ):
        try:
            return ImageFont.truetype(path, size)
        except Exception:
            pass
    return ImageFont.load_default()


def make_sheet(folder: Path, files: list[Path], columns: int = 4, thumb_size: int = 256) -> Path:
    label_h = 54
    title_h = 42
    rows = (len(files) + columns - 1) // columns
    width = columns * thumb_size
    height = title_h + rows * (thumb_size + label_h)
    sheet = Image.new("RGB", (width, height), (28, 28, 30))
    draw = ImageDraw.Draw(sheet)
    title_font = font(22)
    label_font = font(13)

    title = str(folder.relative_to(BASE)).replace("\\", " / ")
    draw.rectangle((0, 0, width, title_h), fill=(22, 22, 24))
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
        draw.text((x0 + 8, y0 + thumb_size + 6), label[:34], fill=(235, 232, 220), font=label_font)

    out = folder / f"_{folder.name}_ContactSheet.png"
    sheet.save(out, compress_level=1)
    return out


def make_index(all_files: list[Path]) -> Path:
    columns = 8
    thumb_size = 170
    label_h = 42
    title_h = 52
    rows = (len(all_files) + columns - 1) // columns
    width = columns * thumb_size
    height = title_h + rows * (thumb_size + label_h)
    sheet = Image.new("RGB", (width, height), (27, 27, 29))
    draw = ImageDraw.Draw(sheet)
    title_font = font(24)
    label_font = font(10)
    draw.rectangle((0, 0, width, title_h), fill=(20, 20, 22))
    draw.text((12, 12), f"SCTD Scenario Tiles ({len(all_files)} assets)", fill=(235, 232, 220), font=title_font)

    for idx, path in enumerate(all_files):
        col = idx % columns
        row = idx // columns
        x0 = col * thumb_size
        y0 = title_h + row * (thumb_size + label_h)
        img = Image.open(path).convert("RGB")
        img.thumbnail((thumb_size, thumb_size), Image.Resampling.LANCZOS)
        x = x0 + (thumb_size - img.width) // 2
        y = y0 + (thumb_size - img.height) // 2
        sheet.paste(img, (x, y))
        rel = path.relative_to(BASE)
        label = str(rel.parent).replace("\\", "/") + "/" + path.stem.replace("TILE_", "")
        draw.text((x0 + 5, y0 + thumb_size + 4), label[:38], fill=(235, 232, 220), font=label_font)

    out = BASE / "_ScenarioTiles_Index.png"
    sheet.save(out, compress_level=1)
    return out


def write_readme(leaf_folders: list[Path], all_files: list[Path]) -> None:
    lines = [
        "# SCTD Scenario Terrain Tiles",
        "",
        "Low-poly/cartoon hex terrain concept tiles for scenario immersion, monster spawning, resources, hazards, and defense gameplay.",
        "",
        f"Total generated tile images: {len(all_files)}",
        "",
        "## Folders",
    ]
    for folder in leaf_folders:
        files = sorted(folder.glob("TILE_*.png"))
        rel = folder.relative_to(BASE).as_posix()
        lines.append(f"- {rel}: {len(files)}")
    lines.append("")
    lines.append("Each leaf folder includes a contact sheet for quick visual review.")
    (BASE / "README.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


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
    write_readme(leaf_folders, sorted(all_files))
    print(f"Created contact sheets for {len(leaf_folders)} folders")
    print(f"Indexed {len(all_files)} scenario tile assets")


if __name__ == "__main__":
    main()
