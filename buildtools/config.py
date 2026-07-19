import json
import os
import platform
from dataclasses import dataclass, field
from pathlib import Path

# Pinned to Qt 6.8 — bumping requires `bootstrap -d android` rerun.
ANDROID_QT_VERSION = "6.8.0"
ANDROID_NDK_VERSION = "26.1.10909125"  # r26b
ANDROID_PLATFORM_API = "34"
ANDROID_BUILD_TOOLS_VERSION = "34.0.0"
ANDROID_MIN_SDK = "23"
JDK_FEATURE_VERSION = "17"  # Gradle 8.x requirement

SUPPORTED_ANDROID_ABIS = ("arm64-v8a", "armeabi-v7a", "x86_64", "x86")
DEFAULT_ANDROID_ABIS: tuple[str, ...] = ("arm64-v8a",)


# aqt's Qt6 Android arch tokens mostly match the NDK ABI with `-`->`_`, EXCEPT
# 32-bit ARM: the NDK ABI is `armeabi-v7a` but aqt's token (and install dir) is
# `android_armv7`. Passing `android_armeabi_v7a` makes aqt's to_folder() return
# empty -> a broken `qt6_680/qt6_680` repo path -> "Failed to locate XML data".
_AQT_ARCH_TOKEN_OVERRIDES = {"armeabi-v7a": "armv7"}


def aqt_arch_for_abi(abi: str) -> str:
    token = _AQT_ARCH_TOKEN_OVERRIDES.get(abi, abi.replace('-', '_'))
    return f"android_{token}"


def qt_install_dirname_for_abi(abi: str) -> str:
    # aqt preserves the `android_` prefix in install paths for Android targets.
    return aqt_arch_for_abi(abi)


def _is_windows() -> bool:
    return platform.system() == "Windows"


def _load_project_settings(project_dir: Path) -> dict:
    """Read optional project.json overrides (deps_dir, vcpkg_dir)."""
    settings_file = project_dir / "project.json"
    if not settings_file.is_file():
        return {}
    try:
        return json.loads(settings_file.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError):
        return {}


def _resolve_dir(value: str | None, default: Path) -> Path:
    if not value:
        return default
    return Path(os.path.expanduser(value))


SUPPORTED_TARGETS = ("windows", "android")


def host_target() -> str:
    system = platform.system()
    if system == "Windows":
        return "windows"
    if system == "Darwin":
        return "macos"
    return "linux"


def _aqt_host() -> str:
    system = platform.system()
    if system == "Windows":
        return "windows"
    if system == "Darwin":
        return "mac"
    return "linux"


def _aqt_host_arch() -> str:
    system = platform.system()
    if system == "Windows":
        return "win64_msvc2022_64"
    if system == "Darwin":
        return "clang_64"
    return "gcc_64"


def _aqt_host_install_dirname() -> str:
    # aqt strips host prefixes from install paths (`win64_msvc2022_64` ->
    # `msvc2022_64`) but leaves arch-only tokens (`gcc_64`) alone.
    arch = _aqt_host_arch()
    for prefix in ("win64_", "win32_", "linux_", "mac_"):
        if arch.startswith(prefix):
            return arch[len(prefix):]
    return arch


def _adoptium_arch() -> str:
    machine = platform.machine().lower()
    if machine in ("amd64", "x86_64"):
        return "x64"
    if machine in ("arm64", "aarch64"):
        return "aarch64"
    return machine


def _adoptium_os() -> str:
    system = platform.system()
    if system == "Windows":
        return "windows"
    if system == "Darwin":
        return "mac"
    return "linux"


@dataclass(frozen=True)
class ProjectConfig:
    """Project-wide build configuration."""

    project_dir: Path
    release: bool = False
    target: str = field(default_factory=host_target)  # "windows" | "android"
    # First ABI doubles as `QT_ANDROID_PATH` anchor; ignored unless target=android.
    android_abis: tuple[str, ...] = DEFAULT_ANDROID_ABIS
    jobs: int = field(default_factory=lambda: os.cpu_count() or 1)
    cmake_defs: list[str] = field(default_factory=list)
    skip_analyze: bool = False
    # `clean-cache --deep`: also drop the vcpkg binary cache (forces a full
    # from-source rebuild next bootstrap). Off = keep it for fast re-bootstraps.
    clean_cache_deep: bool = False
    # Explicit build type (e.g. "MinSizeRel") that supersedes the Debug/Release
    # derived from `release`. Set by the `--all` matrix; None for normal runs.
    build_type_override: str | None = None

    venv_dir: Path = field(init=False)
    deps_dir: Path = field(init=False)
    vcpkg_dir: Path = field(init=False)
    is_windows: bool = field(init=False)

    def __post_init__(self):
        settings = _load_project_settings(self.project_dir)
        object.__setattr__(self, "venv_dir", self.project_dir / "venv")
        object.__setattr__(
            self,
            "deps_dir",
            _resolve_dir(settings.get("deps_dir"), Path.home() / "vcpkg_install_deps"),
        )
        object.__setattr__(
            self,
            "vcpkg_dir",
            _resolve_dir(settings.get("vcpkg_dir"), Path.home() / "vcpkg"),
        )
        object.__setattr__(self, "is_windows", _is_windows())

    @property
    def is_android(self) -> bool:
        return self.target == "android"

    @property
    def build_type(self) -> str:
        if self.build_type_override:
            return self.build_type_override
        return "Release" if self.release else "Debug"

    @property
    def cmake_preset(self) -> str:
        # Preset slugs are not always the lowercased build type (MinSizeRel ->
        # relminsize), so map explicitly and fall back to the lowercased name.
        slug = {
            "Debug": "debug",
            "Release": "release",
            "MinSizeRel": "relminsize",
        }.get(self.build_type, self.build_type.lower())
        if self.is_android:
            return f"android-{slug}"
        return slug

    @property
    def cmake_build_dir(self) -> Path:
        # Android dir is ABI-suffixed so parallel ABI configs don't clobber.
        # E.g. build/android-debug-arm64-v8a, build/android-debug-arm64-v8a_x86_64.
        base = self.project_dir / "build" / self.cmake_preset
        if not self.is_android:
            return base
        suffix = "_".join(self.android_abis)
        return base.with_name(f"{self.cmake_preset}-{suffix}")

    @property
    def _venv_scripts_dir(self) -> Path:
        if self.is_windows:
            return self.venv_dir / "Scripts"
        return self.venv_dir / "bin"

    @property
    def venv_python(self) -> Path:
        return self.venv_executable("python")

    def venv_executable(self, name: str) -> Path:
        if self.is_windows:
            return self._venv_scripts_dir / f"{name}.exe"
        return self._venv_scripts_dir / name

    @property
    def _vcpkg_triplet_dir(self) -> Path:
        base = self.deps_dir / "x64-windows"
        if self.release:
            return base
        return base / "debug"

    @property
    def qt_plugin_dir(self) -> Path:
        return self._vcpkg_triplet_dir / "Qt6" / "plugins"

    @property
    def qt_bin_dir(self) -> Path:
        return self._vcpkg_triplet_dir / "bin"

    @property
    def qt_qml_dir(self) -> Path:
        return self._vcpkg_triplet_dir / "Qt6" / "qml"

    @property
    def qt_tools_dir(self) -> Path:
        # vcpkg always exposes Qt6 host tools under tools/Qt6/bin, regardless of triplet.
        return self.deps_dir / "x64-windows" / "tools" / "Qt6" / "bin"

    def vcpkg_executable(self) -> Path:
        exe = "vcpkg.exe" if self.is_windows else "vcpkg"
        return self.vcpkg_dir / exe

    @property
    def vcpkg_buildtrees_dir(self) -> Path:
        # Short path on the deps drive: Qt's deeply-nested object-file names
        # blow past Windows' 250-char limit under the default buildtrees
        # location, failing the qtdeclarative port build. Kept short here.
        return Path(self.deps_dir.anchor or "C:/") / "vcpkgbt"

    @property
    def vcpkg_default_buildtrees_dir(self) -> Path:
        # vcpkg's in-tree buildtrees, used when no --x-buildtrees-root is passed
        # (older runs, manual `vcpkg install`). Pruned alongside the relocated one.
        return self.vcpkg_dir / "buildtrees"

    @property
    def vcpkg_packages_dir(self) -> Path:
        # Per-port staging vcpkg fills before copying into the install tree;
        # dead weight once the deps_dir install tree is populated.
        return self.vcpkg_dir / "packages"

    @property
    def vcpkg_downloads_dir(self) -> Path:
        # Cached upstream source tarballs; re-fetched on demand if a port rebuilds.
        return self.vcpkg_dir / "downloads"

    @property
    def vcpkg_binary_cache_dir(self) -> Path:
        # vcpkg's default binary cache (prebuilt port archives). Removing it
        # forces a full from-source rebuild on the next bootstrap, so it's only
        # cleared on a --deep clean.
        if self.is_windows:
            local = os.environ.get("LOCALAPPDATA")
            base = Path(local) if local else Path.home() / "AppData" / "Local"
            return base / "vcpkg" / "archives"
        xdg = os.environ.get("XDG_CACHE_HOME")
        base = Path(xdg) if xdg else Path.home() / ".cache"
        return base / "vcpkg" / "archives"

    # ---- Android paths ----------------------------------------------------

    @property
    def android_root(self) -> Path:
        return self.deps_dir / "android"

    @property
    def jdk_dir(self) -> Path:
        return self.android_root / f"jdk-{JDK_FEATURE_VERSION}"

    @property
    def android_sdk_dir(self) -> Path:
        return self.android_root / "sdk"

    @property
    def android_ndk_dir(self) -> Path:
        return self.android_sdk_dir / "ndk" / ANDROID_NDK_VERSION

    @property
    def aqt_qt_root(self) -> Path:
        # aqt's --outputdir: it lays out <outputdir>/<version>/<arch>/.
        return self.android_root / "qt"

    @property
    def qt_host_dir(self) -> Path:
        return self.aqt_qt_root / ANDROID_QT_VERSION / _aqt_host_install_dirname()

    def qt_android_dir_for(self, abi: str) -> Path:
        return (self.aqt_qt_root / ANDROID_QT_VERSION
                / qt_install_dirname_for_abi(abi))

    @property
    def qt_android_dir(self) -> Path:
        # Primary ABI's prefix doubles as the CMAKE_TOOLCHAIN_FILE anchor;
        # the toolchain file is identical across ABIs.
        return self.qt_android_dir_for(self.android_abis[0])

    @property
    def aqt_host(self) -> str:
        return _aqt_host()

    @property
    def aqt_host_arch(self) -> str:
        return _aqt_host_arch()

    @property
    def adoptium_os(self) -> str:
        return _adoptium_os()

    @property
    def adoptium_arch(self) -> str:
        return _adoptium_arch()
