import unreal


def log_prop(obj, prop):
    try:
        value = obj.get_editor_property(prop)
        unreal.log(f"{obj.get_name()}.{prop} = {value.get_path_name() if value else None}")
    except Exception as exc:
        unreal.log_error(f"Failed reading {prop}: {exc}")


rarity = unreal.EditorAssetLibrary.load_asset("/Game/DataTables/DT_SCTD_ItemRarity.DT_SCTD_ItemRarity")
unreal.log(f"Rarity table asset = {rarity}")
if rarity:
    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(rarity)
    unreal.log(f"Rarity row names = {[str(name) for name in row_names]}")
    for row_name in row_names:
        row = unreal.DataTableFunctionLibrary.get_data_table_row_from_name(rarity, row_name)
        unreal.log(f"Rarity row {row_name}: {row}")

bp = unreal.EditorAssetLibrary.load_asset("/Game/Blueprints/BP_DefenseManager.BP_DefenseManager")
unreal.log(f"BP asset = {bp}")
if bp:
    cdo = unreal.get_default_object(bp.generated_class())
    for prop in [
        "base_part_definition_table",
        "weapon_part_definition_table",
        "control_part_definition_table",
        "part_option_table",
        "item_rarity_table",
    ]:
        log_prop(cdo, prop)
