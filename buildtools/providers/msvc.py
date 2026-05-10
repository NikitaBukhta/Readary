import os
import subprocess  # nosec B404 — locates and probes the system MSVC toolchain
import tempfile
from pathlib import Path

from buildtools.config import ProjectConfig
from buildtools.errors import ToolNotFoundError
from buildtools.shell import Shell


class MsvcProvider:
    """Resolves the MSVC build environment (vcvars64) and Ninja for builds.

    On non-Windows platforms this is a no-op: ``env()`` returns ``None`` and
    callers should fall back to the inherited process environment.
    """

    _VSWHERE = Path(
        "C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
    )
    _NINJA_REL = Path(
        "Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
    )
    _SENTINEL = "__VCVARS_ENV_DUMP__"

    def __init__(self, shell: Shell, config: ProjectConfig):
        self.shell = shell
        self.config = config
        self._cached: dict[str, str] | None = None

    def env(self) -> dict[str, str] | None:
        if not self.config.is_windows:
            return None
        if self._cached is not None:
            return self._cached
        self._cached = self._capture()
        return self._cached

    def _capture(self) -> dict[str, str]:
        install = self._find_vs_install()
        vcvars = install / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
        if not vcvars.exists():
            raise ToolNotFoundError(f"vcvars64.bat not found at {vcvars}")

        # Drive vcvars from a temp .bat file: passing the call inline as
        # `cmd /c` argument mangles backslash sequences inside the quoted path.
        script = (
            "@echo off\r\n"
            f'call "{vcvars}" x64 >NUL\r\n'
            f"if errorlevel 1 exit /b 1\r\n"
            f"echo {self._SENTINEL}\r\n"
            "set\r\n"
        )
        with tempfile.NamedTemporaryFile(
            "w", suffix=".bat", delete=False, encoding="utf-8",
        ) as f:
            f.write(script)
            bat_path = Path(f.name)
        try:
            result = subprocess.run(  # nosec B603 B607 — cmd /c on a tempfile we just wrote
                ["cmd", "/c", str(bat_path)],
                capture_output=True, text=True, check=True,
            )
        finally:
            bat_path.unlink(missing_ok=True)

        lines = result.stdout.splitlines()
        try:
            start = lines.index(self._SENTINEL) + 1
        except ValueError:
            raise ToolNotFoundError(
                "Failed to capture MSVC environment (sentinel missing). "
                f"vcvars output:\n{result.stdout[-500:]}"
            )
        env: dict[str, str] = {}
        for line in lines[start:]:
            key, sep, value = line.partition("=")
            if sep and key:
                env[key] = value
        if "INCLUDE" not in env or "LIB" not in env:
            raise ToolNotFoundError(
                f"vcvars64 did not populate INCLUDE/LIB (got {len(env)} vars)"
            )

        ninja = install / self._NINJA_REL
        if ninja.exists():
            path_key = next((k for k in env if k.upper() == "PATH"), "PATH")
            env[path_key] = f"{ninja.parent};{env.get(path_key, '')}"

        # vcvars64 hijacks VCPKG_ROOT to point at VS's bundled vcpkg, which
        # contends for a filesystem lock with our project's vcpkg. Preserve
        # the value the caller set in os.environ (typically VcpkgProvider).
        parent_vcpkg_root = os.environ.get("VCPKG_ROOT")
        if parent_vcpkg_root:
            env["VCPKG_ROOT"] = parent_vcpkg_root

        return env

    def _find_vs_install(self) -> Path:
        if not self._VSWHERE.exists():
            raise ToolNotFoundError(
                f"vswhere.exe not found at {self._VSWHERE}. "
                "Install Visual Studio 2017+ or the Build Tools."
            )
        result = subprocess.run(  # nosec B603 — vswhere.exe is at a fixed system path, args are constants
            [
                str(self._VSWHERE),
                "-latest", "-products", "*",
                "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                "-property", "installationPath",
            ],
            capture_output=True, text=True, check=True,
        )
        lines = result.stdout.strip().splitlines()
        if not lines:
            raise ToolNotFoundError(
                "No Visual Studio installation with the C++ x64 toolchain "
                "was found. Install MSVC via the Visual Studio Installer."
            )
        return Path(lines[0])
