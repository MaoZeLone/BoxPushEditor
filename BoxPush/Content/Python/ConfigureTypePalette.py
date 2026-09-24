"""Write official Type tags, terrain DAs, and DT_TypeDisplay. Does not reset levels."""

import unreal

DATA_DIR = "/Game/Data"
TERRAIN_DIR = "/Game/Data/Terrain"
INTERACT_DIR = "/Game/Data/Interactables"
CHAR_DIR = "/Game/Data/Characters"
REPORT = []


def log(msg):
    unreal.log("[BoxPush Type] " + msg)
    REPORT.append(msg)


def tag_text(tag):
    if not tag:
        return ""
    try:
        name = tag.get_editor_property("tag_name")
        text = str(name) if name is not None else ""
        return "" if text in ("None", "none") else text
    except Exception:
        return str(tag)


def tag_is_set(tag):
    return bool(tag_text(tag))


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
    names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
    log("DT_TypeDisplay rows=" + str([str(n) for n in names]))


def seed_terrain():
    ensure_dir(TERRAIN_DIR)
    for terrain_id in ("Floor", "Wall", "Empty"):
        name = "DA_Terrain_" + terrain_id
        asset = create_or_load_da(name, TERRAIN_DIR, unreal.TerrainDef)
        asset.apply_official_defaults(terrain_id)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        tag = asset.get_editor_property("type")
        log(name + " Type=" + (tag_text(tag) or "none"))


def apply_type_only():
    pairs = (
        (INTERACT_DIR + "/DA_Box_Normal", "Box_Normal"),
        (INTERACT_DIR + "/DA_Box_Slide", "Box_Slide"),
        (INTERACT_DIR + "/DA_Box_Return", "Box_Return"),
        (INTERACT_DIR + "/DA_Target", "Target"),
        (INTERACT_DIR + "/DA_Pedal", "Pedal"),
    )
    for path, definition_id in pairs:
        if not unreal.EditorAssetLibrary.does_asset_exist(path):
            log("missing " + path)
            continue
        asset = unreal.EditorAssetLibrary.load_asset(path)
        tag = asset.get_editor_property("type")
        if not tag_is_set(tag):
            asset.apply_official_defaults(definition_id)
            tag = asset.get_editor_property("type")
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        log(definition_id + " Type=" + (tag_text(tag) or "none"))

    player_path = CHAR_DIR + "/DA_Player"
    if unreal.EditorAssetLibrary.does_asset_exist(player_path):
        player = unreal.EditorAssetLibrary.load_asset(player_path)
        tag = player.get_editor_property("type")
        if not tag_is_set(tag):
            player.apply_official_defaults()
            tag = player.get_editor_property("type")
        unreal.EditorAssetLibrary.save_loaded_asset(player)
        log("DA_Player Type=" + (tag_text(tag) or "none"))


def main():
    for name in ("TerrainDef", "BoxTypeDisplayLibrary", "InteractableDef", "PlayerDef"):
        if not hasattr(unreal, name):
            raise RuntimeError(name + " 未加载，请先编译 BoxPush 再跑。")
    seed_type_display()
    seed_terrain()
    apply_type_only()
    unreal.EditorAssetLibrary.save_directory("/Game/Data", True, True)
    log("done")
    print("\n".join(REPORT))


if __name__ == "__main__":
    main()
