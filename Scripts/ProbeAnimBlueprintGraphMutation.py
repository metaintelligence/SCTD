import unreal


ANIM_BLUEPRINT_PATH = "/Game/Blueprints/ABP_Zombie"
IDLE_ANIMATION_PATH = "/Game/Fab/Zombie_Number_4_-_Animated/zom_4/SkeletalMeshes/zom_4Idle_1.zom_4Idle_1"

anim_bp = unreal.EditorAssetLibrary.load_asset(ANIM_BLUEPRINT_PATH)
if not anim_bp:
    unreal.log_warning(f"Missing {ANIM_BLUEPRINT_PATH}")
    raise SystemExit

unreal.log(f"anim_bp={anim_bp} class={anim_bp.get_class()}")
unreal.log(f"anim_bp graph-ish attrs: {[attr for attr in dir(anim_bp) if 'graph' in attr.lower() or 'blueprint' in attr.lower() or 'node' in attr.lower()]}")

graphs = []
try:
    graphs = list(anim_bp.get_animation_graphs())
    unreal.log(f"get_animation_graphs count={len(graphs)}")
except Exception as error:
    unreal.log_warning(f"get_animation_graphs failed: {error}")

for property_name in ["UbergraphPages", "FunctionGraphs", "AnimGraphPages", "anim_graphs", "function_graphs", "ubergraph_pages"]:
    try:
        value = anim_bp.get_editor_property(property_name)
        unreal.log(f"property {property_name}: {value}")
        if isinstance(value, list) or hasattr(value, "__iter__"):
            for item in value:
                unreal.log(f"  item={item} class={item.get_class() if item else None}")
                if item:
                    attrs = [attr for attr in dir(item) if "node" in attr.lower() or "pin" in attr.lower() or "graph" in attr.lower()]
                    unreal.log(f"  graph attrs={attrs[:100]}")
    except Exception as error:
        unreal.log_warning(f"property {property_name} failed: {error}")

for graph in graphs:
    unreal.log(f"graph={graph} class={graph.get_class() if graph else None}")
    graph_attrs = [attr for attr in dir(graph) if "node" in attr.lower() or "pin" in attr.lower() or "graph" in attr.lower() or "schema" in attr.lower()]
    unreal.log(f"graph attrs={graph_attrs[:200]}")
    for prop in ["Nodes", "nodes", "Schema", "schema"]:
        try:
            value = graph.get_editor_property(prop)
            unreal.log(f"graph property {prop}: {value}")
            try:
                for item in value:
                    unreal.log(f"  graph item={item} class={item.get_class() if item else None}")
                    item_attrs = [attr for attr in dir(item) if "pin" in attr.lower() or "node" in attr.lower() or "link" in attr.lower() or "property" in attr.lower()]
                    unreal.log(f"  item attrs={item_attrs[:200]}")
                    for node_prop in ["Pins", "pins", "Node", "node", "NodePosX", "NodePosY"]:
                        try:
                            unreal.log(f"    item property {node_prop}: {item.get_editor_property(node_prop)}")
                        except Exception as node_error:
                            unreal.log_warning(f"    item property {node_prop} failed: {node_error}")
            except TypeError:
                pass
        except Exception as error:
            unreal.log_warning(f"graph property {prop} failed: {error}")
    for cls_name in ["AnimGraphNode_Root", "AnimGraphNode_SequencePlayer", "AnimGraphNode_BlendListByEnum", "AnimGraphNode_StateMachine"]:
        cls = getattr(unreal, cls_name, None)
        if not cls:
            unreal.log_warning(f"missing class {cls_name}")
            continue
        try:
            nodes = graph.get_graph_nodes_of_class(cls)
            unreal.log(f"graph.get_graph_nodes_of_class({cls_name}) count={len(nodes)} nodes={nodes}")
            for node in nodes:
                unreal.log(f"  existing {cls_name} node={node} attrs={[attr for attr in dir(node) if 'pin' in attr.lower() or 'node' in attr.lower() or 'property' in attr.lower()][:100]}")
                for node_prop in ["Node", "node", "Pins", "pins", "NodePosX", "NodePosY"]:
                    try:
                        unreal.log(f"    existing property {node_prop}: {node.get_editor_property(node_prop)}")
                    except Exception as node_error:
                        unreal.log_warning(f"    existing property {node_prop} failed: {node_error}")
        except Exception as error:
            unreal.log_warning(f"get_graph_nodes_of_class({cls_name}) failed: {error}")

node_cls = getattr(unreal, "AnimGraphNode_SequencePlayer", None)
if node_cls:
    unreal.log(f"node cls dir sample: {[attr for attr in dir(node_cls) if 'pin' in attr.lower() or 'node' in attr.lower() or 'property' in attr.lower() or 'allocate' in attr.lower() or 'reconstruct' in attr.lower()][:200]}")

if graphs and node_cls:
    graph = graphs[0]
    idle_animation = unreal.EditorAssetLibrary.load_asset(IDLE_ANIMATION_PATH)
    try:
        test_node = unreal.new_object(node_cls, graph)
        unreal.log(f"created test_node={test_node} class={test_node.get_class()}")
        unreal.log(f"test_node attrs={ [attr for attr in dir(test_node) if 'pin' in attr.lower() or 'node' in attr.lower() or 'property' in attr.lower() or 'allocate' in attr.lower() or 'reconstruct' in attr.lower() or 'sequence' in attr.lower()][:250] }")
        for prop in ["NodePosX", "NodePosY"]:
            try:
                test_node.set_editor_property(prop, 300 if prop.endswith("X") else 0)
                unreal.log(f"set {prop}")
            except Exception as error:
                unreal.log_warning(f"set {prop} failed: {error}")
        for prop in ["Node", "node"]:
            try:
                inner_node = test_node.get_editor_property(prop)
                unreal.log(f"inner {prop}: {inner_node}")
                unreal.log(f"inner attrs={ [attr for attr in dir(inner_node) if 'sequence' in attr.lower() or 'play' in attr.lower() or 'loop' in attr.lower() or 'property' in attr.lower()][:250] }")
                if idle_animation:
                    for inner_prop in ["Sequence", "sequence"]:
                        try:
                            inner_node.set_editor_property(inner_prop, idle_animation)
                            unreal.log(f"set inner {inner_prop}")
                        except Exception as error:
                            unreal.log_warning(f"set inner {inner_prop} failed: {error}")
            except Exception as error:
                unreal.log_warning(f"inner {prop} failed: {error}")
        for method_name in ["allocate_default_pins", "reconstruct_node", "post_placed_new_node", "create_new_guid"]:
            method = getattr(test_node, method_name, None)
            if method:
                try:
                    result = method()
                    unreal.log(f"{method_name} result={result}")
                except Exception as error:
                    unreal.log_warning(f"{method_name} failed: {error}")
        for graph_method_name in ["add_node", "modify", "notify_graph_changed"]:
            method = getattr(graph, graph_method_name, None)
            if method:
                try:
                    result = method(test_node) if graph_method_name == "add_node" else method()
                    unreal.log(f"graph {graph_method_name} result={result}")
                except Exception as error:
                    unreal.log_warning(f"graph {graph_method_name} failed: {error}")
        for prop in ["Nodes", "nodes"]:
            try:
                nodes = graph.get_editor_property(prop)
                unreal.log(f"post graph {prop} len={len(nodes)}")
            except Exception as error:
                unreal.log_warning(f"post graph {prop} failed: {error}")
        try:
            sequence_nodes = graph.get_graph_nodes_of_class(node_cls)
            unreal.log(f"post get_graph_nodes_of_class sequence count={len(sequence_nodes)} nodes={sequence_nodes}")
        except Exception as error:
            unreal.log_warning(f"post get_graph_nodes_of_class sequence failed: {error}")
    except Exception as error:
        unreal.log_warning(f"create test node failed: {error}")
