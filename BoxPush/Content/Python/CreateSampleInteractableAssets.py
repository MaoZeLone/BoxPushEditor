import unreal

INTERACT_DIR = "/Game/Data/Interactables"

OFFICIAL = (
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


def create_interactables():
    ensure_dir(INTERACT_DIR)
    created = []
    for definition_id in OFFICIAL:
        name = "DA_" + definition_id
        asset = create_or_load_da(name, INTERACT_DIR, unreal.InteractableDef)
        asset.apply_official_defaults(definition_id)
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        created.append(name)
    return created


def main():
    created = create_interactables()
    unreal.log("BoxPush interactable assets created: " + ", ".join(created))


if __name__ == "__main__":
    main()
