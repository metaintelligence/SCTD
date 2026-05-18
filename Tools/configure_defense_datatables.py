import unreal


TABLE_ASSIGNMENTS = {
    "base_part_definition_table": "/Game/DataTables/DT_SCTD_BodyParts.DT_SCTD_BodyParts",
    "weapon_part_definition_table": "/Game/DataTables/DT_SCTD_WeaponParts.DT_SCTD_WeaponParts",
    "control_part_definition_table": "/Game/DataTables/DT_SCTD_ControlParts.DT_SCTD_ControlParts",
    "part_option_table": "/Game/DataTables/DT_SCTD_PartOptions.DT_SCTD_PartOptions",
    "item_rarity_table": "/Game/DataTables/DT_SCTD_ItemRarity.DT_SCTD_ItemRarity",
}

DEFENSE_MANAGER_BLUEPRINTS = [
    "/Game/Blueprints/BP_DefenseManager.BP_DefenseManager",
]

MAPS_TO_CONFIGURE = [
    "/Game/SCTD_initial_map",
    "/Game/Maps/Lobby",
    "/Game/Maps/LAB",
    "/Game/Maps/Inventory",
]


def load_tables():
    tables = {}
    for property_name, asset_path in TABLE_ASSIGNMENTS.items():
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not asset:
            raise RuntimeError(f"Failed to load DataTable asset: {asset_path}")
        tables[property_name] = asset
    return tables


def assign_tables(target, tables):
    changed = False
    for property_name, table in tables.items():
        try:
            target.set_editor_property(property_name, table)
            changed = True
        except Exception as exc:
            unreal.log_warning(f"Could not set {property_name} on {target}: {exc}")
    return changed


tables = load_tables()

for blueprint_path in DEFENSE_MANAGER_BLUEPRINTS:
    blueprint = unreal.EditorAssetLibrary.load_asset(blueprint_path)
    if not blueprint:
        unreal.log_warning(f"DefenseManager blueprint not found: {blueprint_path}")
        continue

    generated_class = blueprint.generated_class()
    default_object = unreal.get_default_object(generated_class)
    if assign_tables(default_object, tables):
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        unreal.EditorAssetLibrary.save_asset(blueprint_path)
        unreal.log(f"Configured blueprint defaults: {blueprint_path}")

level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

for map_path in MAPS_TO_CONFIGURE:
    if not unreal.EditorAssetLibrary.does_asset_exist(map_path):
        continue

    if not level_subsystem.load_level(map_path):
        unreal.log_warning(f"Could not load map: {map_path}")
        continue

    changed = False
    for actor in actor_subsystem.get_all_level_actors():
        if actor.get_class().get_name().endswith("DefenseManager_C") or actor.get_class().get_name() == "DefenseManager":
            changed = assign_tables(actor, tables) or changed

    if changed:
        unreal.EditorLevelLibrary.save_current_level()
        unreal.log(f"Configured DefenseManager instances in map: {map_path}")
