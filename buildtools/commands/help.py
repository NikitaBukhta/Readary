from pathlib import Path

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.shell import Shell


class StatusChecker:
    """Inspects the current state of tools and project directories."""

    def __init__(self, config: ProjectConfig, shell: Shell):
        self.config = config
        self.shell = shell

    def check_tool(self, name: str,
                   extra_paths: list[Path] | None = None) -> tuple[bool, str]:
        found = self.shell.which(name)
        if found:
            return True, found
        for p in (extra_paths or []):
            if p.exists():
                return True, str(p)
        return False, "not found"

    def check_dir(self, path: Path) -> tuple[bool, str]:
        if path.exists():
            return True, str(path)
        return False, "not created"

    def get_tool_version(self, executable: str) -> str:
        version = self.shell.get_output([executable, "--version"])
        return version or "unknown version"


class HelpCommand(Command):
    """Shows an overview of commands, paths, and tool status."""

    name = "help"
    summary = "Show this help message"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 commands: dict[str, Command]):
        self.config = config
        self.checker = StatusChecker(config, shell)
        self.commands = commands

    def execute(self) -> None:
        self._print_header()
        self._print_commands()
        self._print_usage()
        self._print_paths()
        self._print_tools()

    def _print_header(self) -> None:
        print("Library Project — bootstrap & build helper")
        print("=" * 50)

    def _print_commands(self) -> None:
        print("\nCommands:")
        for cmd in self.commands.values():
            print(f"  {cmd.name:<12} {cmd.summary}")

    def _print_usage(self) -> None:
        print("\nUsage:")
        print("  python bootstrap.py <command> [flags]")

        print("\nFlags:")
        print("  --release        Use Release mode (default: Debug)")
        print("                   Applies to: bootstrap, compile, run, analyze")
        print("  -d, --device T   Target device. T = windows | android")
        print("                   Default: host platform")
        print("                   Applies to: bootstrap, compile, run")
        print("  --abi A [A...]   Android ABI(s) to build for (with -d android)")
        print("                   Choices: arm64-v8a armeabi-v7a x86_64 x86 all")
        print("                   Default: arm64-v8a")
        print("                   `all` -> every supported ABI (universal APK)")
        print("                   Multi-ABI: --abi arm64-v8a x86_64")
        print("                   Applies to: bootstrap, compile, run")
        print("  -j, --jobs N     Limit parallel build jobs (default: all cores)")
        print("                   Applies to: compile, test")
        print("  --skip-analyze   Skip clang-tidy + MSVC /analyze gate")
        print("                   Applies to: compile")
        print("  -k, --filter R   Run only tests whose CTest name matches the")
        print("                   regex R (e.g. -k BookTable). The CTest name")
        print("                   is the file name without the `Test` suffix.")
        print("                   Applies to: test")
        print("  -l, --list       List the registered tests, run nothing")
        print("                   Applies to: test")
        print("  --no-build       Run what is already built, build nothing")
        print("                   Applies to: test")
        print("  --python         Also run the buildtools' unittest suite")
        print("  --python-only    Run only that suite (no build, no CTest)")
        print("                   Applies to: test")
        print("  --all            Run every configuration: Debug/Release/")
        print("                   MinSizeRel on desktop, plus each Android ABI")
        print("                   (arm64-v8a, x86_64, ...) per build type.")
        print("                   Overrides -d/--release.")
        print("                   Applies to: bootstrap, compile")

    def _print_paths(self) -> None:
        print("\nPaths:")
        dirs = {
            "Project":      self.config.project_dir,
            "Venv":         self.config.venv_dir,
            "Dependencies": self.config.deps_dir,
            "vcpkg":        self.config.vcpkg_dir,
            "Android root": self.config.android_root,
            "JDK 17":       self.config.jdk_dir,
            "Android SDK":  self.config.android_sdk_dir,
            "Android NDK":  self.config.android_ndk_dir,
            "Host Qt":      self.config.qt_host_dir,
            "Android Qt":   self.config.qt_android_dir,
        }
        for label, path in dirs.items():
            exists, loc = self.checker.check_dir(path)
            status = "OK" if exists else "MISSING"
            print(f"  {label:<14} {loc:<60} [{status}]")

    def _print_tools(self) -> None:
        cmake_venv = self.config.venv_executable("cmake")
        vcpkg_path = self.config.vcpkg_executable()
        clang_format_venv = self.config.venv_executable("clang-format")
        clang_tidy_venv = self.config.venv_executable("clang-tidy")
        def qt_tool_exe(name: str) -> str:
            return f"{name}.exe" if self.config.is_windows else name

        qmlformat_path = self.config.qt_tools_dir / qt_tool_exe("qmlformat")
        lupdate_path = self.config.qt_tools_dir / qt_tool_exe("lupdate")
        lrelease_path = self.config.qt_tools_dir / qt_tool_exe("lrelease")

        tools: dict[str, list[Path]] = {
            "git":          [],
            "cmake":        [cmake_venv],
            "vcpkg":        [vcpkg_path],
            "clang-format": [clang_format_venv],
            "clang-tidy":   [clang_tidy_venv],
            "qmlformat":    [qmlformat_path],
            "lupdate":      [lupdate_path],
            "lrelease":     [lrelease_path],
        }

        print("\nTools:")
        for name, extra in tools.items():
            found, location = self.checker.check_tool(name, extra)
            if found:
                version = self.checker.get_tool_version(location)
                print(f"  {name:<14} {version:<40} [{location}]")
            else:
                print(f"  {name:<14} {'not installed':<40} "
                      "[will be installed on bootstrap]")
