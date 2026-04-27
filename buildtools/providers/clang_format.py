import re
import subprocess
from pathlib import Path

from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager

# Base style for clang-format config generation
CLANG_FORMAT_BASE_STYLE = "LLVM"

# Project-specific overrides applied on top of the base style.
CLANG_FORMAT_OVERRIDES: dict[str, str] = {
    "ColumnLimit": "120",
}


class ClangFormatProvider(ToolProvider):

    def __init__(self, shell: Shell, venv_mgr: VenvManager,
                 project_dir: Path):
        super().__init__(shell)
        self.venv_mgr = venv_mgr
        self.project_dir = project_dir

    def ensure(self) -> Path:
        # system clang-format
        found = self.shell.which("clang-format")
        if found:
            return Path(found)

        # venv clang-format
        venv_cf = self.venv_mgr.find_executable("clang-format")
        if venv_cf:
            return venv_cf

        # install into venv
        print("clang-format not found. Installing into venv...")
        self.venv_mgr.install("clang-format")

        venv_cf = self.venv_mgr.find_executable("clang-format")
        if venv_cf:
            return venv_cf

        raise ToolNotFoundError("clang-format installation into venv failed")

    def ensure_config(self) -> Path:
        """Generate .clang-format in the project root if it doesn't exist."""
        config_path = self.project_dir / ".clang-format"
        if config_path.exists():
            return config_path

        cf_path = self.ensure()
        print(f"Generating .clang-format (base: {CLANG_FORMAT_BASE_STYLE})...")
        result = subprocess.run(
            [str(cf_path), f"-style={CLANG_FORMAT_BASE_STYLE}",
             "--dump-config"],
            capture_output=True, text=True, check=True,
        )
        content = self._apply_overrides(result.stdout, CLANG_FORMAT_OVERRIDES)
        config_path.write_text(content, encoding="utf-8")
        print(f"  Created {config_path}")
        return config_path

    @staticmethod
    def _apply_overrides(content: str, overrides: dict[str, str]) -> str:
        for key, value in overrides.items():
            pattern = rf"^({re.escape(key)}:\s*).*$"
            new_content, count = re.subn(
                pattern, rf"\g<1>{value}", content, count=1, flags=re.MULTILINE,
            )
            if count == 0:
                raise ValueError(
                    f"clang-format override key not found in dumped config: {key}"
                )
            content = new_content
        return content
