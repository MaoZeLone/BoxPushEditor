import unreal

MAP_DIR = "/Game/Maps"
MAP_NAME = "M_Play"
MAP_PATH = MAP_DIR + "/" + MAP_NAME


def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def load_or_create_map():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        if not les.load_level(MAP_PATH):
            raise RuntimeError("Failed to load " + MAP_PATH)
        unreal.log("[BoxPush Map] loaded " + MAP_PATH)
        return
    ensure_dir(MAP_DIR)
    if not les.new_level(MAP_PATH, False):
        raise RuntimeError("Failed to create " + MAP_PATH)
    unreal.log("[BoxPush Map] created " + MAP_PATH)


def actors_of_class(cls):
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return [a for a in subsystem.get_all_level_actors() if isinstance(a, cls)]


def strip_player_starts():
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    removed = 0
    for actor in list(subsystem.get_all_level_actors()):
        if isinstance(actor, unreal.PlayerStart):
            actor.destroy_actor()
            removed += 1
    if removed:
        unreal.log("[BoxPush Map] removed PlayerStart x%d" % removed)


def ensure_light(cls, location, rotation=None):
    if actors_of_class(cls):
        return
    subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    rotation = rotation or unreal.Rotator(0, 0, 0)
    actor = subsystem.spawn_actor_from_class(cls, location, rotation)
    if not actor:
        raise RuntimeError("Failed to spawn " + cls.__name__)
    unreal.log("[BoxPush Map] spawned " + cls.__name__)


def main():
    load_or_create_map()
    strip_player_starts()
    ensure_light(unreal.DirectionalLight, unreal.Vector(0, 0, 400), unreal.Rotator(-50, -40, 0))
    ensure_light(unreal.SkyLight, unreal.Vector(0, 0, 300))
    if hasattr(unreal, "SkyAtmosphere") and not actors_of_class(unreal.SkyAtmosphere):
        ensure_light(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    unreal.EditorLevelLibrary.save_current_level()
    unreal.EditorAssetLibrary.save_directory(MAP_DIR, True, True)
    if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        raise RuntimeError("M_Play missing after save")
    unreal.log("[BoxPush Map] done " + MAP_PATH)


if __name__ == "__main__":
    main()
