import unreal


MAP_PATH = "/Game/SCTD_initial_map"
BLUEPRINT_CLASS_PATH = "/Game/Blueprints/BP_HexGridManager.BP_HexGridManager_C"
ACTOR_LABEL = "BP_HexGridManager"


unreal.EditorLevelLibrary.load_level(MAP_PATH)

hex_grid_actors = [
    actor
    for actor in unreal.EditorLevelLibrary.get_all_level_actors()
    if isinstance(actor, unreal.HexGridManager)
]

if not hex_grid_actors:
    blueprint_class = unreal.load_object(None, BLUEPRINT_CLASS_PATH)
    if not blueprint_class:
        raise RuntimeError(f"Could not load Blueprint class: {BLUEPRINT_CLASS_PATH}")

    actor = unreal.EditorLevelLibrary.spawn_actor_from_class(
        blueprint_class,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(0.0, 0.0, 0.0),
    )
    actor.set_actor_label(ACTOR_LABEL)
    hex_grid_actors.append(actor)
    unreal.log(f"Spawned {ACTOR_LABEL} in {MAP_PATH}")

for actor in hex_grid_actors:
    try:
        actor.set_editor_property("is_spatially_loaded", False)
    except Exception:
        unreal.log_warning(f"{actor.get_actor_label()} does not expose is_spatially_loaded in this context.")

    try:
        actor.set_folder_path("Grid")
    except Exception:
        unreal.log_warning(f"{actor.get_actor_label()} folder path could not be set.")

repaired_count = 0
for actor in hex_grid_actors:
    actor.modify()
    actor.rebuild_tile_slots()
    actor.generate_grid()
    repaired_count += 1

if repaired_count == 0:
    unreal.log_warning(f"No HexGridManager actors found in {MAP_PATH}")
else:
    unreal.log(f"Repaired {repaired_count} HexGridManager actor(s) in {MAP_PATH}")

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
