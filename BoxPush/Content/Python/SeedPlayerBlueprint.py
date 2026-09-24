import unreal

CHAR_DIR = "/Game/Data/Characters"
BP_PATH = CHAR_DIR + "/BP_Player"
PLAYER_PATH = CHAR_DIR + "/DA_Player"


def main():
    if not unreal.EditorAssetLibrary.does_asset_exist(BP_PATH):
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("ParentClass", unreal.BoxPlayerCharacter)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "BP_Player", CHAR_DIR, unreal.Blueprint, factory
        )
        if not blueprint:
            raise RuntimeError("Failed to create BP_Player")
    else:
        blueprint = unreal.EditorAssetLibrary.load_asset(BP_PATH)

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)

    player = unreal.EditorAssetLibrary.load_asset(PLAYER_PATH)
    if not player:
        raise RuntimeError("DA_Player missing")
    player.set_editor_property("pawn_class", blueprint.generated_class())
    unreal.EditorAssetLibrary.save_loaded_asset(player)
    unreal.log("BP_Player is the pawn on DA_Player")


main()
