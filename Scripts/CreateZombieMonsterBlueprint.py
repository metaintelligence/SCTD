import unreal


BLUEPRINT_PATH = "/Game/Blueprints"
BLUEPRINT_NAME = "BP_Zombie"
ZOMBIE_SKELETAL_MESH_PATH = "/Game/Fab/Zombie_Number_4_-_Animated/zom_4/SkeletalMeshes/zom_4.zom_4"
ZOMBIE_IDLE_ANIMATION_PATH = "/Game/Fab/Zombie_Number_4_-_Animated/zom_4/SkeletalMeshes/zom_4Idle_1.zom_4Idle_1"
ZOMBIE_WALKING_ANIMATION_PATH = "/Game/Fab/Zombie_Number_4_-_Animated/zom_4/SkeletalMeshes/zom_4Walking.zom_4Walking"
ZOMBIE_ATTACK_ANIMATION_PATHS = [
    "/Game/Fab/Zombie_Number_4_-_Animated/zom_4/SkeletalMeshes/zom_4Kick_1.zom_4Kick_1",
    "/Game/Fab/Zombie_Number_4_-_Animated/zom_4/SkeletalMeshes/zom_4Kick_2.zom_4Kick_2",
    "/Game/Fab/Zombie_Number_4_-_Animated/zom_4/SkeletalMeshes/zom_4Punch.zom_4Punch",
    "/Game/Fab/Zombie_Number_4_-_Animated/zom_4/SkeletalMeshes/zom_4Punch_2.zom_4Punch_2",
]


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
    factory.set_editor_property("ParentClass", unreal.BaseMonster)
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

zombie_cdo = unreal.get_default_object(generated_class)
safe_set_editor_property(zombie_cdo, "MaxHealth", 40.0)
safe_set_editor_property(zombie_cdo, "MoveSpeed", 10.0)
safe_set_editor_property(zombie_cdo, "AttackDamage", 2.0)
safe_set_editor_property(zombie_cdo, "AttackCooldownSeconds", 7.0)
safe_set_editor_property(zombie_cdo, "AttackRangeTileRadius", 1)
safe_set_editor_property(zombie_cdo, "AttackPreMotionMilliseconds", 800.0)
safe_set_editor_property(zombie_cdo, "AttackPostMotionMilliseconds", 800.0)
safe_set_editor_property(zombie_cdo, "MovementMass", 100.0)
safe_set_editor_property(zombie_cdo, "SecondsToReachMaxSpeed", 0.25)
safe_set_editor_property(zombie_cdo, "SideForceAmount", 0.4)
safe_set_editor_property(zombie_cdo, "SideForceDecisionIntervalSeconds", 0.5)
safe_set_editor_property(zombie_cdo, "CollisionRadius", 1000.0)
safe_set_editor_property(zombie_cdo, "LinearDamping", 1.0)
safe_set_editor_property(zombie_cdo, "CollisionRestitution", 0.85)
safe_set_editor_property(zombie_cdo, "bConstrainToHexGrid", True)
safe_set_editor_property(zombie_cdo, "bAlignToMovementDirection", True)
safe_set_editor_property(zombie_cdo, "RotationInterpSpeed", 10.0)
safe_set_editor_property(zombie_cdo, "VisualForwardYawOffsetDegrees", -90.0)
safe_set_editor_property(zombie_cdo, "bLogMonsterDebug", True)
safe_set_editor_property(zombie_cdo, "AIBehavior", None)
safe_set_editor_property(zombie_cdo, "AIBehaviorClass", unreal.BasicMonsterAIBehavior)
safe_set_editor_property(zombie_cdo, "IdleAnimation", unreal.EditorAssetLibrary.load_asset(ZOMBIE_IDLE_ANIMATION_PATH))
safe_set_editor_property(zombie_cdo, "WalkingAnimation", unreal.EditorAssetLibrary.load_asset(ZOMBIE_WALKING_ANIMATION_PATH))

attack_animations = []
for animation_path in ZOMBIE_ATTACK_ANIMATION_PATHS:
    animation = unreal.EditorAssetLibrary.load_asset(animation_path)
    if animation:
        attack_animations.append(animation)
    else:
        unreal.log_warning(f"Could not load zombie attack animation: {animation_path}")
safe_set_editor_property(zombie_cdo, "AttackAnimations", attack_animations)

try:
    monster_mesh_component = zombie_cdo.get_editor_property("MonsterMesh")
    try:
        monster_mesh_component.set_editor_property("RelativeScale3D", unreal.Vector(20.0, 20.0, 20.0))
    except Exception:
        monster_mesh_component.set_editor_property("relative_scale3d", unreal.Vector(20.0, 20.0, 20.0))

    zombie_mesh = unreal.EditorAssetLibrary.load_asset(ZOMBIE_SKELETAL_MESH_PATH)
    if zombie_mesh:
        try:
            monster_mesh_component.set_editor_property("SkeletalMeshAsset", zombie_mesh)
        except Exception:
            monster_mesh_component.set_editor_property("SkeletalMesh", zombie_mesh)
    else:
        unreal.log_warning(f"Could not load zombie skeletal mesh: {ZOMBIE_SKELETAL_MESH_PATH}")
except Exception as error:
    unreal.log_warning(f"Could not update zombie skeletal mesh: {error}")

unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
unreal.log(f"Created or updated {blueprint_asset_path}")
