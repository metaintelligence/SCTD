import unreal


MAP_PATH = "/Game/Maps/Lobby"


editor_asset_library = unreal.EditorAssetLibrary
editor_asset_library.make_directory("/Game/Maps")

if editor_asset_library.does_asset_exist(MAP_PATH):
    unreal.EditorLevelLibrary.load_level(MAP_PATH)
else:
    unreal.EditorLevelLibrary.new_level(MAP_PATH)

for actor in unreal.EditorLevelLibrary.get_all_level_actors():
    if actor.get_class().get_name() in ("LobbySceneRoot", "CameraActor"):
        unreal.EditorLevelLibrary.destroy_actor(actor)

lobby_root = unreal.EditorLevelLibrary.spawn_actor_from_class(
    unreal.LobbySceneRoot,
    unreal.Vector(0.0, 0.0, 0.0),
    unreal.Rotator(0.0, 0.0, 0.0),
)
lobby_root.set_actor_label("Lobby_UI_Root")

camera = unreal.EditorLevelLibrary.spawn_actor_from_class(
    unreal.CameraActor,
    unreal.Vector(0.0, -600.0, 260.0),
    unreal.Rotator(-18.0, 0.0, 0.0),
)
camera.set_actor_label("Lobby_UI_Camera")

unreal.EditorLevelLibrary.save_current_level()
unreal.log(f"Created or updated {MAP_PATH}")
