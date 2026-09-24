"""Add IA_Redo / Y to official player input without rebuilding other assets."""

import unreal

CHAR_DIR = "/Game/Data/Characters"
INPUT_DIR = "/Game/Data/Characters/Input"


def create_or_load_input_action(name, value_type):
    path = INPUT_DIR + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if asset:
            asset.set_editor_property("value_type", value_type)
            return asset
    factory = unreal.InputAction_Factory()
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, INPUT_DIR, unreal.InputAction, factory
    )
    if not asset:
        raise RuntimeError("Failed to create " + path)
    asset.set_editor_property("value_type", value_type)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def already_mapped(imc, action, key_name):
    mappings = imc.get_editor_property("mappings") or []
    for mapping in mappings:
        mapped_action = mapping.get_editor_property("action")
        key = mapping.get_editor_property("key")
        mapped_key = key.get_editor_property("key_name") if key else ""
        if mapped_action == action and str(mapped_key) == key_name:
            return True
    return False


def bind_key(imc, action, key_name):
    if already_mapped(imc, action, key_name):
        return
    key = unreal.Key()
    key.set_editor_property("key_name", key_name)
    imc.map_key(action, key)


def main():
    ia_redo = create_or_load_input_action("IA_Redo", unreal.InputActionValueType.BOOLEAN)
    imc = unreal.EditorAssetLibrary.load_asset(INPUT_DIR + "/IMC_Player")
    if not imc:
        raise RuntimeError("IMC_Player missing")
    bind_key(imc, ia_redo, "Y")
    unreal.EditorAssetLibrary.save_loaded_asset(imc)

    input_config = unreal.EditorAssetLibrary.load_asset(INPUT_DIR + "/DA_InputConfig_Player")
    if not input_config:
        raise RuntimeError("DA_InputConfig_Player missing")
    has_redo = False
    for row in input_config.get_editor_property("ability_input_actions") or []:
        tag = row.get_editor_property("input_tag")
        tag_name = tag.get_editor_property("tag_name") if tag else ""
        if str(tag_name) == "InputTag.Redo":
            has_redo = True
            break
    if not has_redo:
        input_config.add_ability_input_action(ia_redo, "InputTag.Redo")
    unreal.EditorAssetLibrary.save_loaded_asset(input_config)

    action_set = unreal.EditorAssetLibrary.load_asset(CHAR_DIR + "/DA_ActionSet_Player")
    if action_set and hasattr(action_set, "apply_official_defaults"):
        action_set.apply_official_defaults()
        unreal.EditorAssetLibrary.save_loaded_asset(action_set)

    relationships = unreal.EditorAssetLibrary.load_asset(
        CHAR_DIR + "/DA_AbilityTagRelationships_Player"
    )
    if relationships and hasattr(relationships, "apply_official_defaults"):
        relationships.apply_official_defaults()
        unreal.EditorAssetLibrary.save_loaded_asset(relationships)

    unreal.log("[BoxPush Seed] IA_Redo bound to Y")


if __name__ == "__main__":
    main()
