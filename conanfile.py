import os
from pathlib import Path

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualBuildEnv
from conan.tools.files import copy
from conan.tools.microsoft import check_min_vs, is_msvc_static_runtime, is_msvc
from conan.tools.scm import Version

from jinja2 import Template

required_conan_version = ">=1.53.0"


class CuraEngineGradualFlowPluginConan(ConanFile):
    name = "curaengine_plugin_gradual_flow"
    description = "CuraEngine plugin for gradually smoothing the flow to limit high-flow jumps"
    author = "UltiMaker"
    license = "agpl-3.0"
    url = "https://github.com/Ultimaker/CuraEngine_plugin_gradual_flow"
    homepage = "https://ultimaker.com"
    topics = ("protobuf", "asio", "plugin", "curaengine", "gcode-generation", "3D-printing")
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "enable_testing": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "enable_testing": False,
    }

    def set_version(self):
        if not self.version:
            self.version = "0.1.0-alpha"

    @property
    def _min_cppstd(self):
        return 20

    @property
    def _compilers_minimum_version(self):
        return {
            "gcc": "11",
            "clang": "14",
            "apple-clang": "13",
            "msvc": "192",
            "visual_studio": "17",
        }

    @property
    def _cura_plugin_name(self):
        return "CuraEngineGradualFlow"

    @property
    def _api_version(self):
        return "8"

    @property
    def _sdk_versions(self):
        return ["8.3.0"]

    def _generate_cmdline(self):
        with open(os.path.join(self.recipe_folder, "templates", "include", "plugin", "cmdline.h.jinja"), "r") as f:
            template = Template(f.read())

        version = Version(self.version)
        with open(os.path.join(self.recipe_folder, "include", "plugin", "cmdline.h"), "w") as f:
            f.write(template.render(cura_plugin_name=self._cura_plugin_name,
                                    version=f"{version.major}.{version.minor}.{version.patch}",
                                    curaengine_plugin_name=self.name))

    def _generate_cura_plugin_constants(self):
        with open(os.path.join(self.recipe_folder, "templates", "cura_plugin", "constants.py.jinja"), "r") as f:
            template = Template(f.read())

        version = Version(self.version)
        with open(os.path.join(self.recipe_folder, self._cura_plugin_name, "constants.py"), "w") as f:
            f.write(template.render(cura_plugin_name=self._cura_plugin_name,
                                    version=f"{version.major}.{version.minor}.{version.patch}",
                                    curaengine_plugin_name=self.name,
                                    settings_prefix=f"_plugin__{self._cura_plugin_name.lower()}__{version.major}_{version.minor}_{version.patch}_"))

    def _generate_plugin_metadata(self):
        with open(os.path.join(self.recipe_folder, "templates", "cura_plugin", "plugin.json.jinja"), "r") as f:
            template = Template(f.read())

        with open(os.path.join(self.recipe_folder, self._cura_plugin_name, "plugin.json"), "w") as f:
            f.write(template.render(cura_plugin_name=self._cura_plugin_name,
                                    author=self.author,
                                    version=self.version,
                                    description=self.description,
                                    api_version=self._api_version,
                                    sdk_versions=self._sdk_versions))

    def export_sources(self):
        copy(self, "CMakeLists.txt", self.recipe_folder, self.export_sources_folder)
        copy(self, "*.jinja", os.path.join(self.recipe_folder, "templates"), os.path.join(self.export_sources_folder, "templates"))
        copy(self, "*", os.path.join(self.recipe_folder, "src"), os.path.join(self.export_sources_folder, "src"))
        copy(self, "*", os.path.join(self.recipe_folder, "include"), os.path.join(self.export_sources_folder, "include"))
        copy(self, "*", os.path.join(self.recipe_folder, "tests"), os.path.join(self.export_sources_folder, "tests"))

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        self.options["boost"].header_only = True

        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires("protobuf/3.21.9")
        self.requires("boost/1.82.0")
        self.requires("asio-grpc/2.6.0")
        self.requires("openssl/1.1.1l")
        self.requires("spdlog/1.11.0")
        self.requires("docopt.cpp/0.6.3")
        self.requires("range-v3/0.12.0")
        self.requires("clipper/6.4.2")
        self.requires("ctre/3.7.2")
        self.requires("neargye-semver/0.3.0")
        self.requires("curaengine_grpc_definitions/latest@ultimaker/cura_10446")

    def build_requirements(self):
        if self.options.enable_testing:
            self.test_requires("catch2/[>=2.13.8]")

    def validate(self):
        # validate the minimum cpp standard supported. For C++ projects only
        if self.settings.compiler.cppstd:
            check_min_cppstd(self, self._min_cppstd)
        check_min_vs(self, 191)
        if not is_msvc(self):
            minimum_version = self._compilers_minimum_version.get(str(self.settings.compiler), False)
            if minimum_version and Version(self.settings.compiler.version) < minimum_version:
                raise ConanInvalidConfiguration(
                    f"{self.ref} requires C++{self._min_cppstd}, which your compiler does not support."
                )
        if is_msvc(self) and self.options.shared:
            raise ConanInvalidConfiguration(f"{self.ref} can not be built as shared on Visual Studio and msvc.")

    def generate(self):
        self._generate_cmdline()
        self._generate_cura_plugin_constants()
        self._generate_plugin_metadata()

        # BUILD_SHARED_LIBS and POSITION_INDEPENDENT_CODE are automatically parsed when self.options.shared or self.options.fPIC exist
        tc = CMakeToolchain(self)
        tc.variables["ENABLE_TESTS"] = self.options.enable_testing
        # Boolean values are preferred instead of "ON"/"OFF"
        if is_msvc(self):
            tc.variables["USE_MSVC_RUNTIME_LIBRARY_DLL"] = not is_msvc_static_runtime(self)
        tc.cache_variables["CMAKE_POLICY_DEFAULT_CMP0077"] = "NEW"
        tc.generate()

        tc = CMakeDeps(self)
        tc.generate()

        tc = VirtualBuildEnv(self)
        tc.generate(scope="build")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, pattern="LICENSE", dst=os.path.join(self.package_folder, "licenses"), src=self.source_folder)
        ext = ".exe" if self.settings.os == "Windows" else ""
        copy(self, pattern=f"curaengine_plugin_gradual_flow{ext}", dst="bin", src=os.path.join(self.build_folder))
