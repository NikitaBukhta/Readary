import os
import shutil
import subprocess  # nosec B404 — runs Google's sdkmanager binary on the user's machine
import tempfile
import urllib.request
import zipfile
from pathlib import Path

from buildtools.config import (
    ANDROID_BUILD_TOOLS_VERSION,
    ANDROID_NDK_VERSION,
    ANDROID_PLATFORM_API,
    ProjectConfig,
)
from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.providers.jdk import JdkProvider
from buildtools.shell import Shell


class AndroidSdkProvider(ToolProvider):
    """Installs Android cmdline-tools then sdkmanager-installs platform/build-tools/NDK for Qt 6.8."""

    # cmdline-tools self-updates via sdkmanager, so this is just a bootstrap version.
    _CMDLINE_TOOLS_BUILD = "11076708"
    _CMDLINE_TOOLS_URL = (
        "https://dl.google.com/android/repository/"
        "commandlinetools-{os}-{build}_latest.zip"
    )

    def __init__(self, shell: Shell, config: ProjectConfig, jdk: JdkProvider):
        super().__init__(shell)
        self.config = config
        self.jdk = jdk

    def ensure(self) -> Path:
        sdk_root = self.config.android_sdk_dir
        sdk_root.mkdir(parents=True, exist_ok=True)

        sdkmanager = self._ensure_cmdline_tools(sdk_root)
        self._install_packages(sdkmanager, sdk_root)

        if not self.config.android_ndk_dir.exists():
            raise ToolNotFoundError(
                f"NDK installation failed (expected at "
                f"{self.config.android_ndk_dir})"
            )
        return sdk_root

    def ndk_root(self) -> Path:
        return self.config.android_ndk_dir

    def sdk_root(self) -> Path:
        return self.config.android_sdk_dir

    # ---- internals --------------------------------------------------------

    def _ensure_cmdline_tools(self, sdk_root: Path) -> Path:
        latest = sdk_root / "cmdline-tools" / "latest"
        sdkmanager = latest / "bin" / self._sdkmanager_name()
        if sdkmanager.exists():
            return sdkmanager

        print("Installing Android command-line tools...")
        url = self._CMDLINE_TOOLS_URL.format(
            os=self._cmdline_tools_os(),
            build=self._CMDLINE_TOOLS_BUILD,
        )
        with tempfile.NamedTemporaryFile(suffix=".zip", delete=False) as tf:
            archive_path = Path(tf.name)
        try:
            self._download(url, archive_path)
            with tempfile.TemporaryDirectory() as scratch_str:
                scratch = Path(scratch_str)
                with zipfile.ZipFile(archive_path) as zf:
                    zf.extractall(scratch)
                # Archive's top-level "cmdline-tools/" goes under <SDK>/cmdline-tools/latest/.
                src = scratch / "cmdline-tools"
                if not src.is_dir():
                    raise ToolNotFoundError(
                        "commandlinetools archive layout unexpected "
                        "(missing 'cmdline-tools/' top-level dir)"
                    )
                latest.parent.mkdir(parents=True, exist_ok=True)
                if latest.exists():
                    shutil.rmtree(latest)
                shutil.move(str(src), str(latest))
        finally:
            archive_path.unlink(missing_ok=True)

        if not sdkmanager.exists():
            raise ToolNotFoundError(
                f"sdkmanager not found at {sdkmanager} after extraction"
            )
        return sdkmanager

    def _install_packages(self, sdkmanager: Path, sdk_root: Path) -> None:
        packages = [
            "platform-tools",
            f"platforms;android-{ANDROID_PLATFORM_API}",
            f"build-tools;{ANDROID_BUILD_TOOLS_VERSION}",
            f"ndk;{ANDROID_NDK_VERSION}",
        ]
        if all(self._package_installed(pkg, sdk_root) for pkg in packages):
            return

        print("Installing Android SDK packages via sdkmanager...")
        for pkg in packages:
            print(f"  - {pkg}")

        env = os.environ.copy()
        env["JAVA_HOME"] = str(self.jdk.java_home())
        env["ANDROID_SDK_ROOT"] = str(sdk_root)
        # avdmanager and friends look for sdkmanager via PATH.
        bin_dir = sdkmanager.parent
        env["PATH"] = f"{bin_dir}{os.pathsep}{env.get('PATH', '')}"

        cmd = [str(sdkmanager), f"--sdk_root={sdk_root}", *packages]
        # Auto-accept licenses (~6 prompts; "y\n" * 20 absorbs future ones).
        proc = subprocess.run(  # nosec B603 — sdkmanager is Google-signed and resolved from a managed path
            cmd, input="y\n" * 20, text=True,
            env=env, check=False,
        )
        if proc.returncode != 0:
            raise ToolNotFoundError(
                f"sdkmanager failed (exit {proc.returncode}). "
                "Re-run bootstrap; if the failure persists, accept licenses "
                f"manually: {sdkmanager} --licenses"
            )

    def _package_installed(self, pkg: str, sdk_root: Path) -> bool:
        if pkg == "platform-tools":
            adb = sdk_root / "platform-tools" / self._exe("adb")
            return adb.exists()
        if pkg.startswith("platforms;"):
            api = pkg.split(";", 1)[1]
            return (sdk_root / "platforms" / api).is_dir()
        if pkg.startswith("build-tools;"):
            ver = pkg.split(";", 1)[1]
            return (sdk_root / "build-tools" / ver).is_dir()
        if pkg.startswith("ndk;"):
            ver = pkg.split(";", 1)[1]
            return (sdk_root / "ndk" / ver).is_dir()
        return False

    def _sdkmanager_name(self) -> str:
        return "sdkmanager.bat" if self.config.is_windows else "sdkmanager"

    def _cmdline_tools_os(self) -> str:
        if self.config.is_windows:
            return "win"
        import platform
        return "mac" if platform.system() == "Darwin" else "linux"

    def _exe(self, name: str) -> str:
        return f"{name}.exe" if self.config.is_windows else name

    def _download(self, url: str, dest: Path) -> None:
        req = urllib.request.Request(  # noqa: S310 — fixed Google CDN endpoint, not user input
            url, headers={"User-Agent": "BeeLibrary-bootstrap"},
        )
        with urllib.request.urlopen(req) as resp:  # noqa: S310 nosec B310
            with dest.open("wb") as out:
                shutil.copyfileobj(resp, out)
