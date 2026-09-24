"""Trigger Live Coding compile in the running editor (same as Ctrl+Alt+F11)."""

import unreal


def exec_cmd(command):
    world = None
    try:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    except Exception:
        try:
            world = unreal.EditorLevelLibrary.get_editor_world()
        except Exception:
            world = None
    unreal.SystemLibrary.execute_console_command(world, command)


exec_cmd("LiveCoding")
exec_cmd("LiveCoding.Compile")
unreal.log("LiveCoding.Compile requested (same as Ctrl+Alt+F11)")
