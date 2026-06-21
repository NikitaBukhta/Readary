import os
from pathlib import Path

from buildtools.commands.base import Command
from buildtools.commands.format import FormatCommand
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError
from buildtools.providers.android_sdk import AndroidSdkProvider
from buildtools.providers.aqt import AqtProvider
from buildtools.providers.clang_format import ClangFormatProvider
from buildtools.providers.clang_tidy import ClangTidyProvider
from buildtools.providers.cmake import CMakeProvider
from buildtools.providers.jdk import JdkProvider
from buildtools.providers.msvc import MsvcProvider
from buildtools.providers.ninja import NinjaProvider
from buildtools.providers.qml_format import QmlFormatProvider
from buildtools.shell import Shell


class CompileCommand(Command):
    """Builds the project (must run bootstrap first)."""

    name = "compile"
    summary = "Build the project (with static analysis by default)"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 cmake: CMakeProvider, msvc: MsvcProvider,
                 clang_format: ClangFormatProvider,
                 clang_tidy: ClangTidyProvider,
                 qml_format: QmlFormatProvider,
                 jdk: JdkProvider, android_sdk: AndroidSdkProvider,
                 aqt: AqtProvider, ninja: NinjaProvider):
        self.config = config
        self.shell = shell
        self.cmake = cmake
        self.msvc = msvc
        self.clang_format = clang_format
        self.clang_tidy = clang_tidy
        self.qml_format = qml_format
        self.jdk = jdk
        self.android_sdk = android_sdk
        self.aqt = aqt
        self.ninja = ninja

    def execute(self) -> None:
        if self.config.is_android:
            self._execute_android()
        else:
            self._execute_desktop()

    # ---- desktop ----------------------------------------------------------

    def _execute_desktop(self) -> None:
        FormatCommand(self.config, self.shell, self.clang_format,
                      self.qml_format).execute()

        cmake_path = self.cmake.ensure()
        env = self.msvc.env()
        # Keep the vcpkg install tree consistent with bootstrap on reconfigure.
        env["VCPKG_INSTALLED_DIR"] = self.config.deps_dir.as_posix()

        self._sync_analyze_setting(cmake_path, env)

        analyze_label = "OFF — gate skipped" if self.config.skip_analyze \
            else "ON — clang-tidy + MSVC /analyze"
        print(f"\n=== Building ({self.config.build_type}) ===")
        print(f"  Static analysis: {analyze_label}")
        self.shell.run([
            cmake_path,
            "--build", "--preset", self.config.cmake_preset,
            "--config", self.config.build_type,
            "-j", str(self.config.jobs),
        ], env=env)

        print("\nBuild complete.")

    # ---- android ----------------------------------------------------------

    def _execute_android(self) -> None:
        # No format step here — clang-format/qmlformat are desktop-only;
        # the source is identical so a Windows compile covers it.
        cmake_path = self.cmake.ensure()
        env = self._require_android_env()

        build_dir = self.config.cmake_build_dir
        if not (build_dir / "CMakeCache.txt").exists():
            raise BuildError(
                f"Android build is not configured at {build_dir}. Run "
                "`python bootstrap.py bootstrap -d android"
                f"{' --release' if self.config.release else ''}"
                f" --abi {' '.join(self.config.android_abis)}` first."
            )

        print(f"\n=== Building ({self.config.build_type}, Android APK, "
              f"ABIs: {', '.join(self.config.android_abis)}) ===")
        # Build by dir, not preset — preset's binaryDir doesn't match our
        # ABI-suffixed build_dir.
        self.shell.run([
            cmake_path, "--build", str(build_dir),
            "-j", str(self.config.jobs),
        ], env=env)

        # `apk` target = androiddeployqt + Gradle, produced by qt_finalize_executable.
        print(f"\n=== Packaging APK ({self.config.build_type}) ===")
        self.shell.run([
            cmake_path, "--build", str(build_dir),
            "--target", "apk",
            "-j", str(self.config.jobs),
        ], env=env)

        apk = self._find_apk()
        if apk:
            print(f"\nAPK ready: {apk}")
        else:
            print("\nBuild complete — APK location varies, check "
                  f"{build_dir}/android-build/")

    def _require_android_env(self) -> dict[str, str]:
        sdk_root = self.config.android_sdk_dir
        ndk_root = self.config.android_ndk_dir
        host_qt = self.config.qt_host_dir
        android_qt = self.config.qt_android_dir
        jdk_home = self.config.jdk_dir

        missing = [str(p) for p in (sdk_root, ndk_root, host_qt, android_qt,
                                    jdk_home) if not p.exists()]
        if missing:
            raise BuildError(
                "Android toolchain is not installed. Run "
                "`python bootstrap.py bootstrap -d android` first.\n"
                "Missing:\n  " + "\n  ".join(missing)
            )

        ninja_path = self.ninja.ensure()

        env = os.environ.copy()
        env["JAVA_HOME"] = str(jdk_home)
        env["ANDROID_SDK_ROOT"] = str(sdk_root)
        env["ANDROID_HOME"] = str(sdk_root)
        env["ANDROID_NDK_ROOT"] = str(ndk_root)
        env["ANDROID_NDK_HOME"] = str(ndk_root)
        env["QT_HOST_PATH"] = str(host_qt)
        env["QT_ANDROID_PATH"] = str(android_qt)
        env["PATH"] = (
            f"{jdk_home / 'bin'}{os.pathsep}"
            f"{ninja_path.parent}{os.pathsep}"
            f"{env.get('PATH', '')}"
        )
        return env

    def _find_apk(self) -> Path | None:
        build_dir = self.config.cmake_build_dir
        roots = [
            build_dir / "android-build" / "build" / "outputs" / "apk",
            build_dir / "android-Readary" / "build" / "outputs" / "apk",
        ]
        for root in roots:
            if not root.exists():
                continue
            for apk in root.rglob("*.apk"):
                return apk
        return None

    # ---- shared (desktop only) -------------------------------------------

    def _sync_analyze_setting(self, cmake_path: Path,
                              env: dict[str, str] | None) -> None:
        """Reconfigure CMake if ENABLE_ANALYZE differs from desired value."""
        desired = "OFF" if self.config.skip_analyze else "ON"
        current = self._read_cache_var("ENABLE_ANALYZE")
        if current == desired:
            return

        print(f"\n=== Reconfiguring (ENABLE_ANALYZE={desired}) ===")
        cmake_cmd: list[str | Path] = [
            cmake_path,
            "--preset", self.config.cmake_preset,
            "-S", self.config.project_dir,
            f"-DENABLE_ANALYZE={desired}",
        ]
        if desired == "ON":
            # Re-resolve in case venv was wiped since bootstrap.
            ct_path = self.clang_tidy.ensure()
            wrapper_script = (self.config.project_dir / "buildtools"
                              / "clang_tidy_wrapper.py")
            cmake_cmd.append(f"-DCLANG_TIDY_EXECUTABLE={ct_path}")
            cmake_cmd.append(
                f"-DCLANG_TIDY_WRAPPER_PYTHON={self.config.venv_python}")
            cmake_cmd.append(f"-DCLANG_TIDY_WRAPPER_SCRIPT={wrapper_script}")
        self.shell.run(cmake_cmd, env=env)

    def _read_cache_var(self, name: str) -> str | None:
        cache = self.config.cmake_build_dir / "CMakeCache.txt"
        if not cache.exists():
            return None
        prefix = f"{name}:"
        try:
            for line in cache.read_text(encoding="utf-8").splitlines():
                if line.startswith(prefix):
                    _, _, value = line.partition("=")
                    return self._normalize_bool(value.strip())
        except OSError:
            return None
        return None

    @staticmethod
    def _normalize_bool(value: str) -> str:
        truthy = {"1", "ON", "TRUE", "YES", "Y"}
        falsy = {"0", "OFF", "FALSE", "NO", "N", ""}
        upper = value.upper()
        if upper in truthy:
            return "ON"
        if upper in falsy:
            return "OFF"
        return value
