from pathlib import Path

from buildtools.config import ProjectConfig
from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.shell import Shell


class QmlFormatProvider(ToolProvider):
    """Locates qmlformat — shipped with Qt6 via vcpkg.

    Unlike clang-format, there is nothing to pip-install: `vcpkg_port` names the
    port that carries the tool, and `BootstrapCommand` reinstalls it when the
    tool is missing.
    """

    # Ships every qml* host tool; vcpkg.json already depends on it.
    vcpkg_port = "qtdeclarative"

    def __init__(self, shell: Shell, config: ProjectConfig):
        super().__init__(shell)
        self.config = config

    def ensure(self) -> Path:
        found = self.locate()
        if found:
            return found

        raise ToolNotFoundError(
            f"qmlformat not found. Expected via vcpkg's {self.vcpkg_port} "
            f"install at {self.vcpkg_tool_path()}. "
            "Run `python bootstrap.py bootstrap` to restore it — the tool is "
            "not optional: Qt's own CMake package imports Qt6::qmlformat by "
            "that path, so the configure fails without it."
        )

    def locate(self) -> Path | None:
        """Path to any usable qmlformat, or None when there is none.

        Formatting takes whatever qmlformat is on PATH; only the configure
        insists on vcpkg's own copy — see `vcpkg_tool_path`.
        """
        found = self.shell.which("qmlformat")
        if found:
            return Path(found)

        candidate = self.vcpkg_tool_path()
        return candidate if candidate.exists() else None

    def vcpkg_tool_path(self) -> Path:
        """Where vcpkg's Qt install has to carry the tool.

        Qt6QmlToolsTargets.cmake imports Qt6::qmlformat by exactly this absolute
        path, so another Qt on PATH does not stand in for it — which is why the
        repair gate asks about this path and not about `locate`.
        """
        return self.config.qt_tools_dir / self._executable_name()

    def _executable_name(self) -> str:
        return "qmlformat.exe" if self.config.is_windows else "qmlformat"
