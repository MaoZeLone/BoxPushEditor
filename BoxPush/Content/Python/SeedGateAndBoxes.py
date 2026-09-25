"""普通箱与滑动箱用不同裁切。新建触发器和可消失的墙，并写上贴图。"""

import os

import unreal

ATLAS_PATH = "/Game/Data/Sprites/T_SokobanAtlas"
INTERACT_DIR = "/Game/Data/Interactables"


def log(msg):
    unreal.log("[BoxPush Gate] " + msg)


def save(asset, label):
    asset.modify()
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset):
        raise RuntimeError("保存失败 " + label)
    log(label)


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


def sprite_task(comp_id, x, y, w, h, texture):
    task = unreal.VisualTask()
    task.set_editor_property("type", unreal.VisualTaskType.SET_SPRITE)
    task.set_editor_property("comp_id", unreal.Name(comp_id))
    task.set_editor_property("texture", texture)
    task.set_editor_property("source_x", x)
    task.set_editor_property("source_y", y)
    task.set_editor_property("source_w", w)
    task.set_editor_property("source_h", h)
    return task


def import_atlas():
    png = os.path.normpath(os.path.join(unreal.Paths.project_dir(), "Asset", "Spritesheet", "sokoban_spritesheet.png"))
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
    return texture


def load_or_create(definition_id):
    name = "DA_" + definition_id
    path = INTERACT_DIR + "/" + name
    asset = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if asset:
        return asset
    if not unreal.EditorAssetLibrary.does_directory_exist(INTERACT_DIR):
        unreal.EditorAssetLibrary.make_directory(INTERACT_DIR)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("DataAssetClass", unreal.InteractableDef)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, INTERACT_DIR, unreal.InteractableDef, factory)
    if not asset:
        raise RuntimeError("没能新建 " + path)
    return asset


def set_rect(asset, x, y, w, h, z, texture):
    sprite = make_sprite(asset, x, y, w, h, z, texture)
    asset.set_editor_property("sprite_comps", [sprite])


def tag(name):
    value = unreal.GameplayTag()
    if not value.import_text('(TagName="%s")' % name):
        raise RuntimeError("标签无效 " + name)
    return value


def state(state_id, display, default, event_id=None, tasks=None):
    row = unreal.InteractableStateDef()
    row.set_editor_property("state_id", unreal.Name(state_id))
    row.set_editor_property("display_name", unreal.Text(display))
    row.set_editor_property("default", default)
    if event_id:
        action = unreal.InteractableStateActionDef()
        action.set_editor_property("type", unreal.BoxStateActionType.FIRE_EVENT)
        action.set_editor_property("event_id", unreal.Name(event_id))
        row.set_editor_property("on_enter", [action])
    if tasks:
        row.set_editor_property("tasks", tasks)
    return row


def overlap(frm, condition, to):
    row = unreal.InteractableTransitionDef()
    row.set_editor_property("from_state", unreal.Name(frm))
    row.set_editor_property("condition", condition)
    row.set_editor_property("to_state", unreal.Name(to))
    return row


def on_event(frm, event_id, to):
    row = unreal.InteractableTransitionDef()
    row.set_editor_property("from_state", unreal.Name(frm))
    row.set_editor_property("condition", unreal.BoxTransitionCondition.ON_EVENT)
    row.set_editor_property("event_id", unreal.Name(event_id))
    row.set_editor_property("to_state", unreal.Name(to))
    return row


def visible_task(visible):
    task = unreal.VisualTask()
    task.set_editor_property("type", unreal.VisualTaskType.SET_VISIBLE)
    task.set_editor_property("comp_id", unreal.Name("Sprite"))
    task.set_editor_property("visible", visible)
    return task


def blocking_task(blocks_player, blocks_push):
    task = unreal.VisualTask()
    task.set_editor_property("type", unreal.VisualTaskType.SET_BLOCKING)
    task.set_editor_property("blocks_player", blocks_player)
    task.set_editor_property("blocks_push", blocks_push)
    return task


def main():
    if not hasattr(unreal, "VisualTask"):
        raise RuntimeError("VisualTask 未加载，请先编译 BoxPush 再跑。")
    texture = import_atlas()

    normal = unreal.EditorAssetLibrary.load_asset(INTERACT_DIR + "/DA_Box_Normal")
    if not normal:
        raise RuntimeError("没有 DA_Box_Normal")
    set_rect(normal, 384, 256, 64, 64, 10.0, texture)
    save(normal, "DA_Box_Normal 木色箱子")

    slide = unreal.EditorAssetLibrary.load_asset(INTERACT_DIR + "/DA_Box_Slide")
    if not slide:
        raise RuntimeError("没有 DA_Box_Slide")
    set_rect(slide, 384, 128, 64, 64, 10.0, texture)
    save(slide, "DA_Box_Slide 另一款箱子")

    trigger = load_or_create("Trigger")
    trigger.set_editor_property("definition_id", unreal.Name("Trigger"))
    trigger.set_editor_property("display_name", unreal.Text("触发器"))
    trigger.set_editor_property("type", tag("Type.Interactable"))
    trigger.set_editor_property("designer_note", "箱子留在上面发出 WallOpen，离开发出 WallClose。")
    logic = unreal.new_object(unreal.TriggerLogic, trigger, name="")
    logic.set_editor_property("comp_id", unreal.Name("Trigger"))
    logic.set_editor_property("accept_type", tag("Type.Interactable.Box"))
    logic.set_editor_property("allow_accept_type", True)
    trigger.set_editor_property("logic_comps", [logic])
    trigger.set_editor_property("states", [
        state("Empty", "空", True, "WallClose"),
        state("Held", "压住", False, "WallOpen", [sprite_task("Sprite", 64, 64, 64, 64, texture)]),
    ])
    trigger.set_editor_property("transitions", [
        overlap("Empty", unreal.BoxTransitionCondition.BEGIN_OVERLAP, "Held"),
        overlap("Held", unreal.BoxTransitionCondition.END_OVERLAP, "Empty"),
    ])
    set_rect(trigger, 64, 256, 64, 64, 4.0, texture)
    trigger.set_editor_property("palette_color", unreal.LinearColor(0.95, 0.75, 0.2, 1.0))
    save(trigger, "DA_Trigger")

    gate = load_or_create("Gate")
    gate.set_editor_property("definition_id", unreal.Name("Gate"))
    gate.set_editor_property("display_name", unreal.Text("可消失的墙"))
    gate.set_editor_property("type", tag("Type.Interactable"))
    gate.set_editor_property("designer_note", "听到 WallOpen 进入消失：藏起贴图，并不再挡路。WallClose 回到挡住。")
    blocking = unreal.new_object(unreal.BlockingLogic, gate, name="")
    blocking.set_editor_property("comp_id", unreal.Name("Blocking"))
    gate.set_editor_property("logic_comps", [blocking])
    gate.set_editor_property("states", [
        state("Closed", "挡住", True),
        state("Open", "消失", False, None, [visible_task(False), blocking_task(False, False)]),
    ])
    gate.set_editor_property("transitions", [
        on_event("Closed", "WallOpen", "Open"),
        on_event("Open", "WallClose", "Closed"),
    ])
    set_rect(gate, 320, 448, 64, 64, 10.0, texture)
    gate.set_editor_property("palette_color", unreal.LinearColor(0.45, 0.48, 0.55, 1.0))
    save(gate, "DA_Gate")


main()
