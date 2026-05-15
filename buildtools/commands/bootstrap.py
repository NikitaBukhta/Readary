import os
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

    # ---- desktop ----------------------------------------------------------

    def _execute_desktop(self) -> None:
        self.venv_mgr.ensure()
        cmake_path = self.cmake.ensure()
        self.vcpkg.ensure()
        self.clang_format.ensure_config()
        clang_tidy_path = self.clang_tidy.ensure()
        env = self.msvc.env()

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
        ]
        for d in self.config.cmake_defs:
            cmake_cmd.append(f"-D{d}")
        self.shell.run(cmake_cmd, env=env)

        # qmlformat is only resolvable after vcpkg installs Qt6 above.
        qmlformat_path = self.qml_format.ensure()
        print(f"  qmlformat    : {qmlformat_path}")
        print(f"  clang-tidy   : {clang_tidy_path}")

        print("\nBootstrap complete.")
        print(f"  Dependencies : {self.config.deps_dir}")
        print(f"  Virtual env  : {self.config.venv_dir}")
        flag = " --release" if self.config.release else ""
        print(f"Run `python bootstrap.py compile{flag}` to build the project.")

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
