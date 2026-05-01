import unreal


BLUEPRINT_PATH = "/Game/Blueprints"
BLUEPRINT_NAME = "BP_FlyingPlayerPawn"


def safe_set_editor_property(obj, property_name, value):
    try:
        obj.set_editor_property(property_name, value)
    except Exception as error:
        unreal.log_warning(f"Could not set {property_name}: {error}")


asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_asset_library = unreal.EditorAssetLibrary

editor_asset_library.make_directory(BLUEPRINT_PATH)

blueprint_asset_path = f"{BLUEPRINT_PATH}/{BLUEPRINT_NAME}"
blueprint = None
if editor_asset_library.does_asset_exist(blueprint_asset_path):
    blueprint = editor_asset_library.load_asset(blueprint_asset_path)

if not blueprint:
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", unreal.FlyingPlayerPawn)
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

pawn_cdo = unreal.get_default_object(generated_class)
safe_set_editor_property(pawn_cdo, "FlightAltitude", 1000.0)
safe_set_editor_property(pawn_cdo, "RotationInterpSpeed", 10.0)
safe_set_editor_property(pawn_cdo, "bAlignToMovementDirection", True)
safe_set_editor_property(pawn_cdo, "bConstrainToHexGrid", True)
safe_set_editor_property(pawn_cdo, "CollisionRadius", 50.0)
safe_set_editor_property(pawn_cdo, "LinearDamping", 1.0)
safe_set_editor_property(pawn_cdo, "CollisionRestitution", 0.85)
safe_set_editor_property(pawn_cdo, "BoostRecoveryRate", 10.0)
safe_set_editor_property(pawn_cdo, "BoostConsumeRate", 20.0)

try:
    status_component = pawn_cdo.get_editor_property("StatusComponent")
    safe_set_editor_property(status_component, "MaxHealth", 100.0)
    safe_set_editor_property(status_component, "bUsesBoost", True)
    safe_set_editor_property(status_component, "MaxBoost", 100.0)
except Exception as error:
    unreal.log_warning(f"Could not update StatusComponent defaults: {error}")

try:
    status_display_component = pawn_cdo.get_editor_property("StatusDisplayComponent")
    safe_set_editor_property(status_display_component, "RelativeOffset", unreal.Vector(0.0, 0.0, 40.0))
    safe_set_editor_property(status_display_component, "VisibleSecondsAfterChange", 2.0)
    safe_set_editor_property(status_display_component, "bShowBoost", True)
    safe_set_editor_property(status_display_component, "GaugeWidth", 120.0)
    safe_set_editor_property(status_display_component, "GaugeHeight", 12.0)
    safe_set_editor_property(status_display_component, "GaugeOffsetPx", 2.0)
    safe_set_editor_property(status_display_component, "HealthFillHexColor", "#FF0000")
    safe_set_editor_property(status_display_component, "BoostFillHexColor", "#00FF00")
except Exception as error:
    unreal.log_warning(f"Could not update StatusDisplayComponent defaults: {error}")

try:
    player_model = pawn_cdo.get_editor_property("PlayerModel")
    safe_set_editor_property(player_model, "MoveSpeed", 30.0)
    safe_set_editor_property(player_model, "BoostSpeedMultiplier", 2.0)
    safe_set_editor_property(player_model, "MovementMass", 100.0)
    safe_set_editor_property(player_model, "SecondsToReachMaxSpeed", 0.25)
    safe_set_editor_property(player_model, "BuildSpeed", 10.0)
    safe_set_editor_property(player_model, "AttackSpeed", 1.0)
    safe_set_editor_property(player_model, "AttackRange", 5.0)
    safe_set_editor_property(player_model, "AttackDamage", 5.0)
except Exception as error:
    unreal.log_warning(f"Could not update PlayerModel defaults: {error}")

unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
unreal.log(f"Created or updated {blueprint_asset_path}")
