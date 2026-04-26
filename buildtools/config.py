import os
import platform
from dataclasses import dataclass, field
from pathlib import Path


def _is_windows() -> bool:
    return platform.system() == "Windows"


@dataclass(frozen=True)
class ProjectConfig:
    """Project-wide build configuration."""

    project_dir: Path
    release: bool = False
    jobs: int = field(default_factory=lambda: os.cpu_count() or 1)
    cmake_defs: list[str] = field(default_factory=list)

    venv_dir: Path = field(init=False)
    deps_dir: Path = field(init=False)
    vcpkg_dir: Path = field(init=False)
    is_windows: bool = field(init=False)

    def __post_init__(self):
        object.__setattr__(self, "venv_dir", self.project_dir / "venv")
        object.__setattr__(self, "deps_dir", Path.home() / "BeeLibrary-dependencies")
        object.__setattr__(self, "vcpkg_dir", Path.home() / "vcpkg")
        object.__setattr__(self, "is_windows", _is_windows())

    @property
    def build_type(self) -> str:
        return "Release" if self.release else "Debug"

    @property
    def cmake_preset(self) -> str:
        return self.build_type.lower()

    @property
    def _venv_scripts_dir(self) -> Path:
        if self.is_windows:
            return self.venv_dir / "Scripts"
        return self.venv_dir / "bin"

    @property
    def venv_python(self) -> Path:
        return self.venv_executable("python")

    def venv_executable(self, name: str) -> Path:
        """Return the expected path for an executable inside venv."""
        if self.is_windows:
            return self._venv_scripts_dir / f"{name}.exe"
        return self._venv_scripts_dir / name

    @property
    def _vcpkg_triplet_dir(self) -> Path:
        base = self.deps_dir / "x64-windows"
        if self.release:
            return base
        return base / "debug"

    @property
    def qt_plugin_dir(self) -> Path:
        return self._vcpkg_triplet_dir / "Qt6" / "plugins"

    @property
    def qt_bin_dir(self) -> Path:
        return self._vcpkg_triplet_dir / "bin"

    @property
    def qt_qml_dir(self) -> Path:
        return self._vcpkg_triplet_dir / "Qt6" / "qml"

    def vcpkg_executable(self) -> Path:
        """Return the expected path for the vcpkg binary."""
        exe = "vcpkg.exe" if self.is_windows else "vcpkg"
        return self.vcpkg_dir / exe
