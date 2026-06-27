import json
import os
import shutil
from pathlib import Path

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.providers.android_sdk import AndroidSdkProvider
from buildtools.providers.aqt import AqtProvider
from buildtools.providers.clang_format import ClangFormatProvider
from buildtools.providers.clang_tidy import ClangTidyProvider
from buildtools.providers.cmake import CMakeProvider
from buildtools.providers.jdk import JdkProvider
from buildtools.providers.msvc import MsvcProvider
from buildtools.providers.ninja import NinjaProvider
from buildtools.providers.openssl_android import OpensslAndroidProvider
from buildtools.providers.qml_format import QmlFormatProvider
from buildtools.providers.vcpkg import VcpkgProvider
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager


def _fwd(p: Path) -> str:
    # Forward slashes survive CMake -> JSON -> Qt parsing; `C:\Users` would
    # otherwise get JSON-unescaped to `C:Users` in androiddeployqt input.
    return str(p).replace("\\", "/")


class BootstrapCommand(Command):
    """Installs deps and configures the project (desktop via vcpkg, Android via aqt+SDK+NDK)."""

    name = "bootstrap"
    summary = "Install dependencies and configure the project"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 venv_mgr: VenvManager, cmake: CMakeProvider,
                 vcpkg: VcpkgProvider, msvc: MsvcProvider,
                 clang_format: ClangFormatProvider,
                 clang_tidy: ClangTidyProvider,
                 qml_format: QmlFormatProvider,
                 jdk: JdkProvider, android_sdk: AndroidSdkProvider,
                 aqt: AqtProvider, ninja: NinjaProvider,
                 openssl_android: OpensslAndroidProvider):
        self.config = config
        self.shell = shell
        self.venv_mgr = venv_mgr
        self.cmake = cmake
        self.vcpkg = vcpkg
        self.msvc = msvc
        self.clang_format = clang_format
        self.clang_tidy = clang_tidy
        self.qml_format = qml_format
        self.jdk = jdk
        self.android_sdk = android_sdk
        self.aqt = aqt
        self.ninja = ninja
        self.openssl_android = openssl_android

    def execute(self) -> None:
        if self.config.is_android:
            self._execute_android()
        else:
            self._execute_desktop()

    @staticmethod
    def _prefer_stable_cmake(env: dict[str, str], cmake_path: Path) -> None:
        """Put our resolved (stable) cmake first on PATH for the configure env.

        vcpkg builds ports with whatever cmake it finds on PATH; if that's a
        pre-release (e.g. 4.1-rc), the qtdeclarative port configure breaks
        ('configure_file ... does not exist'). Prepending the dir of the cmake
        CMakeProvider chose forces vcpkg to use the same stable cmake.
        """
        cmake_dir = str(Path(cmake_path).parent)
        # vcvars64 emits the var as "Path" (Windows casing). Reuse that exact
        # key — hardcoding "PATH" would add a SECOND, colliding env entry whose
        # truncated value can win on Windows, dropping cl.exe off the toolchain
        # probe ("No CMAKE_CXX_COMPILER could be found").
        path_key = next((k for k in env if k.upper() == "PATH"), "PATH")
        env[path_key] = f"{cmake_dir}{os.pathsep}{env.get(path_key, '')}"

    def _stage_host_tool_dlls(self) -> None:
        """Copy runtime DLLs next to the Qt host tools (rcc/moc/uic/...).

        vcpkg's split layout puts the host tools in tools/Qt6/bin but their
        runtime DLLs (Qt6Core.dll + icu/pcre2/zstd/...) in bin/. A tool only
        finds those DLLs when bin/ is on PATH. The CLI build sets that PATH, but
        IDE-launched builds (CLion runs ninja with its own environment that does
        NOT apply the CMake configure-preset `environment`) don't — so rcc.exe
        dies with 0xc0000135 (STATUS_DLL_NOT_FOUND). Making the tools dir
        self-contained fixes the build for any caller, PATH or not. Idempotent:
        only copies DLLs the tools dir is missing.
        """
        base = self.config.deps_dir / "x64-windows"
        bin_dir = base / "bin"
        tools_bin = base / "tools" / "Qt6" / "bin"
        if not bin_dir.is_dir() or not tools_bin.is_dir():
            return
        copied = 0
        for dll in bin_dir.glob("*.dll"):
            dest = tools_bin / dll.name
            if not dest.exists():
                shutil.copy2(dll, dest)
                copied += 1
        if copied:
            print(f"  Host tools   : staged {copied} runtime DLL(s) into "
                  f"{tools_bin}")

    # ---- desktop ----------------------------------------------------------

    def _execute_desktop(self) -> None:
        self.venv_mgr.ensure()
        cmake_path = self.cmake.ensure()
        self.vcpkg.ensure()
        self._stage_host_tool_dlls()
        self.clang_format.ensure_config()
        clang_tidy_path = self.clang_tidy.ensure()
        env = self.msvc.env()
        # Single source of truth for the vcpkg install tree (see project.json).
        env["VCPKG_INSTALLED_DIR"] = _fwd(self.config.deps_dir)
        self._prefer_stable_cmake(env, cmake_path)
        # CMake's generate step runs the Qt host tools (rcc/moc/uic) to probe
        # them (AUTORCC etc.). Those are release exes under tools/Qt6/bin that
        # link the release Qt6Core.dll in x64-windows/bin — put that dir on PATH
        # or the probe dies with 0xc0000135 (STATUS_DLL_NOT_FOUND).
        host_dll_dir = str(self.config.deps_dir / "x64-windows" / "bin")
        path_key = next((k for k in env if k.upper() == "PATH"), "PATH")
        env[path_key] = f"{host_dll_dir}{os.pathsep}{env.get(path_key, '')}"

        wrapper_script = (self.config.project_dir / "buildtools"
                          / "clang_tidy_wrapper.py")
        print(f"\n=== Configuring ({self.config.build_type}) ===")
        cmake_cmd = [
            cmake_path,
            "--preset", self.config.cmake_preset,
            "-S", self.config.project_dir,
            f"-DCLANG_TIDY_EXECUTABLE={clang_tidy_path}",
            f"-DCLANG_TIDY_WRAPPER_PYTHON={self.config.venv_python}",
            f"-DCLANG_TIDY_WRAPPER_SCRIPT={wrapper_script}",
            "-DENABLE_ANALYZE=ON",
            f"-DVCPKG_INSTALL_OPTIONS=--x-buildtrees-root="
            f"{_fwd(self.config.vcpkg_buildtrees_dir)}",
        ]
        for d in self.config.cmake_defs:
            cmake_cmd.append(f"-D{d}")
        self.shell.run(cmake_cmd, env=env)

        # qmlformat is only resolvable after vcpkg installs Qt6 above.
        qmlformat_path = self.qml_format.ensure()
        print(f"  qmlformat    : {qmlformat_path}")
        print(f"  clang-tidy   : {clang_tidy_path}")

        user_presets = self._write_user_presets()
        print(f"  IDE presets  : {user_presets}")
        run_dir = self._write_run_configs()
        print(f"  IDE run cfgs : {run_dir}")

        print("\nBootstrap complete.")
        print(f"  Dependencies : {self.config.deps_dir}")
        print(f"  Virtual env  : {self.config.venv_dir}")
        flag = " --release" if self.config.release else ""
        print(f"Run `python bootstrap.py compile{flag}` to build the project.")

    # (slug, CMake build type, pretty label) for each variant we emit.
    _BUILD_TYPES = (
        ("debug", "Debug", "debug"),
        ("release", "Release", "release"),
        ("relminsize", "MinSizeRel", "RelMinSize"),
    )
    # (ABI, short label used in the profile name).
    _ANDROID_ABIS = (
        ("arm64-v8a", "arm64"),
        ("x86_64", "x86_64"),
    )

    def _write_user_presets(self) -> Path:
        """Generate a git-ignored CMakeUserPresets.json for IDEs (CLion, VS).

        The committed CMakePresets.json reads its paths from $env{...} that the
        CLI injects at runtime; IDEs have no such env. We materialize the
        resolved paths here — honoring any project.json deps_dir/vcpkg_dir
        override — so the IDE stays in sync with whatever bootstrap provisioned.
        Emits an amd64 (vcpkg) profile and one per Android ABI, each in
        Debug/Release/MinSizeRel. Regenerated on every bootstrap.

        Only configurePresets are emitted: IDEs derive the build step from the
        configure preset, and adding buildPresets just doubles every entry in
        CLion's profile list.
        """
        configure: list[dict] = []
        configure.extend(self._desktop_presets())
        configure.extend(self._android_presets())

        presets = {
            "version": 6,
            "configurePresets": configure,
        }
        path = self.config.project_dir / "CMakeUserPresets.json"
        path.write_text(json.dumps(presets, indent=2) + "\n", encoding="utf-8")
        return path

    def _desktop_presets(self) -> list[dict]:
        # amd64: vcpkg + Ninja Multi-Config. The preset `environment` carries
        # the Qt runtime env (plugins/qml/PATH) because CLion applies a
        # profile's environment to BOTH build and run — so the app finds its
        # platform plugin and the QML plugins' dependency DLLs (e.g. Qt6Quickd,
        # which vcpkg's applocal does NOT copy beside the exe) without relying
        # on qt.conf or a per-run-config env. Mirrors RunCommand's desktop env.
        base = self.config.deps_dir / "x64-windows"
        clang_tidy = _fwd(self.config.venv_executable("clang-tidy"))
        venv_py = _fwd(self.config.venv_python)
        wrapper = _fwd(self.config.project_dir / "buildtools"
                       / "clang_tidy_wrapper.py")
        out: list[dict] = []
        for slug, btype, label in self._BUILD_TYPES:
            # Debug links the debug triplet; Release/MinSizeRel use the release tree.
            triplet = base / "debug" if slug == "debug" else base
            # PATH must carry BOTH the release bin (always) and the build's own
            # triplet bin. The Qt host tools run during the build — rcc/moc/uic
            # — are RELEASE exes that link the release Qt6Core.dll in
            # x64-windows/bin; without it on PATH they die with 0xc0000135
            # (STATUS_DLL_NOT_FOUND). The triplet bin carries the runtime DLLs
            # the app loads (debug Qt6Cored.dll for debug). The two don't clash:
            # debug DLLs carry the 'd' suffix, so both dirs can coexist on PATH.
            bin_dirs = [_fwd(base / "bin")]
            if triplet != base:
                bin_dirs.append(_fwd(triplet / "bin"))
            env = {
                "VCPKG_ROOT": _fwd(self.config.vcpkg_dir),
                "VCPKG_INSTALLED_DIR": _fwd(self.config.deps_dir),
                "QT_PLUGIN_PATH": _fwd(triplet / "Qt6" / "plugins"),
                "QML2_IMPORT_PATH": _fwd(triplet / "Qt6" / "qml"),
                "PATH": ";".join([*bin_dirs, "$penv{PATH}"]),
            }
            # Analyze gate: OFF for debug (fast iteration), ON for release &
            # relminsize. ON needs the clang-tidy wrapper vars wired in.
            analyze = "OFF" if slug == "debug" else "ON"
            name = f"amd64-{slug}"
            out.append({
                "name": name,
                "displayName": f"amd64 {label}",
                "inherits": "vcpkg",
                "binaryDir": "${sourceDir}/build/" + name,
                "environment": env,
                "cacheVariables": {
                    # vcpkg's toolchain drops MinSizeRel from the multi-config
                    # list; re-add it so CMAKE_DEFAULT_BUILD_TYPE validates
                    # (vcpkg maps MinSizeRel imports to its Release build).
                    "CMAKE_CONFIGURATION_TYPES":
                        "Debug;Release;RelWithDebInfo;MinSizeRel",
                    "CMAKE_DEFAULT_BUILD_TYPE": btype,
                    "ENABLE_ANALYZE": analyze,
                    "CLANG_TIDY_EXECUTABLE": clang_tidy,
                    "CLANG_TIDY_WRAPPER_PYTHON": venv_py,
                    "CLANG_TIDY_WRAPPER_SCRIPT": wrapper,
                    # Short vcpkg buildtrees so a from-source Qt rebuild doesn't
                    # blow past Windows' path limit (see vcpkg_buildtrees_dir).
                    "VCPKG_INSTALL_OPTIONS":
                        "--x-buildtrees-root="
                        f"{_fwd(self.config.vcpkg_buildtrees_dir)}",
                },
            })
        return out

    def _android_presets(self) -> list[dict]:
        # One configure preset per (ABI, build type). Android Qt resolves its
        # toolchain from QT_ANDROID_PATH; we point each ABI at its own Qt prefix
        # (paths exist only after `bootstrap -d android`).
        host_qt = _fwd(self.config.qt_host_dir)
        sdk = _fwd(self.config.android_sdk_dir)
        ndk = _fwd(self.config.android_ndk_dir)
        jdk = _fwd(self.config.jdk_dir)
        out: list[dict] = []
        for abi, abi_label in self._ANDROID_ABIS:
            qt_abi = _fwd(self.config.qt_android_dir_for(abi))
            env = {
                "QT_HOST_PATH": host_qt,
                "QT_ANDROID_PATH": qt_abi,
                "ANDROID_SDK_ROOT": sdk,
                "ANDROID_NDK_ROOT": ndk,
                "JAVA_HOME": jdk,
            }
            for slug, btype, label in self._BUILD_TYPES:
                name = f"android-{abi_label}-{slug}"
                out.append({
                    "name": name,
                    "displayName": f"android {abi_label} {label}",
                    "inherits": "android",
                    "binaryDir": "${sourceDir}/build/" + name,
                    "environment": env,
                    "cacheVariables": {
                        "CMAKE_BUILD_TYPE": btype,
                        "QT_ANDROID_ABIS": abi,
                        f"QT_PATH_ANDROID_ABI_{abi}": qt_abi,
                    },
                })
        return out

    def _write_run_configs(self) -> Path:
        """Generate CLion shared run configs (.run/) for the amd64 profiles.

        Each binds to its CMake profile and injects the Qt runtime env the
        app needs to find its platform plugin — the desktop equivalent of what
        RunCommand sets up. Without this, launching from the IDE dies with
        "no Qt platform plugin could be initialized". Regenerated per bootstrap.
        Android isn't a plain executable run (it's adb install + launch), so
        it's left to `python bootstrap.py run -d android`.
        """
        run_dir = self.config.project_dir / ".run"
        run_dir.mkdir(exist_ok=True)
        base = self.config.deps_dir / "x64-windows"
        for slug, _btype, label in self._BUILD_TYPES:
            # Debug links the debug triplet; Release/MinSizeRel use the release tree.
            triplet = base / "debug" if slug == "debug" else base
            plugins = triplet / "Qt6" / "plugins"
            qml = triplet / "Qt6" / "qml"
            bin_dir = triplet / "bin"
            cfg_name = f"Readary (amd64 {label})"
            # CLion binds run configs by the preset *name* (CONFIG_NAME),
            # not its displayName — confirmed against .idea/workspace.xml.
            profile = f"amd64-{slug}"
            xml = (
                '<component name="ProjectRunConfigurationManager">\n'
                f'  <configuration default="false" name="{cfg_name}"'
                ' type="CMakeRunConfiguration" factoryName="Application"'
                ' PASS_PARENT_ENVS_2="true" PROJECT_NAME="Library"'
                ' TARGET_NAME="Readary"'
                f' CONFIG_NAME="{profile}"'
                ' RUN_TARGET_PROJECT_NAME="Library" RUN_TARGET_NAME="Readary">\n'
                "    <envs>\n"
                f'      <env name="QT_PLUGIN_PATH" value="{plugins}" />\n'
                f'      <env name="QML2_IMPORT_PATH" value="{qml}" />\n'
                f'      <env name="PATH" value="{bin_dir};$PATH$" />\n'
                "    </envs>\n"
                '    <method v="2">\n'
                '      <option name="com.jetbrains.cidr.execution.'
                'CidrBuildBeforeRunTaskProvider$BuildBeforeRunTask"'
                ' enabled="true" />\n'
                "    </method>\n"
                "  </configuration>\n"
                "</component>\n"
            )
            (run_dir / f"Readary_amd64_{slug}.run.xml").write_text(
                xml, encoding="utf-8")

        # Desktop "via script": the most reliable launch. run.py sets the full
        # Qt env (QT_PLUGIN_PATH/QML2_IMPORT_PATH/PATH) in-process, so it can't
        # hit the "no Qt platform plugin" failure that bites when CLion doesn't
        # apply a CMake run config's env. Builds via the script too (skip the
        # analyze gate for fast launches).
        for slug, _btype, label in self._BUILD_TYPES:
            flag = "" if slug == "debug" else " --release"
            cmd = (
                f"python bootstrap.py compile --skip-analyze{flag}; "
                f"if ($?) {{ python bootstrap.py run{flag} }}"
            )
            (run_dir / f"Readary_amd64_{slug}_script.run.xml").write_text(
                self._shell_run_xml(f"Readary (amd64 {label} → script)", cmd),
                encoding="utf-8")

        # Android: CLion can't launch an APK as a host binary. Deployment is
        # bootstrap -> compile -> run (adb install + am start + logcat), which
        # the script already implements and which targets a running emulator or
        # device (RunCommand resolves any `device`-state serial). bootstrap is
        # idempotent, so re-runs are cheap (Qt/SDK already installed).
        for abi, abi_label in self._ANDROID_ABIS:
            cmd = (
                f"python bootstrap.py bootstrap -d android --abi {abi}; "
                f"if ($?) {{ python bootstrap.py compile -d android "
                f"--abi {abi} }}; "
                f"if ($?) {{ python bootstrap.py run -d android --abi {abi} }}"
            )
            (run_dir / f"Readary_android_{abi_label}_deploy.run.xml"
             ).write_text(
                self._shell_run_xml(
                    f"Readary (android {abi_label} → device)", cmd),
                encoding="utf-8")
        return run_dir

    @staticmethod
    def _shell_run_xml(name: str, cmd: str) -> str:
        """A CLion Shell Script run config that runs `cmd` via PowerShell."""
        return (
            '<component name="ProjectRunConfigurationManager">\n'
            f'  <configuration default="false" name="{name}"'
            ' type="ShConfigurationType">\n'
            f'    <option name="SCRIPT_TEXT" value="{cmd}" />\n'
            '    <option name="INDEPENDENT_SCRIPT_PATH" value="true" />\n'
            '    <option name="SCRIPT_PATH" value="" />\n'
            '    <option name="SCRIPT_OPTIONS" value="" />\n'
            '    <option name="INDEPENDENT_SCRIPT_WORKING_DIRECTORY"'
            ' value="true" />\n'
            '    <option name="SCRIPT_WORKING_DIRECTORY"'
            ' value="$PROJECT_DIR$" />\n'
            '    <option name="INDEPENDENT_INTERPRETER_PATH" value="true" />\n'
            '    <option name="INTERPRETER_PATH" value="powershell.exe" />\n'
            '    <option name="INTERPRETER_OPTIONS" value="" />\n'
            '    <option name="EXECUTE_IN_TERMINAL" value="true" />\n'
            '    <option name="EXECUTE_SCRIPT_FILE" value="false" />\n'
            "    <envs />\n"
            '    <method v="2" />\n'
            "  </configuration>\n"
            "</component>\n"
        )

    # ---- android ----------------------------------------------------------

    def _execute_android(self) -> None:
        self.venv_mgr.ensure()
        cmake_path = self.cmake.ensure()
        ninja_path = self.ninja.ensure()

        abis = list(self.config.android_abis)
        print(f"\n=== Installing Android toolchain "
              f"(ABIs: {', '.join(abis)}) ===")
        jdk_home = self.jdk.ensure()
        sdk_root = self.android_sdk.ensure()
        ndk_root = self.android_sdk.ndk_root()
        self.aqt.ensure()
        host_qt = self.aqt.host_qt_dir()
        android_qt_dirs = self.aqt.android_qt_dirs()  # {abi: prefix}
        primary_qt = android_qt_dirs[abis[0]]
        self.openssl_android.ensure()
        all_ssl_libs = self.openssl_android.all_libs()

        env = self._android_env(jdk_home, sdk_root, ndk_root,
                                host_qt, primary_qt, ninja_path)

        print(f"\n=== Configuring ({self.config.build_type}, Android) ===")
        # androiddeployqt picks each lib's ABI from its parent dir name.
        extra_libs = ";".join(_fwd(p) for p in all_ssl_libs)
        package_source_dir = self.config.project_dir / "android"
        cmake_cmd: list[str | Path] = [
            cmake_path,
            "--preset", self.config.cmake_preset,
            "-S", self.config.project_dir,
            # Overrides the preset's binaryDir so each ABI combo gets its own dir.
            "-B", _fwd(self.config.cmake_build_dir),
            "-DENABLE_ANALYZE=OFF",
            "-DBUILD_TESTS=OFF",
            f"-DCMAKE_MAKE_PROGRAM={_fwd(ninja_path)}",
            f"-DQT_ANDROID_ABIS={';'.join(abis)}",
            f"-DANDROID_OPENSSL_EXTRA_LIBS={extra_libs}",
            f"-DANDROID_PACKAGE_SOURCE_DIR={_fwd(package_source_dir)}",
        ]
        # Multi-ABI: Qt resolves non-primary ABIs via QT_PATH_ANDROID_ABI_<abi>.
        for abi, qt_path in android_qt_dirs.items():
            cmake_cmd.append(
                f"-DQT_PATH_ANDROID_ABI_{abi}={_fwd(qt_path)}",
            )
        for d in self.config.cmake_defs:
            cmake_cmd.append(f"-D{d}")
        self.shell.run(cmake_cmd, env=env)

        print("\nBootstrap complete (Android).")
        print(f"  JDK            : {jdk_home}")
        print(f"  Android SDK    : {sdk_root}")
        print(f"  Android NDK    : {ndk_root}")
        print(f"  Host Qt        : {host_qt}")
        for abi, qt_path in android_qt_dirs.items():
            print(f"  Android Qt     : [{abi}] {qt_path}")
        print(f"  OpenSSL ABIs   : {', '.join(abis)} "
              f"({len(all_ssl_libs)} libs)")
        flag = " --release" if self.config.release else ""
        print(f"Run `python bootstrap.py compile -d android{flag}` to build "
              "the APK.")

    @staticmethod
    def _android_env(jdk_home: Path, sdk_root: Path, ndk_root: Path,
                     host_qt: Path, android_qt: Path,
                     ninja_path: Path) -> dict[str, str]:
        env = os.environ.copy()
        env["JAVA_HOME"] = str(jdk_home)
        env["ANDROID_SDK_ROOT"] = str(sdk_root)
        env["ANDROID_HOME"] = str(sdk_root)
        env["ANDROID_NDK_ROOT"] = str(ndk_root)
        env["ANDROID_NDK_HOME"] = str(ndk_root)
        env["QT_HOST_PATH"] = str(host_qt)
        env["QT_ANDROID_PATH"] = str(android_qt)
        # PATH: Gradle wants java/javac; CMake's Ninja generator probes PATH.
        env["PATH"] = (
            f"{jdk_home / 'bin'}{os.pathsep}"
            f"{ninja_path.parent}{os.pathsep}"
            f"{env.get('PATH', '')}"
        )
        return env
