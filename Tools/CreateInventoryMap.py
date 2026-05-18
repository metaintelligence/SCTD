import unreal


MAP_PATH = "/Game/Maps/Inventory"


def main():
    unreal.EditorLevelLibrary.new_level(MAP_PATH)
    scene_root_class = unreal.load_class(None, "/Script/SCTD.InventorySceneRoot")
    unreal.EditorLevelLibrary.spawn_actor_from_class(scene_root_class, unreal.Vector(0.0, 0.0, 0.0))
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)


main()
