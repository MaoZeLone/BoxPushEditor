import unreal

MAP_DIR = "/Game/Maps"
PLAY_PATH = MAP_DIR + "/M_Play"
MENU_PATH = MAP_DIR + "/M_Menu"
MENU_GM = "/Script/BoxPush.BoxMenuGameMode"


def ensure_menu_asset():
    if unreal.EditorAssetLibrary.does_asset_exist(MENU_PATH):
        unreal.log("[BoxPush Menu] asset exists")
        return
    if not unreal.EditorAssetLibrary.does_asset_exist(PLAY_PATH):
        raise RuntimeError("M_Play missing; cannot duplicate menu map")
    if not unreal.EditorAssetLibrary.duplicate_asset(PLAY_PATH, MENU_PATH):
        raise RuntimeError("Failed to duplicate M_Play -> M_Menu")
    unreal.log("[BoxPush Menu] duplicated from M_Play")


def set_menu_game_mode():
    world = unreal.EditorAssetLibrary.load_asset(MENU_PATH)
    if not world:
        raise RuntimeError("Failed to load asset " + MENU_PATH)
    settings = world.get_world_settings()
    cls = None
    if hasattr(unreal, "BoxMenuGameMode"):
        cls = unreal.BoxMenuGameMode.static_class()
    if not cls:
        cls = unreal.load_class(None, MENU_GM)
    if not cls:
        unreal.log_warning("[BoxPush Menu] BoxMenuGameMode not compiled yet")
        return
    settings.set_editor_property("default_game_mode", cls)
    unreal.log("[BoxPush Menu] DefaultGameMode = BoxMenuGameMode")


def main():
    ensure_menu_asset()
    set_menu_game_mode()
    if not unreal.EditorAssetLibrary.save_asset(MENU_PATH, only_if_is_dirty=False):
        raise RuntimeError("Failed to save " + MENU_PATH)
    if not unreal.EditorAssetLibrary.does_asset_exist(MENU_PATH):
        raise RuntimeError("M_Menu missing after save")
    unreal.log("[BoxPush Menu] done " + MENU_PATH)


if __name__ == "__main__":
    main()
