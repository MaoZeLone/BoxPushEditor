import sys
import time

sys.path.insert(
    0,
    r"C:\Program Files\Epic Games\UE_5.5\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python",
)

import remote_execution as re

SCRIPT = r"C:/Users/Maozelone/OneDrive/BoxPush/BoxPush/Content/Python/SeedOfficialAssets.py"


def main():
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
            raise RuntimeError("No Unreal Python remote node found")
        node = session.remote_nodes[0]
        session.open_command_connection(node["node_id"])
        result = session.run_command(SCRIPT, unattended=True, exec_mode=re.MODE_EXEC_FILE, raise_on_failure=True)
        print(result)
    finally:
        session.stop()


if __name__ == "__main__":
    main()
