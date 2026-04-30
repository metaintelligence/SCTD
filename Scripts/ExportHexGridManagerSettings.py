import json
from pathlib import Path

import unreal


BLUEPRINT_PATH = "/Game/Blueprints/BP_HexGridManager"
OUTPUT_PATH = Path(r"C:\Project\SCTD\Saved\BP_HexGridManager.settings.json")


def object_path(value):
    return value.get_path_name() if value else None


def enum_name(value):
    return str(value).split(".")[-1]


def vector_to_dict(value):
    return {
        "x": value.x,
        "y": value.y,
        "z": value.z,
    }


def rotator_to_dict(value):
    return {
        "pitch": value.pitch,
        "yaw": value.yaw,
        "roll": value.roll,
    }


blueprint = unreal.EditorAssetLibrary.load_asset(BLUEPRINT_PATH)
if not blueprint:
    raise RuntimeError(f"Blueprint not found: {BLUEPRINT_PATH}")

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)

generated_class = unreal.BlueprintEditorLibrary.generated_class(blueprint)
if not generated_class:
    raise RuntimeError(f"Generated class not found for: {BLUEPRINT_PATH}")

actor_cdo = unreal.get_default_object(generated_class)

tile_slots = []
for slot in actor_cdo.get_editor_property("TileSlots"):
    tile_slots.append(
        {
            "slot_index": slot.get_editor_property("SlotIndex"),
            "ring": slot.get_editor_property("Ring"),
            "ring_index": slot.get_editor_property("RingIndex"),
            "q": slot.get_editor_property("Q"),
            "r": slot.get_editor_property("R"),
            "local_location": vector_to_dict(slot.get_editor_property("LocalLocation")),
            "tile": {
                "type": enum_name(slot.get_editor_property("TileType")),
                "enemy_spawn": slot.get_editor_property("bEnemySpawn"),
                "mesh": object_path(slot.get_editor_property("Mesh")),
            },
        }
    )

settings = {
    "blueprint": BLUEPRINT_PATH,
    "grid_radius": actor_cdo.get_editor_property("GridRadius"),
    "expected_tile_slot_count": actor_cdo.get_expected_tile_slot_count(),
    "orientation": enum_name(actor_cdo.get_editor_property("Orientation")),
    "use_mesh_bounds_for_tile_spacing": actor_cdo.get_editor_property("bUseMeshBoundsForTileSpacing"),
    "tile_size": actor_cdo.get_editor_property("TileSize"),
    "tile_mesh": object_path(actor_cdo.get_editor_property("TileMesh")),
    "tile_rotation": rotator_to_dict(actor_cdo.get_editor_property("TileRotation")),
    "tile_scale": vector_to_dict(actor_cdo.get_editor_property("TileScale")),
    "center_grid_on_actor": actor_cdo.get_editor_property("bCenterGridOnActor"),
    "generate_on_construction": actor_cdo.get_editor_property("bGenerateOnConstruction"),
    "generate_on_begin_play": actor_cdo.get_editor_property("bGenerateOnBeginPlay"),
    "available_tile_meshes": [
        object_path(mesh) for mesh in actor_cdo.get_editor_property("AvailableTileMeshes")
    ],
    "tile_slots": tile_slots,
}

OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
OUTPUT_PATH.write_text(json.dumps(settings, indent=2), encoding="utf-8")
unreal.log(f"Exported {BLUEPRINT_PATH} settings to {OUTPUT_PATH}")
