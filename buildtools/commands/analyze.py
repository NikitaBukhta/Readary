import concurrent.futures
import subprocess  # nosec B404 — fans out clang-tidy invocations over the source tree
from pathlib import Path

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError
from buildtools.providers.clang_tidy import ClangTidyProvider
from buildtools.shell import Shell


class AnalyzeCommand(Command):
    """Runs clang-tidy over project sources using compile_commands.json."""

    name = "analyze"
    summary = "Run clang-tidy over the project (no compile)"

    _SOURCE_EXTS = (".cpp", ".cxx", ".cc")

    def __init__(self, config: ProjectConfig, shell: Shell,
                 clang_tidy: ClangTidyProvider):
        self.config = config
        self.shell = shell
        self.clang_tidy = clang_tidy

    def execute(self) -> None:
        build_dir = self.config.cmake_build_dir
        compile_db = build_dir / "compile_commands.json"
        if not compile_db.exists():
            raise BuildError(
                f"compile_commands.json not found at {compile_db}. "
                "Run `python bootstrap.py bootstrap` first."
            )

        sources = self._collect_sources()
        if not sources:
            print("No C++ source files to analyze.")
            return

        ct_path = self.clang_tidy.ensure()
        print(f"\n=== Analyzing {len(sources)} file(s) "
              f"with clang-tidy ===")
        print(f"  clang-tidy : {ct_path}")
        print(f"  build dir  : {build_dir}\n")

        failures = self._run_parallel(ct_path, build_dir, sources)
        if failures:
            print(f"\nclang-tidy reported issues in {failures} file(s).")
            raise BuildError("Static analysis failed.")

        print("\nAnalysis complete — no issues found.")

    def _collect_sources(self) -> list[Path]:
        src_root = self.config.project_dir / "src"
        if not src_root.exists():
            return []
        files: list[Path] = []
        for ext in self._SOURCE_EXTS:
            files.extend(src_root.rglob(f"*{ext}"))
        return sorted(files)

    def _run_parallel(self, ct_path: Path, build_dir: Path,
                      sources: list[Path]) -> int:
        cmd_template = [str(ct_path), "-p", str(build_dir), "--quiet"]
        failures = 0
        with concurrent.futures.ThreadPoolExecutor(
            max_workers=self.config.jobs,
        ) as pool:
            futures = {
                pool.submit(self._run_one, cmd_template, src): src
                for src in sources
            }
            for fut in concurrent.futures.as_completed(futures):
                src = futures[fut]
                ok, output = fut.result()
                rel = src.relative_to(self.config.project_dir)
                if ok and not output.strip():
                    continue
                if ok:
                    print(f"--- {rel}")
                    print(output)
                    continue
                failures += 1
                print(f"!!! {rel}")
                print(output)
        return failures

    @staticmethod
    def _run_one(cmd_template: list[str],
                 source: Path) -> tuple[bool, str]:
        result = subprocess.run(  # nosec B603 — cmd_template[0] is the resolved clang-tidy binary
            [*cmd_template, str(source)],
            capture_output=True, text=True,
        )
        combined = result.stdout + result.stderr
        return result.returncode == 0, combined
