import os
import platform
import stat
import sys
from pathlib import Path

from UM.Logger import Logger
from UM.Settings.ContainerRegistry import ContainerRegistry
from UM.Settings.DefinitionContainer import DefinitionContainer
from UM.i18n import i18nCatalog
from cura.BackendPlugin import BackendPlugin

catalog = i18nCatalog("cura")

class GradualFlowPlugin(BackendPlugin):
    def __init__(self):
        super().__init__()

        print("INIT")

        self._supported_slots = [200]  # ModifyPostprocess SlotID
        ContainerRegistry.getInstance().containerLoadComplete.connect(self._on_container_load_complete)

    def _on_container_load_complete(self, container_id) -> None:
        pass
    def getPort(self):
        return 50680

    def isDebug(self):
        return True

    def start(self):
        pass
