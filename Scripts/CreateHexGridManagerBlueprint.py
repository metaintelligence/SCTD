import unreal


BLUEPRINT_PATH = "/Game/Blueprints"
BLUEPRINT_NAME = "BP_HexGridManager"
TILE_MESH_PATH = "/Game/hex_tile_coal/StaticMeshes/hex_tile_coal.hex_tile_coal"


def find_component_by_name(actor_cdo, component_name):
    for component in actor_cdo.get_components_by_class(unreal.ActorComponent):
        if component.get_name() == component_name:
            return component
    return None


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

tile_mesh = editor_asset_library.load_asset(TILE_MESH_PATH)
if not tile_mesh:
    raise RuntimeError(f"Tile mesh not found: {TILE_MESH_PATH}")

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)

generated_class = unreal.BlueprintEditorLibrary.generated_class(blueprint)
if not generated_class:
    raise RuntimeError(f"Generated Blueprint class not found: {blueprint_asset_path}")

actor_cdo = unreal.get_default_object(generated_class)

try:
    actor_cdo.set_editor_property("TileMesh", tile_mesh)
except Exception:
    unreal.log_warning("TileMesh property is not available yet. Compile the updated C++ code, then rerun this script.")

tile_instances = find_component_by_name(actor_cdo, "TileInstances")
if tile_instances:
    tile_instances.set_editor_property("StaticMesh", tile_mesh)
else:
    unreal.log_warning("TileInstances component was not found on BP_HexGridManager.")

try:
    actor_cdo.set_editor_property("GridRadius", 5)
except Exception:
    unreal.log_warning("GridRadius property was not available.")

try:
    actor_cdo.set_editor_property("TileSize", 100.0)
except Exception:
    unreal.log_warning("TileSize property was not available.")

unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
unreal.log(f"Created or updated {blueprint_asset_path} with tile mesh {TILE_MESH_PATH}")
