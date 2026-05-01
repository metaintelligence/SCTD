import unreal


ANIM_BLUEPRINT_PATH = "/Game/Blueprints/ABP_Zombie"

anim_bp = unreal.EditorAssetLibrary.load_asset(ANIM_BLUEPRINT_PATH)
if not anim_bp:
    raise RuntimeError(f"Missing {ANIM_BLUEPRINT_PATH}")

graph = list(anim_bp.get_animation_graphs())[0]
sequence_nodes = graph.get_graph_nodes_of_class(unreal.AnimGraphNode_SequencePlayer)
blend_nodes = graph.get_graph_nodes_of_class(unreal.AnimGraphNode_BlendListByEnum)

unreal.log(f"sequence node count={len(sequence_nodes)}")
for sequence_node in sequence_nodes:
    inner = sequence_node.get_editor_property("Node")
    unreal.log(f"sequence node={sequence_node.get_name()}")
    for prop in [
        "Sequence",
        "sequence",
        "bLoopAnimation",
        "loop_animation",
        "PlayRate",
        "play_rate",
        "StartPosition",
        "start_position",
    ]:
        try:
            unreal.log(f"  {prop}={inner.get_editor_property(prop)}")
        except Exception as error:
            unreal.log_warning(f"  {prop} failed: {error}")

unreal.log(f"blend node count={len(blend_nodes)}")
for blend_node in blend_nodes:
    unreal.log(f"blend node={blend_node.get_name()}")
    for pin in blend_node.get_editor_property("Pins") if False else []:
        unreal.log(f"  pin={pin}")
