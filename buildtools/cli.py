import argparse
import subprocess  # nosec B404 — build CLI: invokes vetted toolchain binaries only
import sys
from pathlib import Path

from buildtools.commands import (
    AnalyzeCommand,
    BootstrapCommand,
    CleanCommand,
    Command,
    CompileCommand,
    FormatCommand,
    HelpCommand,
    PackageCommand,
    RunCommand,
    TestCommand,
    TranslateCommand,
)
from buildtools.config import (
    DEFAULT_ANDROID_ABIS,
    SUPPORTED_ANDROID_ABIS,
    SUPPORTED_TARGETS,
    ProjectConfig,
    host_target,
)
from buildtools.errors import BuildError, ToolNotFoundError
from buildtools.providers import (
    AndroidSdkProvider,
    AqtProvider,
    CMakeProvider,
    ClangFormatProvider,
    ClangTidyProvider,
    GitProvider,
    JdkProvider,
    LinguistProvider,
    MsvcProvider,
    NinjaProvider,
    OpensslAndroidProvider,
    QmlFormatProvider,
    VcpkgProvider,
)
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager


class CommandRegistry:
    """Builds the command graph from providers and config."""

    def __init__(self, config: ProjectConfig):
        self.config = config
        self.shell = Shell()
        self._venv_mgr = VenvManager(config, self.shell)
        self._git = GitProvider(self.shell)
        self._cmake = CMakeProvider(self.shell, self._venv_mgr)
        self._vcpkg = VcpkgProvider(self.shell, config, self._git)
        self._msvc = MsvcProvider(self.shell, config)
        self._clang_format = ClangFormatProvider(
            self.shell, self._venv_mgr, config.project_dir,
        )
        self._clang_tidy = ClangTidyProvider(self.shell, self._venv_mgr)
        self._qml_format = QmlFormatProvider(self.shell, config)
        self._linguist = LinguistProvider(self.shell, config)
        self._jdk = JdkProvider(self.shell, config)
        self._android_sdk = AndroidSdkProvider(self.shell, config, self._jdk)
        self._aqt = AqtProvider(self.shell, config, self._venv_mgr)
        self._ninja = NinjaProvider(self.shell, self._venv_mgr)
        self._openssl_android = OpensslAndroidProvider(
            self.shell, config, self._git,
        )

    def build(self) -> dict[str, Command]:
        commands: dict[str, Command] = {
            "bootstrap": BootstrapCommand(
                self.config, self.shell,
                self._venv_mgr, self._cmake, self._vcpkg,
                self._msvc, self._clang_format, self._clang_tidy,
                self._qml_format,
                self._jdk, self._android_sdk, self._aqt, self._ninja,
                self._openssl_android,
            ),
            "compile": CompileCommand(
                self.config, self.shell, self._cmake,
                self._msvc, self._clang_format, self._clang_tidy,
                self._qml_format,
                self._jdk, self._android_sdk, self._aqt, self._ninja,
            ),
            "analyze": AnalyzeCommand(
                self.config, self.shell, self._clang_tidy,
            ),
            "format": FormatCommand(
                self.config, self.shell, self._clang_format,
                self._qml_format,
            ),
            "run": RunCommand(
                self.config, self.shell, self._jdk, self._android_sdk,
            ),
            "test": TestCommand(self.config),
            "translate": TranslateCommand(
                self.config, self.shell, self._venv_mgr, self._linguist,
            ),
            "package": PackageCommand(self.config, self.shell),
            "clean": CleanCommand(self.config),
        }
        commands["help"] = HelpCommand(self.config, self.shell, commands)
        return commands


class CLI:
    """Parses arguments and dispatches commands."""

    def __init__(self):
        self.parser = argparse.ArgumentParser(
            description="Bootstrap and build the Library project.",
            add_help=False,
        )
        self._setup_subcommands()

    def _setup_subcommands(self) -> None:
        self.parser.add_argument(
            "-h", "--help", action="store_true", default=False,
        )

        help_parser = argparse.ArgumentParser(add_help=False)
        help_parser.add_argument(
            "-h", "--help", action="store_true", default=False,
        )

        release_parser = argparse.ArgumentParser(add_help=False)
        release_parser.add_argument(
            "--release", action="store_true",
            help="Use Release mode (default: Debug)",
        )

        target_parser = argparse.ArgumentParser(add_help=False)
        target_parser.add_argument(
            "-d", "--device", dest="target", default=None,
            choices=list(SUPPORTED_TARGETS),
            help=f"Target device (default: host = {host_target()}). "
                 f"Choices: {', '.join(SUPPORTED_TARGETS)}",
        )
        target_parser.add_argument(
            "--abi", "--abis", dest="abis", nargs="+", default=None,
            choices=[*SUPPORTED_ANDROID_ABIS, "all"],
            metavar="ABI",
            help=f"Android ABI(s) to build for (only with -d android). "
                 f"Default: {' '.join(DEFAULT_ANDROID_ABIS)}. "
                 f"Choices: {', '.join(SUPPORTED_ANDROID_ABIS)}, all. "
                 f"`all` expands to every supported ABI. "
                 f"Multi-ABI: --abi arm64-v8a x86_64",
        )

        subs = self.parser.add_subparsers(dest="command")

        p_bootstrap = subs.add_parser(
            "bootstrap", parents=[help_parser, release_parser, target_parser],
            add_help=False,
            help="Install dependencies and configure",
        )
        p_bootstrap.add_argument(
            "-D", dest="cmake_defs", action="append", default=[],
            metavar="VAR=VALUE",
            help="Pass CMake cache variable (e.g. -DBUILD_TESTS=OFF)",
        )

        p_compile = subs.add_parser(
            "compile", parents=[help_parser, release_parser, target_parser],
            add_help=False,
            help="Build the project (with static analysis by default)",
        )
        p_compile.add_argument(
            "-j", "--jobs", type=int, default=None,
            help="Parallel build jobs",
        )
        p_compile.add_argument(
            "--skip-analyze", action="store_true",
            help="Skip clang-tidy + MSVC /analyze gate (faster, less safe)",
        )

        subs.add_parser(
            "analyze", parents=[help_parser, release_parser],
            add_help=False,
            help="Run clang-tidy over the project (no compile)",
        )

        subs.add_parser(
            "run", parents=[help_parser, release_parser, target_parser],
            add_help=False,
            help="Run the built application "
                 "(-d android: install + launch APK on connected device)",
        )

        subs.add_parser(
            "package", parents=[help_parser],
            add_help=False,
            help="Create installer (Inno Setup)",
        )

        subs.add_parser(
            "test", parents=[help_parser, release_parser],
            add_help=False,
            help="Run unit tests",
        )

        subs.add_parser(
            "format", parents=[help_parser],
            add_help=False,
            help="Auto-format C++ source files",
        )

        subs.add_parser(
            "translate", parents=[help_parser],
            add_help=False,
            help="Update .ts and compile .qm translation files",
        )

        subs.add_parser(
            "clean", parents=[help_parser],
            add_help=False,
            help="Remove dependencies, build dirs, and venv",
        )

        subs.add_parser("help", parents=[help_parser],
                         add_help=False,
                         help="Show detailed help and status")

    def run(self, project_dir: Path) -> None:
        args = self.parser.parse_args()
        if args.help:
            args.command = "help"
        kwargs: dict = {
            "project_dir": project_dir,
            "release": getattr(args, "release", False),
        }
        target = getattr(args, "target", None)
        if target:
            kwargs["target"] = target
        abis = getattr(args, "abis", None)
        if abis:
            # Expand `all`, dedupe preserving user-typed order (first = anchor ABI).
            expanded: list[str] = []
            for a in abis:
                if a == "all":
                    expanded.extend(SUPPORTED_ANDROID_ABIS)
                else:
                    expanded.append(a)
            seen: set[str] = set()
            uniq: list[str] = []
            for a in expanded:
                if a not in seen:
                    seen.add(a)
                    uniq.append(a)
            kwargs["android_abis"] = tuple(uniq)
        jobs = getattr(args, "jobs", None)
        if jobs is not None:
            kwargs["jobs"] = jobs
        cmake_defs = getattr(args, "cmake_defs", None)
        if cmake_defs:
            kwargs["cmake_defs"] = cmake_defs
        if getattr(args, "skip_analyze", False):
            kwargs["skip_analyze"] = True
        config = ProjectConfig(**kwargs)
        command_name = args.command or "help"
        registry = CommandRegistry(config)
        commands = registry.build()

        try:
            commands[command_name].execute()
        except (ToolNotFoundError, BuildError) as e:
            print(f"\nERROR: {e}")
            sys.exit(1)
        except subprocess.CalledProcessError as e:
            print(f"\nERROR: Command failed with exit code {e.returncode}")
            sys.exit(e.returncode)
