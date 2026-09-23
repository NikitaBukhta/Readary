import argparse
import subprocess  # nosec B404 — build CLI: invokes vetted toolchain binaries only
import sys
from pathlib import Path

from buildtools.commands import (
    AnalyzeCommand,
    BootstrapCommand,
    CleanCacheCommand,
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
            "test": TestCommand(
                self.config, self.shell, self._cmake, self._msvc,
            ),
            "translate": TranslateCommand(
                self.config, self.shell, self._venv_mgr, self._linguist,
            ),
            "package": PackageCommand(self.config, self.shell),
            "clean": CleanCommand(self.config),
            "clean-cache": CleanCacheCommand(self.config),
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
        p_bootstrap.add_argument(
            "--skip-vcpkg", action="store_true",
            help="Skip the vcpkg dependency check/install on configure "
                 "(uses the already-installed deps as they are)",
        )
        p_bootstrap.add_argument(
            "--all", dest="all_configs", action="store_true",
            help="Run for every configuration: Debug/Release/MinSizeRel on "
                 "desktop, plus each Android ABI per build type. "
                 "Overrides -d/--release.",
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
        p_compile.add_argument(
            "--skip-vcpkg", action="store_true",
            help="Skip the vcpkg dependency check if CMake reconfigures "
                 "(uses the already-installed deps as they are)",
        )
        p_compile.add_argument(
            "--all", dest="all_configs", action="store_true",
            help="Build every configuration: Debug/Release/MinSizeRel on "
                 "desktop, plus each Android ABI per build type. "
                 "Overrides -d/--release.",
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

        p_test = subs.add_parser(
            "test", parents=[help_parser, release_parser],
            add_help=False,
            help="Build and run the autotests",
        )
        p_test.add_argument(
            "-k", "--filter", dest="test_filter", default=None,
            metavar="REGEX",
            help="Only run tests whose CTest name matches REGEX "
                 "(e.g. -k BookTable). Names drop the `Test` suffix.",
        )
        p_test.add_argument(
            "-l", "--list", dest="list_tests", action="store_true",
            help="List the registered tests without running them",
        )
        p_test.add_argument(
            "--no-build", dest="skip_test_build", action="store_true",
            help="Run whatever is already built instead of building first",
        )
        p_test.add_argument(
            "-j", "--jobs", type=int, default=None,
            help="Parallel build jobs for the build step",
        )
        p_test.add_argument(
            "--python", dest="python_tests", action="store_true",
            help="Also run the buildtools' own unittest suite",
        )
        p_test.add_argument(
            "--python-only", dest="test_python_only", action="store_true",
            help="Run only the buildtools' unittest suite (no build, no CTest)",
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

        p_clean_cache = subs.add_parser(
            "clean-cache", parents=[help_parser],
            add_help=False,
            help="Remove vcpkg build caches (keeps installed deps)",
        )
        p_clean_cache.add_argument(
            "--deep", dest="clean_cache_deep", action="store_true",
            help="Also clear the vcpkg binary cache (forces a full "
                 "from-source rebuild on the next bootstrap)",
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
        if getattr(args, "skip_vcpkg", False):
            kwargs["skip_vcpkg"] = True
        if getattr(args, "clean_cache_deep", False):
            kwargs["clean_cache_deep"] = True
        test_filter = getattr(args, "test_filter", None)
        if test_filter:
            kwargs["test_filter"] = test_filter
        if getattr(args, "list_tests", False):
            kwargs["list_tests"] = True
        if getattr(args, "skip_test_build", False):
            kwargs["skip_test_build"] = True
        # --python-only implies --python: one flag decides whether the suite
        # runs, the other whether anything else does.
        if getattr(args, "test_python_only", False):
            kwargs["python_tests"] = True
            kwargs["test_python_only"] = True
        elif getattr(args, "python_tests", False):
            kwargs["python_tests"] = True

        command_name = args.command or "help"

        if getattr(args, "all_configs", False) and not args.help:
            self._run_all(command_name, kwargs)
            return

        self._run_one(command_name, ProjectConfig(**kwargs))

    # Every configuration the `--all` matrix expands to: each build type, on
    # desktop (the host) and on Android. Desktop is a single config per build
    # type; Android fans out one config per supported ABI (each built on its
    # own) so `--all` covers every architecture. An explicit --abi narrows the
    # Android ABIs to that set.
    _ALL_BUILD_TYPES = ("Debug", "Release", "MinSizeRel")

    def _run_all(self, command_name: str, base_kwargs: dict) -> None:
        variants: list[ProjectConfig] = []
        # Android ABIs: honor an explicit --abi, else every supported ABI.
        android_abis = base_kwargs.get("android_abis") or SUPPORTED_ANDROID_ABIS
        for build_type in self._ALL_BUILD_TYPES:
            kwargs = dict(base_kwargs)
            kwargs["target"] = host_target()
            kwargs.pop("android_abis", None)
            kwargs["build_type_override"] = build_type
            # Debug links the debug vcpkg triplet; Release/MinSizeRel use the
            # release tree (vcpkg maps MinSizeRel imports to Release).
            kwargs["release"] = build_type != "Debug"
            variants.append(ProjectConfig(**kwargs))
        for build_type in self._ALL_BUILD_TYPES:
            for abi in android_abis:
                kwargs = dict(base_kwargs)
                kwargs["target"] = "android"
                kwargs["android_abis"] = (abi,)
                kwargs["build_type_override"] = build_type
                kwargs["release"] = build_type != "Debug"
                variants.append(ProjectConfig(**kwargs))

        print(f"\n=== `{command_name} --all`: {len(variants)} configurations ===")
        for cfg in variants:
            label = (cfg.cmake_preset
                     + (f" [{', '.join(cfg.android_abis)}]"
                        if cfg.is_android else ""))
            print(f"  - {label}")

        # Run every variant; one config's failure must not abort the rest of
        # the matrix (e.g. a transient aqt download failure for a single ABI).
        # Collect outcomes and report a summary, exiting non-zero if any failed.
        failures: list[str] = []
        for cfg in variants:
            label = cfg.cmake_preset
            if cfg.is_android:
                label += f" ({', '.join(cfg.android_abis)})"
            print(f"\n########## {command_name}: {label} ##########")
            err = self._run_one(command_name, cfg, abort_on_error=False)
            if err is not None:
                failures.append(f"{label}: {err}")

        ok = len(variants) - len(failures)
        print(f"\n=== `{command_name} --all` summary: "
              f"{ok}/{len(variants)} succeeded ===")
        for f in failures:
            print(f"  FAILED  {f}")
        if failures:
            sys.exit(1)

    @staticmethod
    def _run_one(command_name: str, config: ProjectConfig,
                 abort_on_error: bool = True) -> str | None:
        """Run one command for one config.

        Returns None on success. With abort_on_error (default, single-config
        runs) a failure prints and exits the process; with it off (the --all
        matrix) the error message is returned so the caller can continue.
        """
        registry = CommandRegistry(config)
        commands = registry.build()
        try:
            commands[command_name].execute()
            return None
        except (ToolNotFoundError, BuildError) as e:
            msg = str(e)
            print(f"\nERROR: {msg}")
            if abort_on_error:
                sys.exit(1)
            return msg
        except subprocess.CalledProcessError as e:
            msg = f"Command failed with exit code {e.returncode}"
            print(f"\nERROR: {msg}")
            if abort_on_error:
                sys.exit(e.returncode)
            return msg
