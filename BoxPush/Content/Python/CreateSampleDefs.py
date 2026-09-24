import unreal

CHAR_DIR = "/Game/Data/Characters"
INPUT_DIR = "/Game/Data/Characters/Input"
INTERACT_DIR = "/Game/Data/Interactables"

OFFICIAL_INTERACTABLES = (
    "Box_Normal",
    "Box_Slide",
    "Box_Return",
    "Target",
    "Pedal",
)


def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def create_or_load_da(asset_name, directory, da_class):
    path = directory + "/" + asset_name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("DataAssetClass", da_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, directory, da_class, factory
    )
    if not asset:
        raise RuntimeError("Failed to create " + path)
    return asset


def create_or_load_input_action(name, value_type):
    path = INPUT_DIR + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    factory = unreal.InputAction_Factory()
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, INPUT_DIR, unreal.InputAction, factory
    )
    if not asset:
        raise RuntimeError("Failed to create " + path)
    asset.set_editor_property("value_type", value_type)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def create_or_load_imc():
    path = INPUT_DIR + "/IMC_Player"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    factory = unreal.InputMappingContext_Factory()
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "IMC_Player", INPUT_DIR, unreal.InputMappingContext, factory
    )
    if not asset:
        raise RuntimeError("Failed to create IMC_Player")
    return asset


def bind_key(imc, action, key_name):
    key = unreal.Key()
    key.set_editor_property("key_name", key_name)
    imc.map_key(action, key)


def fill_tagged_binds(input_config, pairs, reset_fn_name, add_fn_name, property_name):
    reset_fn = getattr(input_config, reset_fn_name, None)
    add_fn = getattr(input_config, add_fn_name, None)
    if reset_fn and add_fn:
        reset_fn()
        for action, tag_name in pairs:
            add_fn(action, tag_name)
        return
    binds = []
    for action, tag_name in pairs:
        entry = unreal.BoxInputAction()
        text = '(InputAction="%s",InputTag=(TagName="%s"))' % (action.get_path_name(), tag_name)
        if not entry.import_text(text):
            raise RuntimeError("Failed to import BoxInputAction " + text)
        binds.append(entry)
    input_config.set_editor_property(property_name, binds)


def fill_native_binds(input_config, pairs):
    fill_tagged_binds(
        input_config, pairs, "reset_native_input_actions", "add_native_input_action", "native_input_actions"
    )


def fill_ability_binds(input_config, pairs):
    fill_tagged_binds(
        input_config, pairs, "reset_ability_input_actions", "add_ability_input_action", "ability_input_actions"
    )



def create_interactables():
    ensure_dir(INTERACT_DIR)
    created = []
    for definition_id in OFFICIAL_INTERACTABLES:
        name = "DA_" + definition_id
        asset = create_or_load_da(name, INTERACT_DIR, unreal.InteractableDef)
        asset.apply_official_defaults(definition_id)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        created.append(name)
    return created


def create_player():
    ensure_dir(CHAR_DIR)
    ensure_dir(INPUT_DIR)

    ia_move = create_or_load_input_action("IA_Move", unreal.InputActionValueType.AXIS2D)
    ia_undo = create_or_load_input_action("IA_Undo", unreal.InputActionValueType.BOOLEAN)
    ia_redo = create_or_load_input_action("IA_Redo", unreal.InputActionValueType.BOOLEAN)
    ia_restart = create_or_load_input_action("IA_Restart", unreal.InputActionValueType.BOOLEAN)
    ia_pause = create_or_load_input_action("IA_Pause", unreal.InputActionValueType.BOOLEAN)

    imc = create_or_load_imc()
    imc.unmap_all()
    bind_key(imc, ia_move, "W")
    bind_key(imc, ia_move, "A")
    bind_key(imc, ia_move, "S")
    bind_key(imc, ia_move, "D")
    bind_key(imc, ia_undo, "Z")
    bind_key(imc, ia_redo, "Y")
    bind_key(imc, ia_restart, "R")
    bind_key(imc, ia_pause, "Escape")
    unreal.EditorAssetLibrary.save_loaded_asset(imc)

    input_config = create_or_load_da("DA_InputConfig_Player", INPUT_DIR, unreal.BoxInputConfig)
    input_config.set_editor_property("default_mapping_context", imc)
    fill_native_binds(input_config, [(ia_move, "InputTag.Move")])
    fill_ability_binds(
        input_config,
        [
            (ia_undo, "InputTag.Undo"),
            (ia_redo, "InputTag.Redo"),
            (ia_restart, "InputTag.Restart"),
            (ia_pause, "InputTag.Pause"),
        ],
    )
    unreal.EditorAssetLibrary.save_loaded_asset(input_config)

    old_ability_set = CHAR_DIR + "/DA_AbilitySet_Player"
    if unreal.EditorAssetLibrary.does_asset_exist(old_ability_set):
        unreal.EditorAssetLibrary.delete_asset(old_ability_set)

    action_set = None
    if hasattr(unreal, "BoxActionSet"):
        action_set = create_or_load_da("DA_ActionSet_Player", CHAR_DIR, unreal.BoxActionSet)
        action_set.apply_official_defaults()
        unreal.EditorAssetLibrary.save_loaded_asset(action_set)

    relationships = None
    if hasattr(unreal, "BoxAbilityTagRelationshipMapping"):
        relationships = create_or_load_da(
            "DA_AbilityTagRelationships_Player", CHAR_DIR, unreal.BoxAbilityTagRelationshipMapping
        )
        relationships.apply_official_defaults()
        unreal.EditorAssetLibrary.save_loaded_asset(relationships)

    player = create_or_load_da("DA_Player", CHAR_DIR, unreal.PlayerDef)
    player.apply_official_defaults()
    player.set_editor_property("input_config", input_config)
    if action_set:
        player.set_editor_property("action_set", action_set)
    if relationships:
        player.set_editor_property("tag_relationship_mapping", relationships)
    unreal.EditorAssetLibrary.save_loaded_asset(player)


def list_created():
    paths = [
        INTERACT_DIR + "/DA_Box_Normal",
        INTERACT_DIR + "/DA_Box_Slide",
        INTERACT_DIR + "/DA_Box_Return",
        INTERACT_DIR + "/DA_Target",
        INTERACT_DIR + "/DA_Pedal",
        CHAR_DIR + "/DA_Player",
        CHAR_DIR + "/DA_ActionSet_Player",
        CHAR_DIR + "/DA_AbilityTagRelationships_Player",
        INPUT_DIR + "/DA_InputConfig_Player",
        INPUT_DIR + "/IMC_Player",
        INPUT_DIR + "/IA_Move",
        INPUT_DIR + "/IA_Undo",
        INPUT_DIR + "/IA_Redo",
        INPUT_DIR + "/IA_Restart",
        INPUT_DIR + "/IA_Pause",
    ]
    ok = []
    missing = []
    for path in paths:
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            ok.append(path)
        else:
            missing.append(path)
    unreal.log("BoxPush defs ok: " + ", ".join(ok))
    if missing:
        unreal.log_warning("BoxPush defs missing: " + ", ".join(missing))
    unreal.EditorAssetLibrary.save_directory("/Game/Data", True, True)
    return missing


def main():
    created = []
    if hasattr(unreal, "InteractableDef"):
        created.extend(create_interactables())
    else:
        unreal.log_warning("InteractableDef 尚未加载，跳过交互物 DA。")
    if hasattr(unreal, "PlayerDef") and hasattr(unreal, "BoxInputConfig"):
        create_player()
        created.extend(["DA_Player", "DA_InputConfig_Player", "DA_ActionSet_Player", "DA_AbilityTagRelationships_Player"])
    else:
        unreal.log_warning("PlayerDef / BoxInputConfig 尚未加载，跳过角色 DA。请编译后再跑。")
    unreal.log("BoxPush defs created: " + ", ".join(created))
    list_created()


main()
