import unreal


BLUEPRINT_PATH = "/Game/Blueprints"
BLUEPRINT_NAME = "BP_DefenseManager"


asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_asset_library = unreal.EditorAssetLibrary

editor_asset_library.make_directory(BLUEPRINT_PATH)

blueprint_asset_path = f"{BLUEPRINT_PATH}/{BLUEPRINT_NAME}"
blueprint = None
if editor_asset_library.does_asset_exist(blueprint_asset_path):
    blueprint = editor_asset_library.load_asset(blueprint_asset_path)

if not blueprint:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", unreal.DefenseManager)
    blueprint = asset_tools.create_asset(
        asset_name=BLUEPRINT_NAME,
        package_path=BLUEPRINT_PATH,
        asset_class=unreal.Blueprint,
        factory=factory,
    )

unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)

generated_class = unreal.BlueprintEditorLibrary.generated_class(blueprint)
if not generated_class:
    raise RuntimeError(f"Generated Blueprint class not found: {blueprint_asset_path}")

defense_manager_cdo = unreal.get_default_object(generated_class)
defense_manager_cdo.set_editor_property("DefenseDurationSeconds", 300.0)
defense_manager_cdo.set_editor_property("InitialScrap", 300)
defense_manager_cdo.set_editor_property("ScrapPerSecond", 10.0)
defense_manager_cdo.set_editor_property("MonsterSpawnIntervalSeconds", 5.0)
defense_manager_cdo.set_editor_property("bStartOnBeginPlay", True)

unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
unreal.log(f"Created or updated {blueprint_asset_path}")
