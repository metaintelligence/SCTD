import unreal


BLUEPRINT_PATH = "/Game/Blueprints"
BLUEPRINT_NAME = "BP_TopDownEdgeScrollCamera"


asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_asset_library = unreal.EditorAssetLibrary

editor_asset_library.make_directory(BLUEPRINT_PATH)

blueprint_asset_path = f"{BLUEPRINT_PATH}/{BLUEPRINT_NAME}"
blueprint = None
if editor_asset_library.does_asset_exist(blueprint_asset_path):
    blueprint = editor_asset_library.load_asset(blueprint_asset_path)

if not blueprint:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", unreal.TopDownEdgeScrollCamera)
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

camera_cdo = unreal.get_default_object(generated_class)
camera_cdo.set_editor_property("bEnableEdgeScroll", True)
camera_cdo.set_editor_property("EdgeScrollMarginViewportRatio", 0.01)
camera_cdo.set_editor_property("EdgeScrollSpeed", 20000.0)
camera_cdo.set_editor_property("bUseSmoothEdgeStrength", True)
camera_cdo.set_editor_property("bSetAsPlayerViewTargetOnBeginPlay", True)
camera_cdo.set_editor_property("bShowMouseCursor", True)
camera_cdo.set_editor_property("bFollowPlayer", True)
camera_cdo.set_editor_property("bUseInitialOffsetFromPlayer", False)
camera_cdo.set_editor_property("PlayerFollowOffset", unreal.Vector(-1000.0, 0.0, 1732.0))
camera_cdo.set_editor_property("FollowSpringStrength", 36.0)
camera_cdo.set_editor_property("FollowDamping", 10.0)
camera_cdo.set_editor_property("MaxFollowSpeed", 30000.0)
camera_cdo.set_editor_property("MaxFollowAcceleration", 40000.0)
camera_cdo.set_editor_property("bEnableMouseWheelZoom", True)
camera_cdo.set_editor_property("MinZoomDistance", 5000.0)
camera_cdo.set_editor_property("MaxZoomDistance", 30000.0)
camera_cdo.set_editor_property("InitialZoomRatio", 0.5)
camera_cdo.set_editor_property("ZoomSpringStrength", 48.0)
camera_cdo.set_editor_property("ZoomDamping", 12.0)
camera_cdo.set_editor_property("TopEdgeWorldDirection", unreal.Vector(1.0, 0.0, 0.0))
camera_cdo.set_editor_property("RightEdgeWorldDirection", unreal.Vector(0.0, 1.0, 0.0))
camera_cdo.set_editor_property("bClampToBounds", False)
camera_cdo.set_editor_property("DamageShakeDurationSeconds", 0.1)
camera_cdo.set_editor_property("DamageShakeAmplitude", 200.0)
camera_cdo.set_editor_property("LowHealthOverlayStartRatio", 0.3)
camera_cdo.set_editor_property("DamageOverlayColorHex", "#FF0000")
camera_cdo.set_editor_property("DamageOverlayFlashDurationSeconds", 0.2)

unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
unreal.log(f"Created or updated {blueprint_asset_path}")
