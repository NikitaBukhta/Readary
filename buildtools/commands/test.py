import os

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError
from buildtools.providers.cmake import CMakeProvider
from buildtools.shell import Shell


class TestCommand(Command):
    """Runs unit tests with the correct environment."""

    name = "test"
    summary = "Run unit tests"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 cmake: CMakeProvider):
        self.config = config
        self.shell = shell
        self.cmake = cmake

    def execute(self) -> None:
        if self.config.is_android:
            raise BuildError(
                "Tests are desktop-only; they are forced OFF on Android."
            )

        build_root = self.config.cmake_build_dir
        if not (build_root / "CTestTestfile.cmake").exists():
            raise BuildError(
                f"No CTest configuration in {build_root}. Run "
                "`python bootstrap.py compile"
                f"{' --release' if self.config.release else ''}` first."
            )

        ctest_path = self.cmake.ensure_ctest()

        env = os.environ.copy()
        env["PATH"] = str(self.config.qt_bin_dir) + os.pathsep + env.get("PATH", "")
        env["QT_PLUGIN_PATH"] = str(self.config.qt_plugin_dir)
        env["QT_QPA_PLATFORM"] = "offscreen"

        print(f"\n=== Running tests ({self.config.build_type}) ===")
        self.shell.run([
            ctest_path,
            "--test-dir", build_root,
            "--output-on-failure",
            "-C", self.config.build_type,
        ], env=env)

        print("\nAll tests passed.")
