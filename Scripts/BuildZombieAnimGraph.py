import unreal


ANIM_BLUEPRINT_PATH = "/Game/Blueprints/ABP_Zombie"
IDLE_ANIMATION_PATH = "/Game/Fab/Zombie_Number_4_-_Animated/zom_4/SkeletalMeshes/zom_4Idle_1.zom_4Idle_1"
WALKING_ANIMATION_PATH = "/Game/Fab/Zombie_Number_4_-_Animated/zom_4/SkeletalMeshes/zom_4Walking.zom_4Walking"
ATTACK_ANIMATION_PATH = "/Game/Fab/Zombie_Number_4_-_Animated/zom_4/SkeletalMeshes/zom_4Kick_1.zom_4Kick_1"

anim_blueprint = unreal.EditorAssetLibrary.load_asset(ANIM_BLUEPRINT_PATH)
idle_animation = unreal.EditorAssetLibrary.load_asset(IDLE_ANIMATION_PATH)
walking_animation = unreal.EditorAssetLibrary.load_asset(WALKING_ANIMATION_PATH)
attack_animation = unreal.EditorAssetLibrary.load_asset(ATTACK_ANIMATION_PATH)

if not anim_blueprint:
    raise RuntimeError(f"Missing anim blueprint: {ANIM_BLUEPRINT_PATH}")
if not idle_animation:
    raise RuntimeError(f"Missing idle animation: {IDLE_ANIMATION_PATH}")
if not walking_animation:
    raise RuntimeError(f"Missing walking animation: {WALKING_ANIMATION_PATH}")
if not attack_animation:
    raise RuntimeError(f"Missing attack animation: {ATTACK_ANIMATION_PATH}")

unreal.log(f"SCTDAnimBlueprintTools attrs: {[attr for attr in dir(unreal.SCTDAnimBlueprintTools) if 'anim' in attr.lower() or 'graph' in attr.lower()]}")
result = unreal.SCTDAnimBlueprintTools.build_monster_state_blend_anim_graph(
    anim_blueprint,
    idle_animation,
    walking_animation,
    attack_animation,
)
unreal.log(f"build_monster_state_blend_anim_graph result={result}")

unreal.EditorAssetLibrary.save_loaded_asset(anim_blueprint)
