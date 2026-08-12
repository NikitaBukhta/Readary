import re
from pathlib import Path

from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager

# Oldest CMake we trust. The presets need version 6 (CMake >= 3.25); 3.25 is
# the floor. We also reject pre-releases (e.g. 4.1.0-rc1), which have broken
# the vcpkg qtdeclarative port build — see docs / git history.
_MIN_CMAKE = (3, 25)
# Pinned stable used when no acceptable system CMake is found.
_PINNED_CMAKE = "cmake==3.31.6"


class CMakeProvider(ToolProvider):

    def __init__(self, shell: Shell, venv_mgr: VenvManager):
        super().__init__(shell)
        self.venv_mgr = venv_mgr

    def ensure(self) -> Path:
        # System cmake — only if it's a stable release new enough for us.
        found = self.shell.which("cmake")
        if found and self._is_acceptable(Path(found)):
            return Path(found)
        if found:
            print(f"System cmake at {found} is a pre-release or too old; "
                  "preferring a pinned stable cmake in the venv.")

        # Already have an acceptable cmake in the venv.
        venv_cmake = self.venv_mgr.find_executable("cmake")
        if venv_cmake and self._is_acceptable(venv_cmake):
            return venv_cmake

        # Install the pinned stable cmake into the venv.
        print(f"Installing {_PINNED_CMAKE} into venv...")
        self.venv_mgr.install(_PINNED_CMAKE)
        venv_cmake = self.venv_mgr.find_executable("cmake")
        if venv_cmake and self._is_acceptable(venv_cmake):
            return venv_cmake

        raise ToolNotFoundError(
            "No acceptable CMake found. Install a stable CMake "
            f">= {'.'.join(map(str, _MIN_CMAKE))} (not a release candidate), "
            "or let the venv install one."
        )

    def ensure_ctest(self) -> Path:
        """Path to the ctest shipped alongside the cmake we resolved.

        ctest is not necessarily on PATH — when cmake comes from the venv,
        nothing exports venv/Scripts. Take the sibling of the resolved cmake.
        """
        cmake_path = self.ensure()
        ctest = cmake_path.with_name(f"ctest{cmake_path.suffix}")
        if ctest.exists():
            return ctest

        found = self.shell.which("ctest")
        if found:
            return Path(found)

        raise ToolNotFoundError(
            f"ctest not found next to {cmake_path} nor on PATH. Reinstall "
            "CMake, or run `python bootstrap.py bootstrap` to provision one "
            "into the venv."
        )

    def _is_acceptable(self, cmake: Path) -> bool:
        """True if `cmake` is a stable release at or above the minimum version."""
        line = self.shell.get_output([cmake, "--version"])
        if not line:
            return False
        # "cmake version 3.31.6" / "cmake version 4.1.0-rc1"
        match = re.search(r"version\s+(\d+)\.(\d+)\.(\d+)([\w.-]*)", line)
        if not match:
            return False
        major, minor = int(match.group(1)), int(match.group(2))
        suffix = match.group(4)
        # Reject pre-releases (rc/alpha/beta/dev): vcpkg ports break on them.
        if suffix and re.search(r"(rc|alpha|beta|dev)", suffix, re.IGNORECASE):
            return False
        return (major, minor) >= _MIN_CMAKE
