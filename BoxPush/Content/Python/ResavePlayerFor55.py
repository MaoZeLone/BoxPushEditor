import unreal

CHAR_DIR = "/Game/Data/Characters"
BP_PATH = CHAR_DIR + "/BP_Player"
PLAYER_PATH = CHAR_DIR + "/DA_Player"
INPUT_CONFIG = CHAR_DIR + "/Input/DA_InputConfig_Player"
ACTION_SET = CHAR_DIR + "/DA_ActionSet_Player"
RELATIONSHIPS = CHAR_DIR + "/DA_AbilityTagRelationships_Player"
CAMERA = CHAR_DIR + "/Camera/CA_Play"


def discard(path):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        if not unreal.EditorAssetLibrary.delete_asset(path):
            raise RuntimeError("Failed to delete " + path)
        unreal.log("deleted " + path)


def load_required(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError("Missing " + path)
    return asset


def main():
    discard(BP_PATH)
    discard(PLAYER_PATH)

    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", unreal.BoxPlayerCharacter)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "BP_Player", CHAR_DIR, unreal.Blueprint, factory
    )
    if not blueprint:
        raise RuntimeError("Failed to create BP_Player")
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint)

    da_factory = unreal.DataAssetFactory()
    da_factory.set_editor_property("DataAssetClass", unreal.PlayerDef)
    player = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DA_Player", CHAR_DIR, unreal.PlayerDef, da_factory
    )
    if not player:
        raise RuntimeError("Failed to create DA_Player")

    player.apply_official_defaults()
    player.set_editor_property("pawn_class", blueprint.generated_class())
    player.set_editor_property("input_config", load_required(INPUT_CONFIG))
    player.set_editor_property("action_set", load_required(ACTION_SET))
    player.set_editor_property("tag_relationship_mapping", load_required(RELATIONSHIPS))
    camera = unreal.EditorAssetLibrary.load_asset(CAMERA)
    if camera:
        player.set_editor_property("default_camera_asset", camera)
    unreal.EditorAssetLibrary.save_loaded_asset(player)
    unreal.log("Resaved BP_Player and DA_Player for UE 5.5")


main()
