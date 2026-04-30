import unreal


BLUEPRINT_PATH = "/Game/Blueprints"
BLUEPRINT_NAME = "BP_HexGridManager"
HEX_TILES_PATH = "/Game/Hextiles"


def load_static_meshes_under_folder(folder_path):
    meshes = []
    for asset_path in editor_asset_library.list_assets(folder_path, recursive=True, include_folder=False):
        asset = editor_asset_library.load_asset(asset_path)
        if isinstance(asset, unreal.StaticMesh):
            meshes.append(asset)

    meshes.sort(key=lambda mesh: mesh.get_path_name())
    return meshes


asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_asset_library = unreal.EditorAssetLibrary

editor_asset_library.make_directory(BLUEPRINT_PATH)

blueprint_asset_path = f"{BLUEPRINT_PATH}/{BLUEPRINT_NAME}"
existing_asset = editor_asset_library.load_asset(blueprint_asset_path)

if existing_asset:
    blueprint = existing_asset
else:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", unreal.HexGridManager)
    blueprint = asset_tools.create_asset(
        asset_name=BLUEPRINT_NAME,
        package_path=BLUEPRINT_PATH,
        asset_class=unreal.Blueprint,
        factory=factory,
    )

tile_meshes = load_static_meshes_under_folder(HEX_TILES_PATH)
if not tile_meshes:
    raise RuntimeError(f"No StaticMesh assets found under: {HEX_TILES_PATH}")

default_tile_mesh = tile_meshes[0]

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)

generated_class = unreal.BlueprintEditorLibrary.generated_class(blueprint)
if not generated_class:
    raise RuntimeError(f"Generated Blueprint class not found: {blueprint_asset_path}")

actor_cdo = unreal.get_default_object(generated_class)

try:
    actor_cdo.set_editor_property("AvailableTileMeshes", tile_meshes)
except Exception:
    unreal.log_warning("AvailableTileMeshes property is not available yet. Compile the updated C++ code, then rerun this script.")

try:
    actor_cdo.set_editor_property("TileMesh", default_tile_mesh)
except Exception:
    unreal.log_warning("TileMesh property is not available yet. Compile the updated C++ code, then rerun this script.")

try:
    actor_cdo.set_editor_property("GridRadius", 5)
except Exception:
    unreal.log_warning("GridRadius property was not available.")

try:
    actor_cdo.set_editor_property("TileSize", 100.0)
except Exception:
    unreal.log_warning("TileSize property was not available.")

try:
    actor_cdo.set_editor_property("bUseMeshBoundsForTileSpacing", True)
except Exception:
    unreal.log_warning("bUseMeshBoundsForTileSpacing property was not available.")

try:
    actor_cdo.rebuild_tile_slots()
except Exception:
    unreal.log_warning("RebuildTileSlots function was not available.")

unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
unreal.log(f"Created or updated {blueprint_asset_path} with {len(tile_meshes)} hex tile mesh(es) from {HEX_TILES_PATH}")
