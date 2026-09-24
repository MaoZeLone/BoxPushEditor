"""Create CA_Play and hook it to DA_Player.DefaultCameraAsset."""

import unreal

PLAYER_PATH = "/Game/Data/Characters/DA_Player"


def log(msg):
    unreal.log("[BoxPush Camera] " + msg)


def main():
    if not unreal.EditorAssetLibrary.does_asset_exist(PLAYER_PATH):
        raise RuntimeError("Missing DA_Player at " + PLAYER_PATH)
    player = unreal.EditorAssetLibrary.load_asset(PLAYER_PATH)
    camera = player.call_method("SeedOfficialPlayCamera", (True,))
    if not camera:
        raise RuntimeError("SeedOfficialPlayCamera returned None")
    player.set_editor_property("DefaultCameraAsset", camera)
    unreal.EditorAssetLibrary.save_loaded_asset(player)
    hooked = player.get_editor_property("DefaultCameraAsset")
    log("CA_Play = " + camera.get_path_name())
    log("DA_Player.DefaultCameraAsset = " + (hooked.get_path_name() if hooked else "None"))
    log("done")


if __name__ == "__main__":
    main()
