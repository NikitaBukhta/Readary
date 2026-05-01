from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.providers.clang_format import ClangFormatProvider
from buildtools.providers.cmake import CMakeProvider
from buildtools.providers.msvc import MsvcProvider
from buildtools.providers.qml_format import QmlFormatProvider
from buildtools.providers.vcpkg import VcpkgProvider
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager


class BootstrapCommand(Command):
    """Downloads tools, installs vcpkg dependencies, configures the project."""

    name = "bootstrap"
    summary = "Install dependencies and configure the project"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 venv_mgr: VenvManager, cmake: CMakeProvider,
                 vcpkg: VcpkgProvider, msvc: MsvcProvider,
                 clang_format: ClangFormatProvider,
                 qml_format: QmlFormatProvider):
        self.config = config
        self.shell = shell
        self.venv_mgr = venv_mgr
        self.cmake = cmake
        self.vcpkg = vcpkg
        self.msvc = msvc
        self.clang_format = clang_format
        self.qml_format = qml_format

    def execute(self) -> None:
        self.venv_mgr.ensure()
        cmake_path = self.cmake.ensure()
        self.vcpkg.ensure()
        self.clang_format.ensure_config()
        env = self.msvc.env()

        print(f"\n=== Configuring ({self.config.build_type}) ===")
        cmake_cmd = [
            cmake_path,
            "--preset", self.config.cmake_preset,
            "-S", self.config.project_dir,
        ]
        for d in self.config.cmake_defs:
            cmake_cmd.append(f"-D{d}")
        self.shell.run(cmake_cmd, env=env)

        # qmlformat ships with Qt6 — only resolvable after vcpkg installs Qt
        # during the cmake configure step above.
        qmlformat_path = self.qml_format.ensure()
        print(f"  qmlformat    : {qmlformat_path}")

        print("\nBootstrap complete.")
        print(f"  Dependencies : {self.config.deps_dir}")
        print(f"  Virtual env  : {self.config.venv_dir}")
        flag = " --release" if self.config.release else ""
        print(f"Run `python bootstrap.py compile{flag}` to build the project.")
