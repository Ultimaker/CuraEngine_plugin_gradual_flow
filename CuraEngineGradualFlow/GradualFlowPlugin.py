import os
import platform
import stat
import sys
from pathlib import Path

from UM.Logger import Logger
from UM.Settings.ContainerRegistry import ContainerRegistry
from UM.i18n import i18nCatalog
from cura.BackendPlugin import BackendPlugin
from cura.CuraApplication import CuraApplication


catalog = i18nCatalog("cura")


class GradualFlowPlugin(BackendPlugin):
    def __init__(self):
        super().__init__()
        self.definition_file_paths = [Path(__file__).parent.joinpath("gradual_flow_settings.def.json").as_posix()]
        if not self.isDebug():
            if not self.binaryPath().exists():
                Logger.error(f"Could not find curaengine_plugin_gradual_flow binary at {self.binaryPath().as_posix()}")
            if platform.system() != "Windows" and self.binaryPath().exists():
                st = os.stat(self.binaryPath())
                os.chmod(self.binaryPath(), st.st_mode | stat.S_IEXEC)

            self._plugin_command = [self.binaryPath().as_posix()]

        self._supported_slots = [103]  # GCODE_PATHS_MODIFY SlotID
        ContainerRegistry.getInstance().containerLoadComplete.connect(self._on_container_load_complete)

    def _on_container_load_complete(self, container_id) -> None:
        pass

    def gradualFlowEnabled(self):
        # FIXME: This should only be True when we actually use it for any extruder
        return CuraApplication.getInstance().getGlobalContainerStack().getProperty(f"_plugin__curaenginegradualflow__0_1_0__gradual_flow_enabled", "value")

    def getPort(self):
        return super().getPort() if not self.isDebug() else int(os.environ["CURAENGINE_GCODE_PATHS_MODIFY_PORT"])

    def isDebug(self):
        return not hasattr(sys, "frozen") and os.environ.get("CURAENGINE_GCODE_PATHS_MODIFY_PORT", None) is not None

    def start(self):
        if self.gradualFlowEnabled() and not self.isDebug():
            super().start()

    def binaryPath(self) -> Path:
        ext = ".exe" if platform.system() == "Windows" else ""

        machine = platform.machine()
        if machine == "AMD64":
            machine = "x86_64"
        return Path(__file__).parent.joinpath(machine, platform.system(), f"curaengine_plugin_gradual_flow{ext}").resolve()
