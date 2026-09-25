"""Create / refresh official BoxPush config assets at documented paths."""

import unreal

CHAR_DIR = "/Game/Data/Characters"
INPUT_DIR = "/Game/Data/Characters/Input"
INTERACT_DIR = "/Game/Data/Interactables"
LEVEL_DIR = "/Game/Data/Levels/BuiltIn"
DATA_DIR = "/Game/Data"
MATERIAL_DIR = "/Game/Data/Materials"
TERRAIN_DIR = "/Game/Data/Terrain"
SPRITE_DIR = "/Game/Data/2D/Interactables"
FX_DIR = "/Game/Data/FX"
TARGET_FX_SRC = "/Game/StarterContent/Particles/P_Steam_Lit"
TARGET_FX_DST = FX_DIR + "/PS_Target"

OFFICIAL_INTERACTABLES = (
    "Box_Normal",
    "Box_Slide",
    "Box_Return",
    "Target",
    "Pedal",
)

REPORT = []


def log(msg):
    unreal.log("[BoxPush Seed] " + msg)
    REPORT.append(msg)


def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def create_or_load_da(asset_name, directory, da_class):
    path = directory + "/" + asset_name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if asset:
            return asset
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


def create_or_load_imc():
    path = INPUT_DIR + "/IMC_Player"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if asset:
            return asset
    factory = unreal.InputMappingContext_Factory()
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "IMC_Player", INPUT_DIR, unreal.InputMappingContext, factory
    )
    if not asset:
        raise RuntimeError("Failed to create IMC_Player")
    return asset


def bind_key(imc, action, key_name, swizzle=False, negate=False):
    key = unreal.Key()
    key.set_editor_property("key_name", key_name)
    mapping = imc.map_key(action, key)
    modifiers = []
    if swizzle:
        sw = unreal.InputModifierSwizzleAxis()
        sw.set_editor_property("order", unreal.InputAxisSwizzle.YXZ)
        modifiers.append(sw)
    if negate:
        modifiers.append(unreal.InputModifierNegate())
    if modifiers:
        try:
            mapping.set_editor_property("modifiers", modifiers)
        except Exception:
            pass


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


def seed_target_fx():
    ensure_dir(FX_DIR)
    if unreal.EditorAssetLibrary.does_asset_exist(TARGET_FX_DST):
        log("PS_Target")
        return
    if not unreal.EditorAssetLibrary.does_asset_exist(TARGET_FX_SRC):
        raise RuntimeError("missing starter fx " + TARGET_FX_SRC)
    if not unreal.EditorAssetLibrary.duplicate_asset(TARGET_FX_SRC, TARGET_FX_DST):
        raise RuntimeError("failed to duplicate " + TARGET_FX_SRC + " -> " + TARGET_FX_DST)
    unreal.EditorAssetLibrary.save_asset(TARGET_FX_DST)
    log("PS_Target")


def seed_tile_visual():
    ensure_dir(DATA_DIR)
    ensure_dir(MATERIAL_DIR)
    cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
    parent = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/BasicShapeMaterial")

    def make_mic(name, r, g, b):
        path = MATERIAL_DIR + "/" + name
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            asset = unreal.EditorAssetLibrary.load_asset(path)
        else:
            factory = unreal.MaterialInstanceConstantFactoryNew()
            asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                name, MATERIAL_DIR, unreal.MaterialInstanceConstant, factory
            )
        if not asset:
            raise RuntimeError("Failed to create " + path)
        if parent:
            asset.set_editor_property("parent", parent)
        if hasattr(unreal, "MaterialEditingLibrary"):
            color = unreal.LinearColor(r, g, b, 1.0)
            unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
                asset, "Color", color
            )
            if hasattr(unreal.MaterialEditingLibrary, "update_material_instance"):
                unreal.MaterialEditingLibrary.update_material_instance(asset)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        return asset

    floor_mat = make_mic("MI_Floor", 0.32, 0.34, 0.38)
    wall_mat = make_mic("MI_Wall", 0.08, 0.09, 0.11)
    asset = create_or_load_da("DA_TileVisual", DATA_DIR, unreal.TileVisual)
    asset.apply_official_defaults()
    if cube:
        asset.set_editor_property("floor_mesh", cube)
        asset.set_editor_property("wall_mesh", cube)
    asset.set_editor_property("floor_material", floor_mat)
    asset.set_editor_property("wall_material", wall_mat)
    asset.set_editor_property("cell_size", 200.0)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    log("DA_TileVisual")


def seed_terrain():
    ensure_dir(TERRAIN_DIR)
    for terrain_id in ("Floor", "Wall", "Empty"):
        name = "DA_Terrain_" + terrain_id
        asset = create_or_load_da(name, TERRAIN_DIR, unreal.TerrainDef)
        asset.apply_official_defaults(terrain_id)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        log("terrain " + name)


def seed_type_display():
    ensure_dir(DATA_DIR)
    path = DATA_DIR + "/DT_TypeDisplay"
    row_struct = unreal.BoxTypeDisplayRow.static_struct()
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        table = unreal.EditorAssetLibrary.load_asset(path)
    else:
        factory = unreal.DataTableFactory()
        factory.set_editor_property("Struct", row_struct)
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DT_TypeDisplay", DATA_DIR, unreal.DataTable, factory
        )
    if not table:
        raise RuntimeError("Failed to create DT_TypeDisplay")
    unreal.BoxTypeDisplayLibrary.apply_official_defaults(table)
    unreal.EditorAssetLibrary.save_loaded_asset(table)
    log("DT_TypeDisplay")


def seed_interactables():
    ensure_dir(INTERACT_DIR)
    for definition_id in OFFICIAL_INTERACTABLES:
        name = "DA_" + definition_id
        asset = create_or_load_da(name, INTERACT_DIR, unreal.InteractableDef)
        asset.apply_official_defaults(definition_id)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        log("interactable " + name)


def seed_interactable_sprites():
    ensure_dir("/Game/Data/2D")
    ensure_dir(SPRITE_DIR)
    for definition_id in OFFICIAL_INTERACTABLES:
        name = "DA_" + definition_id
        asset = create_or_load_da(name, SPRITE_DIR, unreal.InteractableSpriteDef)
        asset.apply_official_defaults(definition_id)
        asset.modify()
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        log("sprite " + name)


def seed_player():
    ensure_dir(CHAR_DIR)
    ensure_dir(INPUT_DIR)
    ensure_dir(CHAR_DIR + "/Camera")

    ia_move = create_or_load_input_action("IA_Move", unreal.InputActionValueType.AXIS2D)
    ia_undo = create_or_load_input_action("IA_Undo", unreal.InputActionValueType.BOOLEAN)
    ia_redo = create_or_load_input_action("IA_Redo", unreal.InputActionValueType.BOOLEAN)
    ia_restart = create_or_load_input_action("IA_Restart", unreal.InputActionValueType.BOOLEAN)
    ia_pause = create_or_load_input_action("IA_Pause", unreal.InputActionValueType.BOOLEAN)

    imc = create_or_load_imc()
    imc.unmap_all()
    bind_key(imc, ia_move, "W", swizzle=True)
    bind_key(imc, ia_move, "S", swizzle=True, negate=True)
    bind_key(imc, ia_move, "D")
    bind_key(imc, ia_move, "A", negate=True)
    bind_key(imc, ia_move, "Up", swizzle=True)
    bind_key(imc, ia_move, "Down", swizzle=True, negate=True)
    bind_key(imc, ia_move, "Right")
    bind_key(imc, ia_move, "Left", negate=True)
    bind_key(imc, ia_undo, "Z")
    bind_key(imc, ia_redo, "Y")
    bind_key(imc, ia_restart, "R")
    bind_key(imc, ia_pause, "Escape")
    unreal.EditorAssetLibrary.save_loaded_asset(imc)
    log("IMC_Player")

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
    log("DA_InputConfig_Player")

    for stale in (CHAR_DIR + "/DA_AbilitySet_Player", "/Game/Characters/DA_AbilitySet_Player"):
        if unreal.EditorAssetLibrary.does_asset_exist(stale):
            unreal.EditorAssetLibrary.delete_asset(stale)
            log("deleted " + stale)

    action_set = create_or_load_da("DA_ActionSet_Player", CHAR_DIR, unreal.BoxActionSet)
    action_set.apply_official_defaults()
    unreal.EditorAssetLibrary.save_loaded_asset(action_set)
    log("DA_ActionSet_Player")

    relationships = create_or_load_da(
        "DA_AbilityTagRelationships_Player", CHAR_DIR, unreal.BoxAbilityTagRelationshipMapping
    )
    relationships.apply_official_defaults()
    unreal.EditorAssetLibrary.save_loaded_asset(relationships)
    log("DA_AbilityTagRelationships_Player")

    player = create_or_load_da("DA_Player", CHAR_DIR, unreal.PlayerDef)
    player.apply_official_defaults()
    player.set_editor_property("input_config", input_config)
    player.set_editor_property("action_set", action_set)
    player.set_editor_property("tag_relationship_mapping", relationships)
    if hasattr(unreal.PlayerDef, "seed_official_play_camera"):
        camera = unreal.PlayerDef.seed_official_play_camera(True)
        if camera:
            player.set_editor_property("default_camera_asset", camera)
            log("CA_Play")
    unreal.EditorAssetLibrary.save_loaded_asset(player)
    log("DA_Player")


def seed_level_and_catalog():
    ensure_dir(LEVEL_DIR)
    ensure_dir(DATA_DIR)

    level_path = LEVEL_DIR + "/DA_Level_01"
    if unreal.EditorAssetLibrary.does_asset_exist(level_path):
        level = unreal.EditorAssetLibrary.load_asset(level_path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("DataAssetClass", unreal.LevelData)
        level = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_Level_01", LEVEL_DIR, unreal.LevelData, factory
        )
    if not level:
        raise RuntimeError("Failed to create DA_Level_01")
    level.apply_new_level_defaults("LV_01")
    level.set_editor_property("display_name", unreal.Text("推一下"))
    level.set_editor_property("designer_note", "第一关：把箱子直线推上目标。")
    if hasattr(level, "par_moves"):
        level.set_editor_property("par_moves", 3)
    unreal.EditorAssetLibrary.save_loaded_asset(level)
    log("DA_Level_01")

    catalog_path = DATA_DIR + "/DT_LevelCatalog"
    row_struct = unreal.LevelCatalogRow.static_struct()
    if unreal.EditorAssetLibrary.does_asset_exist(catalog_path):
        table = unreal.EditorAssetLibrary.load_asset(catalog_path)
    else:
        factory = unreal.DataTableFactory()
        factory.set_editor_property("Struct", row_struct)
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DT_LevelCatalog", DATA_DIR, unreal.DataTable, factory
        )
    if not table:
        raise RuntimeError("Failed to create DT_LevelCatalog")

    json_text = """[
        {
            "Name": "LV_01",
            "LevelId": "LV_01",
            "LevelAsset": "/Game/Data/Levels/BuiltIn/DA_Level_01.DA_Level_01",
            "SortOrder": 0,
            "bListed": true
        }
    ]"""
    ok = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json_text, row_struct)
    if not ok and hasattr(unreal, "BoxDataLibrary"):
        export_text = '(LevelId=LV_01,LevelAsset="/Game/Data/Levels/BuiltIn/DA_Level_01.DA_Level_01",SortOrder=0,bListed=True)'
        ok = unreal.BoxDataLibrary.set_table_row_export_text(table, "LV_01", export_text)
    if not ok:
        raise RuntimeError("Failed to fill DT_LevelCatalog")
    unreal.EditorAssetLibrary.save_loaded_asset(table)
    log("DT_LevelCatalog LV_01")


def required_paths():
    return [
        INTERACT_DIR + "/DA_Box_Normal",
        INTERACT_DIR + "/DA_Box_Slide",
        INTERACT_DIR + "/DA_Box_Return",
        INTERACT_DIR + "/DA_Target",
        INTERACT_DIR + "/DA_Pedal",
        SPRITE_DIR + "/DA_Box_Normal",
        SPRITE_DIR + "/DA_Box_Slide",
        SPRITE_DIR + "/DA_Box_Return",
        SPRITE_DIR + "/DA_Target",
        SPRITE_DIR + "/DA_Pedal",
        CHAR_DIR + "/DA_Player",
        CHAR_DIR + "/Camera/CA_Play",
        CHAR_DIR + "/DA_ActionSet_Player",
        CHAR_DIR + "/DA_AbilityTagRelationships_Player",
        INPUT_DIR + "/DA_InputConfig_Player",
        INPUT_DIR + "/IMC_Player",
        INPUT_DIR + "/IA_Move",
        INPUT_DIR + "/IA_Undo",
        INPUT_DIR + "/IA_Redo",
        INPUT_DIR + "/IA_Restart",
        INPUT_DIR + "/IA_Pause",
        LEVEL_DIR + "/DA_Level_01",
        DATA_DIR + "/DT_LevelCatalog",
        DATA_DIR + "/DT_TypeDisplay",
        DATA_DIR + "/DA_TileVisual",
        TERRAIN_DIR + "/DA_Terrain_Floor",
        TERRAIN_DIR + "/DA_Terrain_Wall",
        TERRAIN_DIR + "/DA_Terrain_Empty",
        FX_DIR + "/PS_Target",
    ]


def verify():
    missing = []
    for path in required_paths():
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            log("ok " + path)
        else:
            missing.append(path)
            log("MISSING " + path)
    return missing


def directory_has_assets(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        return False
    return bool(unreal.EditorAssetLibrary.list_assets(path, True, False))


def migrate_directory(src_dir, dst_dir):
    if not directory_has_assets(src_dir):
        return
    ensure_dir(dst_dir)
    for src in unreal.EditorAssetLibrary.list_assets(src_dir, True, False):
        package = src.split(".", 1)[0]
        if not package.startswith(src_dir):
            continue
        dst = dst_dir + package[len(src_dir) :]
        if package == dst:
            continue
        ensure_dir(dst.rsplit("/", 1)[0])
        if unreal.EditorAssetLibrary.does_asset_exist(dst):
            unreal.EditorAssetLibrary.delete_asset(package)
            log("dropped leftover " + package)
            continue
        if unreal.EditorAssetLibrary.rename_asset(src, dst):
            log("moved " + package + " -> " + dst)
            continue
        if unreal.EditorAssetLibrary.duplicate_asset(package, dst):
            unreal.EditorAssetLibrary.delete_asset(package)
            log("copied " + package + " -> " + dst)
            continue
        raise RuntimeError("failed to move " + package + " -> " + dst)
    if unreal.EditorAssetLibrary.does_directory_exist(src_dir) and not directory_has_assets(src_dir):
        unreal.EditorAssetLibrary.delete_directory(src_dir)
        log("deleted leftover " + src_dir)


def migrate_into_data():
    migrate_directory("/Game/Characters", CHAR_DIR)
    migrate_directory("/Game/Interactables", INTERACT_DIR)
    migrate_directory("/Game/Levels", "/Game/Data/Levels")


def main():
    needed = (
        "InteractableDef",
        "InteractableSpriteDef",
        "PlayerDef",
        "BoxInputConfig",
        "BoxActionSet",
        "BoxAbilityTagRelationshipMapping",
        "LevelData",
        "LevelCatalogRow",
        "TileVisual",
        "TerrainDef",
        "BoxTypeDisplayRow",
        "BoxTypeDisplayLibrary",
    )
    for name in needed:
        if not hasattr(unreal, name):
            raise RuntimeError(name + " 未加载，请先编译 BoxPush 再跑。")

    migrate_into_data()

    if hasattr(unreal, "BoxOfficialAssets"):
        log("using C++ BoxOfficialAssets.seed_all")
        unreal.BoxOfficialAssets.seed_all(True)

    seed_target_fx()
    seed_type_display()
    seed_terrain()
    seed_interactables()
    seed_interactable_sprites()
    seed_player()
    seed_level_and_catalog()
    seed_tile_visual()
    unreal.EditorAssetLibrary.save_directory("/Game/Data", True, True)
    missing = verify()
    if missing:
        raise RuntimeError("seed missing: " + ", ".join(missing))
    log("done")


if __name__ == "__main__":
    main()
