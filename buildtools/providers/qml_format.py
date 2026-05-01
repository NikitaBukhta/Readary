from pathlib import Path

from buildtools.config import ProjectConfig
from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.shell import Shell


class QmlFormatProvider(ToolProvider):
    """Locates qmlformat — shipped with Qt6 via vcpkg."""

    def __init__(self, shell: Shell, config: ProjectConfig):
        super().__init__(shell)
        self.config = config

    def ensure(self) -> Path:
        # system qmlformat
        found = self.shell.which("qmlformat")
        if found:
            return Path(found)

        # vcpkg-installed Qt6 tools
        candidate = self.config.qt_tools_dir / self._executable_name()
        if candidate.exists():
            return candidate

        raise ToolNotFoundError(
            f"qmlformat not found. Expected via vcpkg Qt6 install at "
            f"{candidate}. Run `python bootstrap.py bootstrap` first."
        )

    def _executable_name(self) -> str:
        return "qmlformat.exe" if self.config.is_windows else "qmlformat"
