"""把 Kenney 图集贴图和裁切写进原来的交互物、角色、地形 DA。不调用 ApplyOfficialDefaults，避免清掉逻辑。"""

import os

import unreal

ATLAS_PATH = "/Game/Data/Sprites/T_SokobanAtlas"

REPORT = []


def log(msg):
    unreal.log("[BoxPush Look] " + msg)
    REPORT.append(msg)


def frame(x, y, w, h, texture):
    item = unreal.BoxAtlasFrame()
    item.set_editor_property("texture", texture)
    item.set_editor_property("source_x", x)
    item.set_editor_property("source_y", y)
    item.set_editor_property("source_w", w)
    item.set_editor_property("source_h", h)
    return item


def facing(idle, walk_a, walk_b, push_a, push_b, texture):
    look = unreal.PlayerFacingLook()
    look.set_editor_property("idle", frame(*idle, texture))
    look.set_editor_property("walk", [frame(*walk_a, texture), frame(*walk_b, texture)])
    look.set_editor_property("push", [frame(*push_a, texture), frame(*push_b, texture)])
    return look


def make_sprite(outer, x, y, w, h, z, texture):
    sprite = unreal.new_object(unreal.VisualSpriteComp, outer, name="")
    sprite.set_editor_property("comp_id", unreal.Name("Sprite"))
    sprite.set_editor_property("texture", texture)
    sprite.set_editor_property("source_x", x)
    sprite.set_editor_property("source_y", y)
    sprite.set_editor_property("source_w", w)
    sprite.set_editor_property("source_h", h)
    sprite.set_editor_property("relative_location", unreal.Vector(0.0, 0.0, z))
    return sprite


def import_atlas():
    png = os.path.normpath(os.path.join(unreal.Paths.project_dir(), "..", "Asset", "Spritesheet", "sokoban_spritesheet.png"))
    if not os.path.isfile(png):
        raise RuntimeError("找不到图集 " + png)
    if not unreal.EditorAssetLibrary.does_asset_exist(ATLAS_PATH):
        task = unreal.AssetImportTask()
        task.filename = png
        task.destination_path = "/Game/Data/Sprites"
        task.destination_name = "T_SokobanAtlas"
        task.automated = True
        task.save = True
        task.replace_existing = True
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.EditorAssetLibrary.load_asset(ATLAS_PATH)
    if not texture:
        raise RuntimeError("图集没导入成功")
    texture.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property("srgb", True)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    save(texture, ATLAS_PATH)
    return texture


def save(asset, label):
    asset.modify()
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError("保存失败 " + label)
    log(label)


def load(path):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if not asset:
        raise RuntimeError("打不开 " + path)
    return asset


def seed_interactable(path, rect, z, texture):
    asset = load(path)
    sprite = make_sprite(asset, rect[0], rect[1], rect[2], rect[3], z, texture)
    asset.set_editor_property("sprite_comps", [sprite])
    save(asset, path)


def seed_terrain(path, rect, texture):
    asset = load(path)
    asset.set_editor_property("sprite", make_sprite(asset, rect[0], rect[1], rect[2], rect[3], 0.0, texture))
    save(asset, path)


def player_look(facing, pushing, walking, step):
    idle = {
        0: (554, 158, 42, 50),
        1: (493, 548, 45, 50),
        2: (558, 0, 42, 50),
        3: (543, 490, 45, 50),
    }
    walk = {
        0: ((546, 350, 42, 50), (546, 300, 42, 50)),
        1: ((512, 108, 45, 50), (448, 548, 45, 50)),
        2: ((554, 208, 42, 50), (557, 108, 42, 50)),
        3: ((543, 440, 45, 50), (538, 548, 45, 50)),
    }
    push = {
        0: ((512, 0, 46, 54), (497, 440, 46, 54)),
        1: ((448, 440, 49, 54), (448, 494, 49, 54)),
        2: ((508, 192, 46, 54), (497, 494, 46, 54)),
        3: ((497, 386, 49, 54), (448, 332, 49, 54)),
    }
    if pushing:
        return push[facing][step]
    if walking:
        return walk[facing][step]
    return idle[facing]


def main():
    if not hasattr(unreal, "VisualSpriteComp"):
        raise RuntimeError("VisualSpriteComp 未加载，请先编译 BoxPush 再跑。")

    texture = import_atlas()
    crate = (384, 256, 64, 64)
    goal = (64, 384, 64, 64)
    pedal = (0, 448, 64, 64)
    seed_interactable("/Game/Data/Interactables/DA_Box_Normal", crate, 10.0, texture)
    seed_interactable("/Game/Data/Interactables/DA_Box_Slide", crate, 10.0, texture)
    seed_interactable("/Game/Data/Interactables/DA_Box_Return", crate, 10.0, texture)
    seed_interactable("/Game/Data/Interactables/DA_Target", goal, 4.0, texture)
    seed_interactable("/Game/Data/Interactables/DA_Pedal", pedal, 4.0, texture)

    seed_terrain("/Game/Data/Terrain/DA_Terrain_Floor", (64, 128, 64, 64), texture)
    seed_terrain("/Game/Data/Terrain/DA_Terrain_Wall", (320, 448, 64, 64), texture)

    player = ensure_player(texture)
    save(player, "/Game/Data/Characters/DA_Player")

    box = load("/Game/Data/Interactables/DA_Box_Normal")
    sprites = box.get_editor_property("sprite_comps")
    if len(sprites) != 1:
        raise RuntimeError("DA_Box_Normal 表现组件数量不对")
    log("箱子裁切 X=" + str(sprites[0].get_editor_property("source_x")))
    floor = load("/Game/Data/Terrain/DA_Terrain_Floor")
    floor_sprite = floor.get_editor_property("sprite")
    if not floor_sprite:
        raise RuntimeError("地板没有表现")
    log("地板裁切 X=" + str(floor_sprite.get_editor_property("source_x")))
    print("\n".join(REPORT))


def write_sprite(sprite, texture):
    sprite.set_editor_property("frame_interval", 0.16)
    sprite.set_editor_property("north", facing(player_look(0, False, False, 0), player_look(0, False, True, 0), player_look(0, False, True, 1), player_look(0, True, False, 0), player_look(0, True, False, 1), texture))
    sprite.set_editor_property("screen_right", facing(player_look(1, False, False, 0), player_look(1, False, True, 0), player_look(1, False, True, 1), player_look(1, True, False, 0), player_look(1, True, False, 1), texture))
    sprite.set_editor_property("south", facing(player_look(2, False, False, 0), player_look(2, False, True, 0), player_look(2, False, True, 1), player_look(2, True, False, 0), player_look(2, True, False, 1), texture))
    sprite.set_editor_property("screen_left", facing(player_look(3, False, False, 0), player_look(3, False, True, 0), player_look(3, False, True, 1), player_look(3, True, False, 0), player_look(3, True, False, 1), texture))


def ensure_sprite(texture):
    path = "/Game/Data/Characters/DA_PlayerSprite"
    sprite = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not sprite:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("DataAssetClass", unreal.PlayerSpriteDef)
        sprite = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_PlayerSprite", "/Game/Data/Characters", unreal.PlayerSpriteDef, factory
        )
    if not sprite:
        raise RuntimeError("DA_PlayerSprite 没能新建")
    write_sprite(sprite, texture)
    save(sprite, path)
    return sprite


def ensure_player(texture):
    sprite = ensure_sprite(texture)
    path = "/Game/Data/Characters/DA_Player"
    player = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if player:
        player.set_editor_property("sprite", sprite)
        log("DA_Player 引用 DA_PlayerSprite")
        return player

    factory = unreal.DataAssetFactory()
    factory.set_editor_property("DataAssetClass", unreal.PlayerDef)
    player = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "DA_Player", "/Game/Data/Characters", unreal.PlayerDef, factory
    )
    if not player:
        raise RuntimeError("DA_Player 没能新建")
    player.apply_official_defaults()

    def attach(prop, asset_path):
        other = unreal.load_asset(asset_path)
        if other:
            player.set_editor_property(prop, other)
            log("挂上 " + asset_path)
        else:
            log("没找到 " + asset_path)

    attach("input_config", "/Game/Data/Characters/Input/DA_InputConfig_Player")
    attach("action_set", "/Game/Data/Characters/DA_ActionSet_Player")
    attach("tag_relationship_mapping", "/Game/Data/Characters/DA_AbilityTagRelationships_Player")
    player.set_editor_property("sprite", sprite)
    log("DA_Player 原文件读不了，已重建并引用 DA_PlayerSprite")
    return player


main()
