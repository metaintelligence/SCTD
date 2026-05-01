import unreal


names = [
    "AnimGraphNode_SequencePlayer",
    "AnimGraphNode_Root",
    "AnimGraphNode_BlendListByEnum",
    "AnimGraphNode_BlendListByBool",
    "AnimGraphNode_StateMachine",
    "AnimationGraphSchema",
    "GraphEditorSubsystem",
    "BlueprintEditorLibrary",
    "BlueprintEditorSubsystem",
    "KismetEditorUtilities",
]

for name in names:
    obj = getattr(unreal, name, None)
    unreal.log(f"{name}: {obj}")
    if obj:
        attrs = [attr for attr in dir(obj) if "node" in attr.lower() or "graph" in attr.lower() or "pin" in attr.lower()]
        unreal.log(f"{name} attrs: {attrs[:80]}")

subsystems = [
    getattr(unreal, "GraphEditorSubsystem", None),
    getattr(unreal, "BlueprintEditorSubsystem", None),
]
for subsystem_cls in subsystems:
    if subsystem_cls:
        try:
            subsystem = unreal.get_editor_subsystem(subsystem_cls)
            unreal.log(f"subsystem {subsystem_cls}: {subsystem}")
            unreal.log(f"subsystem attrs: {[attr for attr in dir(subsystem) if 'node' in attr.lower() or 'graph' in attr.lower() or 'pin' in attr.lower()][:120]}")
        except Exception as error:
            unreal.log_warning(f"subsystem failed {subsystem_cls}: {error}")
