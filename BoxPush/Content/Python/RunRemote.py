import sys
import time

sys.path.insert(
    0,
    r"C:\Program Files\Epic Games\UE_5.5\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python",
)

import remote_execution as re

DEFAULT_SCRIPT = r"C:/Users/Maozelone/OneDrive/BoxPush/BoxPush/Content/Python/SeedOfficialAssets.py"


def run(script_path):
    config = re.RemoteExecutionConfig()
    config.multicast_bind_address = "0.0.0.0"
    session = re.RemoteExecution(config)
    session.start()
    try:
        for _ in range(20):
            time.sleep(0.25)
            if session.remote_nodes:
                break
        if not session.remote_nodes:
            raise RuntimeError("No Unreal Python remote node found. Is the editor open with Python remote execution enabled?")
        session.open_command_connection(session.remote_nodes[0]["node_id"])
        return session.run_command(script_path, unattended=True, exec_mode=re.MODE_EXEC_FILE, raise_on_failure=True)
    finally:
        session.stop()


def main():
    script = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SCRIPT
    result = run(script)
    print(result)


if __name__ == "__main__":
    main()
