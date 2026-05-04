from pathlib import Path

from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager


class ClangTidyProvider(ToolProvider):
    """Resolves clang-tidy. Prefers system, falls back to pip into venv."""

    def __init__(self, shell: Shell, venv_mgr: VenvManager):
        super().__init__(shell)
        self.venv_mgr = venv_mgr

    def ensure(self) -> Path:
        # system clang-tidy (e.g. LLVM install or VS "C++ Clang tools" component)
        found = self.shell.which("clang-tidy")
        if found:
            return Path(found)

        # venv clang-tidy
        venv_ct = self.venv_mgr.find_executable("clang-tidy")
        if venv_ct:
            return venv_ct

        # install into venv via pip
        print("clang-tidy not found. Installing into venv...")
        self.venv_mgr.install("clang-tidy")

        venv_ct = self.venv_mgr.find_executable("clang-tidy")
        if venv_ct:
            return venv_ct

        raise ToolNotFoundError("clang-tidy installation into venv failed")
