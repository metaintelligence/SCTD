import unreal


PROJECT_DIR = r"C:/Project/SCTD"
DESTINATION = "/Game/DataTables"


TABLES = [
    ("DT_SCTD_BodyParts", "Tools/DataTables/DT_SCTD_BodyParts.csv", "/Script/SCTD.SCTDTurretPartDefinitionRow"),
    ("DT_SCTD_WeaponParts", "Tools/DataTables/DT_SCTD_WeaponParts.csv", "/Script/SCTD.SCTDTurretPartDefinitionRow"),
    ("DT_SCTD_ControlParts", "Tools/DataTables/DT_SCTD_ControlParts.csv", "/Script/SCTD.SCTDTurretPartDefinitionRow"),
    ("DT_SCTD_PartOptions", "Tools/DataTables/DT_SCTD_PartOptions.csv", "/Script/SCTD.SCTDTurretPartOptionDefinitionRow"),
    ("DT_SCTD_ItemRarity", "Tools/DataTables/DT_SCTD_ItemRarity.csv", "/Script/SCTD.SCTDItemRarityDefinitionRow"),
]


def import_table(asset_name, relative_csv_path, row_struct_path):
    row_struct = unreal.load_object(None, row_struct_path)
    if not row_struct:
        raise RuntimeError(f"Failed to load row struct: {row_struct_path}")

    factory = unreal.CSVImportFactory()
    factory.automated_import_settings.import_type = unreal.CSVImportType.ECSV_DATA_TABLE
    factory.automated_import_settings.import_row_struct = row_struct

    task = unreal.AssetImportTask()
    task.filename = f"{PROJECT_DIR}/{relative_csv_path}"
    task.destination_path = DESTINATION
    task.destination_name = asset_name
    task.replace_existing = True
    task.automated = True
    task.save = True
    task.factory = factory

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported_path = f"{DESTINATION}/{asset_name}"
    asset = unreal.EditorAssetLibrary.load_asset(imported_path)
    if not asset:
        raise RuntimeError(f"Failed to import DataTable: {imported_path}")
    unreal.EditorAssetLibrary.save_asset(imported_path)
    unreal.log(f"Imported {imported_path}")


unreal.EditorAssetLibrary.make_directory(DESTINATION)
for table in TABLES:
    import_table(*table)
