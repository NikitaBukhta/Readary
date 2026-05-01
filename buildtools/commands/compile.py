from buildtools.commands.base import Command
from buildtools.commands.format import FormatCommand
from buildtools.config import ProjectConfig
from buildtools.providers.clang_format import ClangFormatProvider
from buildtools.providers.cmake import CMakeProvider
from buildtools.providers.msvc import MsvcProvider
from buildtools.providers.qml_format import QmlFormatProvider
from buildtools.shell import Shell


class CompileCommand(Command):
    """Builds the project (must run bootstrap first)."""

    name = "compile"
    summary = "Build the project"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 cmake: CMakeProvider, msvc: MsvcProvider,
                 clang_format: ClangFormatProvider,
                 qml_format: QmlFormatProvider):
        self.config = config
        self.shell = shell
        self.cmake = cmake
        self.msvc = msvc
        self.clang_format = clang_format
        self.qml_format = qml_format

    def execute(self) -> None:
        FormatCommand(self.config, self.shell, self.clang_format,
                      self.qml_format).execute()

        cmake_path = self.cmake.ensure()
        env = self.msvc.env()

        print(f"\n=== Building ({self.config.build_type}) ===")
        self.shell.run([
            cmake_path,
            "--build", "--preset", self.config.cmake_preset,
            "--config", self.config.build_type,
            "-j", str(self.config.jobs),
        ], env=env)

        print("\nBuild complete.")
