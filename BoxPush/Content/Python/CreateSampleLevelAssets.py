import unreal

LEVEL_DIR = "/Game/Data/Levels/BuiltIn"
DATA_DIR = "/Game/Data"
LEVEL_NAME = "DA_Level_01"
CATALOG_NAME = "DT_LevelCatalog"
LEVEL_PATH = LEVEL_DIR + "/" + LEVEL_NAME
CATALOG_PATH = DATA_DIR + "/" + CATALOG_NAME


def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def create_or_load_level():
    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        asset = unreal.EditorAssetLibrary.load_asset(LEVEL_PATH)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("DataAssetClass", unreal.LevelData)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            LEVEL_NAME, LEVEL_DIR, unreal.LevelData, factory
        )
    if not asset:
        raise RuntimeError("Failed to create ULevelData")
    asset.apply_new_level_defaults("LV_01")
    asset.set_editor_property("display_name", unreal.Text("推一下"))
    asset.set_editor_property("designer_note", "第一关：把箱子直线推上目标。")
    asset.set_editor_property("par_moves", 3)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return asset


def create_or_load_catalog():
    row_struct = unreal.LevelCatalogRow.static_struct()
    if unreal.EditorAssetLibrary.does_asset_exist(CATALOG_PATH):
        table = unreal.EditorAssetLibrary.load_asset(CATALOG_PATH)
    else:
        factory = unreal.DataTableFactory()
        factory.set_editor_property("Struct", row_struct)
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            CATALOG_NAME, DATA_DIR, unreal.DataTable, factory
        )
    if not table:
        raise RuntimeError("Failed to create DataTable")
    json_text = """[
        {
            "Name": "LV_01",
            "LevelId": "LV_01",
            "LevelAsset": "/Game/Data/Levels/BuiltIn/DA_Level_01.DA_Level_01",
            "SortOrder": 1,
            "bListed": true
        }
    ]"""
    ok = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json_text, row_struct)
    if not ok:
        raise RuntimeError("Failed to fill DT_LevelCatalog")
    unreal.EditorAssetLibrary.save_loaded_asset(table)
    return table


def main():
    ensure_dir(LEVEL_DIR)
    ensure_dir(DATA_DIR)
    create_or_load_level()
    create_or_load_catalog()
    unreal.log("BoxPush sample assets created: DA_Level_01, DT_LevelCatalog")


main()
