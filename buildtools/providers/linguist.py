from pathlib import Path

from buildtools.config import ProjectConfig
from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.shell import Shell


class LinguistProvider(ToolProvider):
    """Locates lupdate / lrelease — shipped with Qt6 (qttools) via vcpkg."""

    def __init__(self, shell: Shell, config: ProjectConfig):
        super().__init__(shell)
        self.config = config

    def ensure(self) -> Path:
        return self.lupdate()

    def lupdate(self) -> Path:
        return self._tool("lupdate")

    def lrelease(self) -> Path:
        return self._tool("lrelease")

    def _tool(self, name: str) -> Path:
        found = self.shell.which(name)
        if found:
            return Path(found)

        candidate = self.config.qt_tools_dir / self._executable_name(name)
        if candidate.exists():
            return candidate

        raise ToolNotFoundError(
            f"{name} not found. Expected via vcpkg Qt6 (qttools) install at "
            f"{candidate}. Add `qttools` to vcpkg.json and re-run "
            "`python bootstrap.py bootstrap`."
        )

    def _executable_name(self, name: str) -> str:
        return f"{name}.exe" if self.config.is_windows else name
