from pathlib import Path

from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager


class NinjaProvider(ToolProvider):
    """Resolves ``ninja`` for Android builds. Order: system PATH, venv, then pip-install."""

    def __init__(self, shell: Shell, venv_mgr: VenvManager):
        super().__init__(shell)
        self.venv_mgr = venv_mgr

    def ensure(self) -> Path:
        found = self.shell.which("ninja")
        if found:
            return Path(found)

        venv_ninja = self.venv_mgr.find_executable("ninja")
        if venv_ninja:
            return venv_ninja

        print("Ninja not found. Installing into venv...")
        self.venv_mgr.install("ninja")

        venv_ninja = self.venv_mgr.find_executable("ninja")
        if venv_ninja:
            return venv_ninja

        raise ToolNotFoundError("ninja installation into venv failed")
