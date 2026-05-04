from pathlib import Path

from buildtools.commands.base import Command
from buildtools.commands.format import FormatCommand
from buildtools.config import ProjectConfig
from buildtools.providers.clang_format import ClangFormatProvider
from buildtools.providers.clang_tidy import ClangTidyProvider
from buildtools.providers.cmake import CMakeProvider
from buildtools.providers.msvc import MsvcProvider
from buildtools.providers.qml_format import QmlFormatProvider
from buildtools.shell import Shell


class CompileCommand(Command):
    """Builds the project (must run bootstrap first)."""

    name = "compile"
    summary = "Build the project (with static analysis by default)"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 cmake: CMakeProvider, msvc: MsvcProvider,
                 clang_format: ClangFormatProvider,
                 clang_tidy: ClangTidyProvider,
                 qml_format: QmlFormatProvider):
        self.config = config
        self.shell = shell
        self.cmake = cmake
        self.msvc = msvc
        self.clang_format = clang_format
        self.clang_tidy = clang_tidy
        self.qml_format = qml_format

    def execute(self) -> None:
        FormatCommand(self.config, self.shell, self.clang_format,
                      self.qml_format).execute()

        cmake_path = self.cmake.ensure()
        env = self.msvc.env()

        self._sync_analyze_setting(cmake_path, env)

        analyze_label = "OFF — gate skipped" if self.config.skip_analyze \
            else "ON — clang-tidy + MSVC /analyze"
        print(f"\n=== Building ({self.config.build_type}) ===")
        print(f"  Static analysis: {analyze_label}")
        self.shell.run([
            cmake_path,
            "--build", "--preset", self.config.cmake_preset,
            "--config", self.config.build_type,
            "-j", str(self.config.jobs),
        ], env=env)

        print("\nBuild complete.")

    def _sync_analyze_setting(self, cmake_path: Path,
                              env: dict[str, str] | None) -> None:
        """Reconfigure CMake if ENABLE_ANALYZE differs from desired value."""
        desired = "OFF" if self.config.skip_analyze else "ON"
        current = self._read_cache_var("ENABLE_ANALYZE")
        if current == desired:
            return

        print(f"\n=== Reconfiguring (ENABLE_ANALYZE={desired}) ===")
        cmake_cmd: list[str | Path] = [
            cmake_path,
            "--preset", self.config.cmake_preset,
            "-S", self.config.project_dir,
            f"-DENABLE_ANALYZE={desired}",
        ]
        if desired == "ON":
            # Refresh clang-tidy path + wrapper in case the venv was wiped
            # since bootstrap.
            ct_path = self.clang_tidy.ensure()
            wrapper_script = (self.config.project_dir / "buildtools"
                              / "clang_tidy_wrapper.py")
            cmake_cmd.append(f"-DCLANG_TIDY_EXECUTABLE={ct_path}")
            cmake_cmd.append(
                f"-DCLANG_TIDY_WRAPPER_PYTHON={self.config.venv_python}")
            cmake_cmd.append(f"-DCLANG_TIDY_WRAPPER_SCRIPT={wrapper_script}")
        self.shell.run(cmake_cmd, env=env)

    def _read_cache_var(self, name: str) -> str | None:
        cache = (self.config.project_dir / "build"
                 / self.config.cmake_preset / "CMakeCache.txt")
        if not cache.exists():
            return None
        prefix = f"{name}:"
        try:
            for line in cache.read_text(encoding="utf-8").splitlines():
                if line.startswith(prefix):
                    _, _, value = line.partition("=")
                    return self._normalize_bool(value.strip())
        except OSError:
            return None
        return None

    @staticmethod
    def _normalize_bool(value: str) -> str:
        truthy = {"1", "ON", "TRUE", "YES", "Y"}
        falsy = {"0", "OFF", "FALSE", "NO", "N", ""}
        upper = value.upper()
        if upper in truthy:
            return "ON"
        if upper in falsy:
            return "OFF"
        return value
