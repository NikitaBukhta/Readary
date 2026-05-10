import os
import subprocess  # nosec B404 — invokes ctest from PATH

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError


class TestCommand(Command):
    """Runs unit tests with the correct environment."""

    name = "test"
    summary = "Run unit tests"

    def __init__(self, config: ProjectConfig):
        self.config = config

    def execute(self) -> None:
        build_root = self.config.project_dir / "build" / self.config.cmake_preset

        env = os.environ.copy()
        env["PATH"] = str(self.config.qt_bin_dir) + os.pathsep + env.get("PATH", "")
        env["QT_PLUGIN_PATH"] = str(self.config.qt_plugin_dir)
        env["QT_QPA_PLATFORM"] = "offscreen"

        print(f"\n=== Running tests ({self.config.build_type}) ===")
        result = subprocess.run(  # nosec B603 B607 — ctest is part of the CMake install we depend on
            [
                "ctest",
                "--test-dir", str(build_root),
                "--output-on-failure",
                "-C", self.config.build_type,
            ],
            env=env,
        )

        if result.returncode != 0:
            raise BuildError(f"Tests failed with exit code {result.returncode}")

        print("\nAll tests passed.")
