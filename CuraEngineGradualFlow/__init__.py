# Copyright (c) 2023 UltiMaker
# Cura is released under the terms of the LGPLv3 or higher.

import platform
from UM.i18n import i18nCatalog


catalog = i18nCatalog("cura")

if platform.machine() in ["AMD64", "x86_64"]:
    from . import GradualFlowPlugin


    def getMetaData():
        return {}

    def register(app):
        return { "backend_plugin":  GradualFlowPlugin.GradualFlowPlugin() }
else:
    from UM.Logger import Logger

    Logger.error("CuraEngineGradualFlow plugin is only supported on x86_64 systems")

    def getMetaData():
        return {}

    def register(app):
        return {}
