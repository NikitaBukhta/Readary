import os
import sys
from pathlib import Path

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError
from buildtools.providers.cmake import CMakeProvider
from buildtools.providers.msvc import MsvcProvider
from buildtools.shell import Shell


class TestCommand(Command):
    """Builds and runs the autotests."""

    name = "test"
    summary = "Build and run the autotests"

    # Where the buildtools' own unittest suite lives, relative to the project.
    _PYTHON_TESTS_DIR = "buildtools/tests"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 cmake: CMakeProvider, msvc: MsvcProvider):
        self.config = config
        self.shell = shell
        self.cmake = cmake
        self.msvc = msvc

    def execute(self) -> None:
        if self.config.is_android:
            raise BuildError(
                "Tests are desktop-only; they are forced OFF on Android."
            )

        if self.config.python_tests:
            self._run_python_tests()
            if self.config.test_python_only:
                return

        ctest_path = self.cmake.ensure_ctest()
        build_root = self._require_configured_build()

        if self.config.list_tests:
            self._list(ctest_path, build_root)
            return

        if not self.config.skip_test_build:
            self._build()

        self._run_ctest(ctest_path, build_root)

    # ---- steps ------------------------------------------------------------

    def _require_configured_build(self) -> Path:
        build_root = self.config.cmake_build_dir
        if not (build_root / "CTestTestfile.cmake").exists():
            raise BuildError(
                f"No CTest configuration in {build_root}. Run "
                "`python bootstrap.py compile"
                f"{' --release' if self.config.release else ''}` first."
            )
        return build_root

    def _build(self) -> None:
        """Bring the test executables up to date before running them.

        Builds the default target rather than the test targets one by one:
        every test links ReadaryCore, so the work is the same and the roster
        stays a CMake concern. Whatever ENABLE_ANALYZE the build tree was
        configured with still applies — this never reconfigures.
        """
        cmake_path = self.cmake.ensure()
        # MsvcProvider.env() is None off Windows, where the inherited
        # environment already has the compiler.
        env = self.msvc.env() or os.environ.copy()
        env["VCPKG_INSTALLED_DIR"] = self.config.deps_dir.as_posix()
        cmake_dir = str(Path(cmake_path).parent)
        env["PATH"] = f"{cmake_dir}{os.pathsep}{env.get('PATH', '')}"

        print(f"\n=== Building tests ({self.config.build_type}) ===")
        self.shell.run([
            cmake_path,
            "--build", "--preset", self.config.cmake_preset,
            "--config", self.config.build_type,
            "-j", str(self.config.jobs),
        ], env=env)

    def _list(self, ctest_path: Path, build_root: Path) -> None:
        print(f"\n=== Registered tests ({self.config.build_type}) ===")
        self.shell.run([
            ctest_path,
            "--test-dir", build_root,
            "-C", self.config.build_type,
            *self._filter_args(),
            "-N",
        ], env=self._test_env())

    def _run_ctest(self, ctest_path: Path, build_root: Path) -> None:
        label = f" matching '{self.config.test_filter}'" \
            if self.config.test_filter else ""
        print(f"\n=== Running tests ({self.config.build_type}){label} ===")
        self.shell.run([
            ctest_path,
            "--test-dir", build_root,
            "--output-on-failure",
            "-C", self.config.build_type,
            *self._filter_args(),
        ], env=self._test_env())

        print("\nAll tests passed.")

    def _run_python_tests(self) -> None:
        """Run the build system's own unittest suite.

        Opt-in rather than part of every run: these cover `buildtools/`, not
        the application, and they need no build tree at all.
        """
        python = self.config.venv_python
        if not python.exists():
            python = Path(sys.executable)

        print("\n=== Running buildtools tests ===")
        self.shell.run([
            python, "-m", "unittest", "discover",
            "-s", self._PYTHON_TESTS_DIR,
            "-t", ".",
            "-v",
        ])

    # ---- helpers ----------------------------------------------------------

    def _filter_args(self) -> list[str]:
        if not self.config.test_filter:
            return []
        return ["-R", self.config.test_filter]

    def _test_env(self) -> dict[str, str]:
        env = os.environ.copy()
        env["PATH"] = str(self.config.qt_bin_dir) + os.pathsep + env.get("PATH", "")
        env["QT_PLUGIN_PATH"] = str(self.config.qt_plugin_dir)
        env["QT_QPA_PLATFORM"] = "offscreen"
        return env
