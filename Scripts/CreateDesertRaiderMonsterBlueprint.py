import unreal


BLUEPRINT_PATH = "/Game/Blueprints"
BLUEPRINT_NAME = "BP_DesertRaider"
ANIM_BLUEPRINT_NAME = "ABP_DesertRaider"
RAIDER_ASSET_DIR = "/Game/Characters/Raider/RuinsRaider"
RAIDER_SKELETAL_MESH_PATH = f"{RAIDER_ASSET_DIR}/ruins_raider.ruins_raider"
RAIDER_DEATH_FADE_MATERIAL_NAME = "M_DesertRaiderDeathFade"


def safe_set_editor_property(obj, property_name, value):
    try:
        obj.set_editor_property(property_name, value)
        return True
    except Exception as error:
        unreal.log_warning(f"Could not set {property_name}: {error}")
        return False


def get_asset_skeleton(skeletal_mesh):
    if not skeletal_mesh:
        return None

    for property_name in ("Skeleton", "skeleton"):
        try:
            skeleton = skeletal_mesh.get_editor_property(property_name)
            if skeleton:
                return skeleton
        except Exception:
            pass
    return None


def list_raider_animations():
    animation_assets = []
    for asset_path in unreal.EditorAssetLibrary.list_assets(RAIDER_ASSET_DIR, recursive=True, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if isinstance(asset, unreal.AnimSequence):
            animation_assets.append(asset)

    animation_assets.sort(key=lambda asset: asset.get_name().lower())
    return animation_assets


def pick_animation(animations, keywords, fallback=None):
    for animation in animations:
        name = animation.get_name().lower()
        if any(keyword in name for keyword in keywords):
            return animation
    return fallback or (animations[0] if animations else None)


def get_or_create_death_fade_material():
    material_asset_path = f"{BLUEPRINT_PATH}/{RAIDER_DEATH_FADE_MATERIAL_NAME}"
    material = unreal.EditorAssetLibrary.load_asset(material_asset_path)
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name=RAIDER_DEATH_FADE_MATERIAL_NAME,
            package_path=BLUEPRINT_PATH,
            asset_class=unreal.Material,
            factory=unreal.MaterialFactoryNew(),
        )

    if not material:
        return None

    safe_set_editor_property(material, "blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    safe_set_editor_property(material, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    safe_set_editor_property(material, "two_sided", True)

    try:
        unreal.MaterialEditingLibrary.delete_all_material_expressions(material)

        color_expression = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionVectorParameter,
            -400,
            -120,
        )
        safe_set_editor_property(color_expression, "parameter_name", "Color")
        safe_set_editor_property(color_expression, "default_value", unreal.LinearColor(0.58, 0.43, 0.31, 1.0))

        opacity_expression = unreal.MaterialEditingLibrary.create_material_expression(
            material,
            unreal.MaterialExpressionScalarParameter,
            -400,
            120,
        )
        safe_set_editor_property(opacity_expression, "parameter_name", "Opacity")
        safe_set_editor_property(opacity_expression, "default_value", 1.0)

        unreal.MaterialEditingLibrary.connect_material_property(color_expression, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        unreal.MaterialEditingLibrary.connect_material_property(opacity_expression, "", unreal.MaterialProperty.MP_OPACITY)
        unreal.MaterialEditingLibrary.layout_material_expressions(material)
        unreal.MaterialEditingLibrary.recompile_material(material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
    except Exception as error:
        unreal.log_warning(f"Could not rebuild desert raider death fade material graph: {error}")

    return material


asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
editor_asset_library = unreal.EditorAssetLibrary
editor_asset_library.make_directory(BLUEPRINT_PATH)

raider_mesh = editor_asset_library.load_asset(RAIDER_SKELETAL_MESH_PATH)
if not raider_mesh:
    raise RuntimeError(f"Missing raider skeletal mesh: {RAIDER_SKELETAL_MESH_PATH}")

raider_skeleton = get_asset_skeleton(raider_mesh)
if not raider_skeleton:
    raise RuntimeError(f"Missing skeleton on raider skeletal mesh: {RAIDER_SKELETAL_MESH_PATH}")

raider_animations = list_raider_animations()
if not raider_animations:
    raise RuntimeError(f"No AnimSequence assets found under {RAIDER_ASSET_DIR}")

fallback_animation = raider_animations[0]
idle_animation = pick_animation(raider_animations, ["idle", "stand", "breath"], fallback_animation)
walking_animation = pick_animation(raider_animations, ["walk", "run", "move", "locomotion"], fallback_animation)
attack_animation = pick_animation(raider_animations, ["attack", "punch", "slash", "hit", "melee", "shoot"], fallback_animation)
death_animation = pick_animation(raider_animations, ["death", "die", "dead", "fall"], fallback_animation)
attack_animations = [
    animation
    for animation in raider_animations
    if any(keyword in animation.get_name().lower() for keyword in ["attack", "punch", "slash", "hit", "melee", "shoot"])
]
if not attack_animations:
    attack_animations = [attack_animation]

unreal.log(
    "DesertRaider animation mapping: "
    f"idle={idle_animation.get_name()} walking={walking_animation.get_name()} "
    f"attack={attack_animation.get_name()} death={death_animation.get_name()} "
    f"all={[animation.get_name() for animation in raider_animations]}"
)

anim_blueprint_asset_path = f"{BLUEPRINT_PATH}/{ANIM_BLUEPRINT_NAME}"
anim_blueprint = editor_asset_library.load_asset(anim_blueprint_asset_path)
if not anim_blueprint:
    anim_factory = unreal.AnimBlueprintFactory()
    anim_factory.set_editor_property("ParentClass", unreal.MonsterAnimInstance)
    anim_factory.set_editor_property("TargetSkeleton", raider_skeleton)
    anim_factory.set_editor_property("PreviewSkeletalMesh", raider_mesh)
    anim_blueprint = asset_tools.create_asset(
        asset_name=ANIM_BLUEPRINT_NAME,
        package_path=BLUEPRINT_PATH,
        asset_class=unreal.AnimBlueprint,
        factory=anim_factory,
    )

if not anim_blueprint:
    raise RuntimeError(f"Could not create or load {anim_blueprint_asset_path}")

blueprint_asset_path = f"{BLUEPRINT_PATH}/{BLUEPRINT_NAME}"
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

if not blueprint:
    raise RuntimeError(f"Could not create or load {blueprint_asset_path}")

unreal.BlueprintEditorLibrary.compile_blueprint(anim_blueprint)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)

anim_generated_class = unreal.BlueprintEditorLibrary.generated_class(anim_blueprint)
generated_class = unreal.BlueprintEditorLibrary.generated_class(blueprint)
if not generated_class:
    raise RuntimeError(f"Generated Blueprint class not found: {blueprint_asset_path}")

raider_cdo = unreal.get_default_object(generated_class)
safe_set_editor_property(raider_cdo, "MoveSpeed", 14.0)
safe_set_editor_property(raider_cdo, "MinAttackDamage", 1.0)
safe_set_editor_property(raider_cdo, "MaxAttackDamage", 3.0)
safe_set_editor_property(raider_cdo, "AttackCooldownSeconds", 4.0)
safe_set_editor_property(raider_cdo, "AttackRangeTileRadius", 2)
safe_set_editor_property(raider_cdo, "AttackPreMotionMilliseconds", 450.0)
safe_set_editor_property(raider_cdo, "AttackPostMotionMilliseconds", 650.0)
safe_set_editor_property(raider_cdo, "ScrapReward", 25)
safe_set_editor_property(raider_cdo, "ExpReward", 120)
safe_set_editor_property(raider_cdo, "ItemDropRate", 0.12)
safe_set_editor_property(raider_cdo, "MovementMass", 80.0)
safe_set_editor_property(raider_cdo, "SecondsToReachMaxSpeed", 0.20)
safe_set_editor_property(raider_cdo, "SideForceAmount", 0.55)
safe_set_editor_property(raider_cdo, "SideForceDecisionIntervalSeconds", 0.35)
safe_set_editor_property(raider_cdo, "CollisionRadius", 800.0)
safe_set_editor_property(raider_cdo, "LinearDamping", 1.0)
safe_set_editor_property(raider_cdo, "CollisionRestitution", 0.85)
safe_set_editor_property(raider_cdo, "bConstrainToHexGrid", True)
safe_set_editor_property(raider_cdo, "bAlignToMovementDirection", True)
safe_set_editor_property(raider_cdo, "RotationInterpSpeed", 12.0)
safe_set_editor_property(raider_cdo, "VisualForwardYawOffsetDegrees", -90.0)
safe_set_editor_property(raider_cdo, "AnimationTransitionBlendSeconds", 0.12)
safe_set_editor_property(raider_cdo, "bLogMonsterDebug", True)
safe_set_editor_property(raider_cdo, "AIBehavior", None)
safe_set_editor_property(raider_cdo, "AIBehaviorClass", unreal.BasicMonsterAIBehavior)

safe_set_editor_property(raider_cdo, "IdleAnimation", idle_animation)
safe_set_editor_property(raider_cdo, "WalkingAnimation", walking_animation)
safe_set_editor_property(raider_cdo, "AttackAnimations", attack_animations)
safe_set_editor_property(raider_cdo, "DeathAnimation", death_animation)
safe_set_editor_property(raider_cdo, "DeathAnimationDurationSeconds", 1.0)
safe_set_editor_property(raider_cdo, "DeathFadeDurationSeconds", 0.3)
if anim_generated_class:
    safe_set_editor_property(raider_cdo, "MonsterAnimInstanceClass", anim_generated_class)
else:
    safe_set_editor_property(raider_cdo, "MonsterAnimInstanceClass", unreal.MonsterAnimInstance)

death_fade_material = get_or_create_death_fade_material()
if death_fade_material:
    safe_set_editor_property(raider_cdo, "DeathFadeMaterial", death_fade_material)

try:
    status_component = raider_cdo.get_editor_property("StatusComponent")
    safe_set_editor_property(status_component, "MaxHealth", 28.0)
    safe_set_editor_property(status_component, "bUsesBoost", False)
except Exception as error:
    unreal.log_warning(f"Could not update raider StatusComponent defaults: {error}")

try:
    status_display_component = raider_cdo.get_editor_property("StatusDisplayComponent")
    safe_set_editor_property(status_display_component, "RelativeOffset", unreal.Vector(0.0, 0.0, 70.0))
    safe_set_editor_property(status_display_component, "VisibleSecondsAfterChange", 2.0)
    safe_set_editor_property(status_display_component, "bShowBoost", False)
    safe_set_editor_property(status_display_component, "GaugeWidth", 110.0)
    safe_set_editor_property(status_display_component, "GaugeHeight", 10.0)
    safe_set_editor_property(status_display_component, "GaugeOffsetPx", 2.0)
    safe_set_editor_property(status_display_component, "HealthFillHexColor", "#E05230")
    safe_set_editor_property(status_display_component, "BoostFillHexColor", "#00FF00")
except Exception as error:
    unreal.log_warning(f"Could not update raider StatusDisplayComponent defaults: {error}")

try:
    monster_mesh_component = raider_cdo.get_editor_property("MonsterMesh")
    safe_set_editor_property(monster_mesh_component, "RelativeScale3D", unreal.Vector(10.0, 10.0, 10.0))
    safe_set_editor_property(monster_mesh_component, "RelativeLocation", unreal.Vector(0.0, 0.0, 0.0))
    safe_set_editor_property(monster_mesh_component, "pause_anims", False)
    safe_set_editor_property(monster_mesh_component, "no_skeleton_update", False)
    safe_set_editor_property(monster_mesh_component, "enable_update_rate_optimizations", False)
    safe_set_editor_property(monster_mesh_component, "global_anim_rate_scale", 1.0)
    safe_set_editor_property(
        monster_mesh_component,
        "visibility_based_anim_tick_option",
        unreal.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES,
    )
    try:
        monster_mesh_component.set_editor_property("SkeletalMeshAsset", raider_mesh)
    except Exception:
        monster_mesh_component.set_editor_property("SkeletalMesh", raider_mesh)
    if anim_generated_class:
        safe_set_editor_property(monster_mesh_component, "AnimClass", anim_generated_class)
except Exception as error:
    unreal.log_warning(f"Could not update raider skeletal mesh component: {error}")

anim_tools = getattr(unreal, "SCTDAnimBlueprintTools", None)
if anim_tools and hasattr(anim_tools, "build_monster_state_blend_anim_graph"):
    result = anim_tools.build_monster_state_blend_anim_graph(
        anim_blueprint,
        idle_animation,
        walking_animation,
        attack_animation,
        death_animation,
    )
    unreal.log(f"Updated {ANIM_BLUEPRINT_NAME} animation graph: {result}")
else:
    unreal.log_warning("SCTDAnimBlueprintTools is unavailable; ABP graph was not generated.")

unreal.BlueprintEditorLibrary.compile_blueprint(anim_blueprint)
unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)

raider_cdo.set_actor_scale3d(unreal.Vector(10.0, 10.0, 10.0))
unreal.log(f"DesertRaider saved ActorScale3D: {raider_cdo.get_actor_scale3d()}")

unreal.EditorAssetLibrary.save_loaded_asset(anim_blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(blueprint)
unreal.EditorAssetLibrary.save_directory(BLUEPRINT_PATH, only_if_is_dirty=True, recursive=True)

unreal.log(f"Created or updated {blueprint_asset_path}")
