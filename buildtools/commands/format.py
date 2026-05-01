from pathlib import Path

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.providers.clang_format import ClangFormatProvider
from buildtools.providers.qml_format import QmlFormatProvider
from buildtools.shell import Shell


class FormatCommand(Command):
    """Formats C++ sources with clang-format and QML sources with qmlformat."""

    name = "format"
    summary = "Auto-format C++ and QML source files"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 clang_format: ClangFormatProvider,
                 qml_format: QmlFormatProvider):
        self.config = config
        self.shell = shell
        self.clang_format = clang_format
        self.qml_format = qml_format

    def execute(self) -> None:
        self._format_cpp()
        self._format_qml()
        print("Formatting complete.")

    def _format_cpp(self) -> None:
        sources = self._collect(self.config.project_dir / "src",
                                (".cpp", ".hpp", ".h", ".cxx", ".cc"))
        if not sources:
            print("No C++ source files found.")
            return

        cf_path = self.clang_format.ensure()
        print(f"\n=== Formatting {len(sources)} C++ file(s) ===")
        self.shell.run([cf_path, "-i", *sources])

    def _format_qml(self) -> None:
        sources = self._collect(self.config.project_dir / "qml", (".qml",))
        if not sources:
            print("No QML source files found.")
            return

        qf_path = self.qml_format.ensure()
        print(f"\n=== Formatting {len(sources)} QML file(s) ===")
        self.shell.run([qf_path, "--inplace", *sources])

    @staticmethod
    def _collect(root: Path, extensions: tuple[str, ...]) -> list[Path]:
        if not root.exists():
            return []
        files: list[Path] = []
        for ext in extensions:
            files.extend(root.rglob(f"*{ext}"))
        return sorted(files)
