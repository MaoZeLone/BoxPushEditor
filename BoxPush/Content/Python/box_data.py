"""DataTable / 资产创建。角色 3C 请走 InputConfig.AddNativeInputAction、ActionSet.AddGrantedAction。"""

import unreal


def lib():
    if not hasattr(unreal, "BoxDataLibrary"):
        raise RuntimeError("BoxDataLibrary 未加载，请编译 BoxPush 模块后再跑。")
    return unreal.BoxDataLibrary


def create_or_load(asset_class, package_path):
    asset = lib().create_or_load_asset(asset_class, package_path)
    if not asset:
        raise RuntimeError("Failed to create/load " + package_path)
    return asset


def create_or_load_table(row_struct, package_path):
    table = lib().create_or_load_data_table(row_struct, package_path)
    if not table:
        raise RuntimeError("Failed to create/load table " + package_path)
    return table


def save(asset):
    if not lib().save_asset(asset):
        raise RuntimeError("Failed to save " + str(asset))


def set_table_row(table, row_name, export_text):
    if not lib().set_table_row_export_text(table, row_name, export_text):
        raise RuntimeError("Failed to write row " + str(row_name))
