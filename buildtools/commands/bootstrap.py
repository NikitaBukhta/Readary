from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.providers.clang_format import ClangFormatProvider
from buildtools.providers.clang_tidy import ClangTidyProvider
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
                 clang_tidy: ClangTidyProvider,
                 qml_format: QmlFormatProvider):
        self.config = config
        self.shell = shell
        self.venv_mgr = venv_mgr
        self.cmake = cmake
        self.vcpkg = vcpkg
        self.msvc = msvc
        self.clang_format = clang_format
        self.clang_tidy = clang_tidy
        self.qml_format = qml_format

    def execute(self) -> None:
        self.venv_mgr.ensure()
        cmake_path = self.cmake.ensure()
        self.vcpkg.ensure()
        self.clang_format.ensure_config()
        clang_tidy_path = self.clang_tidy.ensure()
        env = self.msvc.env()

        wrapper_script = (self.config.project_dir / "buildtools"
                          / "clang_tidy_wrapper.py")
        print(f"\n=== Configuring ({self.config.build_type}) ===")
        cmake_cmd = [
            cmake_path,
            "--preset", self.config.cmake_preset,
            "-S", self.config.project_dir,
            f"-DCLANG_TIDY_EXECUTABLE={clang_tidy_path}",
            f"-DCLANG_TIDY_WRAPPER_PYTHON={self.config.venv_python}",
            f"-DCLANG_TIDY_WRAPPER_SCRIPT={wrapper_script}",
            "-DENABLE_ANALYZE=ON",
        ]
        for d in self.config.cmake_defs:
            cmake_cmd.append(f"-D{d}")
        self.shell.run(cmake_cmd, env=env)

        # qmlformat ships with Qt6 — only resolvable after vcpkg installs Qt
        # during the cmake configure step above.
        qmlformat_path = self.qml_format.ensure()
        print(f"  qmlformat    : {qmlformat_path}")
        print(f"  clang-tidy   : {clang_tidy_path}")

        print("\nBootstrap complete.")
        print(f"  Dependencies : {self.config.deps_dir}")
        print(f"  Virtual env  : {self.config.venv_dir}")
        flag = " --release" if self.config.release else ""
        print(f"Run `python bootstrap.py compile{flag}` to build the project.")
