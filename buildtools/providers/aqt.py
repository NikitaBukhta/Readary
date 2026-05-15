from pathlib import Path

from buildtools.config import (
    ANDROID_QT_VERSION,
    ProjectConfig,
    aqt_arch_for_abi,
)
from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager


class AqtProvider(ToolProvider):
    """Installs host Qt (for moc/rcc/qmltyperegistrar) and per-ABI Android Qt via aqtinstall."""

    # Qt 6.8 bundles qtdeclarative in both base archives; passing it explicitly errors out.
    _MODULES_BY_TARGET: dict[str, list[str]] = {
        "desktop": [],
        "android": [],
    }

    def __init__(self, shell: Shell, config: ProjectConfig,
                 venv_mgr: VenvManager):
        super().__init__(shell)
        self.config = config
        self.venv_mgr = venv_mgr

    def ensure(self) -> Path:
        """Install host + per-ABI Android Qt; returns the primary ABI's prefix."""
        self._ensure_aqt()

        host_dir = self._ensure_host_qt()
        android_dirs = [self._ensure_android_qt(abi)
                        for abi in self.config.android_abis]

        missing = [d for d in (host_dir, *android_dirs) if not d.exists()]
        if missing:
            raise ToolNotFoundError(
                "aqt install completed but the expected Qt prefixes are "
                "missing:\n  " + "\n  ".join(str(p) for p in missing)
            )
        return android_dirs[0]

    def host_qt_dir(self) -> Path:
        return self.config.qt_host_dir

    def android_qt_dir(self, abi: str | None = None) -> Path:
        if abi is None:
            return self.config.qt_android_dir
        return self.config.qt_android_dir_for(abi)

    def android_qt_dirs(self) -> dict[str, Path]:
        return {abi: self.config.qt_android_dir_for(abi)
                for abi in self.config.android_abis}

    # ---- internals --------------------------------------------------------

    def _ensure_aqt(self) -> None:
        if self.venv_mgr.find_executable("aqt"):
            return
        print("Installing aqtinstall into venv...")
        self.venv_mgr.install("aqtinstall")
        if not self.venv_mgr.find_executable("aqt"):
            raise ToolNotFoundError("aqtinstall installation into venv failed")

    def _ensure_host_qt(self) -> Path:
        target_dir = self.config.qt_host_dir
        if self._looks_installed(target_dir):
            return target_dir

        print(f"Installing host Qt {ANDROID_QT_VERSION} "
              f"({self.config.aqt_host_arch}) via aqt...")
        self._run_aqt_install(
            "desktop", self.config.aqt_host_arch, self.config.aqt_qt_root,
        )
        return target_dir

    def _ensure_android_qt(self, abi: str) -> Path:
        target_dir = self.config.qt_android_dir_for(abi)
        if self._looks_installed(target_dir):
            return target_dir

        arch_token = aqt_arch_for_abi(abi)
        print(f"Installing Android Qt {ANDROID_QT_VERSION} ({arch_token}) "
              "via aqt...")
        self._run_aqt_install(
            "android", arch_token, self.config.aqt_qt_root,
        )
        return target_dir

    def _run_aqt_install(self, target: str, arch: str,
                         outputdir: Path) -> None:
        outputdir.mkdir(parents=True, exist_ok=True)
        aqt = self.venv_mgr.find_executable("aqt")
        if not aqt:
            raise ToolNotFoundError("aqt executable missing from venv")
        cmd: list = [
            aqt, "install-qt",
            self.config.aqt_host, target, ANDROID_QT_VERSION, arch,
            "--outputdir", outputdir,
        ]
        modules = self._MODULES_BY_TARGET.get(target, [])
        if modules:
            cmd.extend(["--modules", *modules])
        self.shell.run(cmd)

    def _looks_installed(self, qt_prefix: Path) -> bool:
        # Presence of Qt6Config.cmake is the cheapest "Qt is installed here" probe.
        return (qt_prefix / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake").exists()
