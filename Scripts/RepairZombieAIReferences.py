import unreal


MAP_PATH = "/Game/SCTD_initial_map"


def safe_set_editor_property(obj, property_name, value):
    try:
        obj.set_editor_property(property_name, value)
    except Exception as error:
        unreal.log_warning(f"Could not set {property_name} on {obj.get_name()}: {error}")


unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
repaired_count = 0
for actor in actor_subsystem.get_all_level_actors():
    if not isinstance(actor, unreal.BaseMonster):
        continue

    safe_set_editor_property(actor, "AIBehavior", None)
    safe_set_editor_property(actor, "AIBehaviorClass", unreal.BasicMonsterAIBehavior)
    repaired_count += 1

unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
unreal.log(f"Repaired AI references on {repaired_count} monster actor(s).")
