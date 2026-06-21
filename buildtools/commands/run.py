import os
import subprocess  # nosec B404 — launches the just-built app binary
import time
from pathlib import Path

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError, ToolNotFoundError
from buildtools.providers.android_sdk import AndroidSdkProvider
from buildtools.providers.jdk import JdkProvider
from buildtools.shell import Shell

APP_NAME = "Readary"
ANDROID_PACKAGE = "org.qtproject.example.Readary"
ANDROID_ACTIVITY = "org.qtproject.qt.android.bindings.QtActivity"
LOGCAT_PID_TIMEOUT_S = 8.0


class RunCommand(Command):
    """Runs the built application with the correct environment."""

    name = "run"
    summary = "Run the built application"

    def __init__(self, config: ProjectConfig, shell: Shell,
                 jdk: JdkProvider, android_sdk: AndroidSdkProvider):
        self.config = config
        self.shell = shell
        self.jdk = jdk
        self.android_sdk = android_sdk

    def execute(self) -> None:
        if self.config.is_android:
            self._execute_android()
        else:
            self._execute_desktop()

    # ---- desktop ----------------------------------------------------------

    def _execute_desktop(self) -> None:
        executable = self._find_executable()

        build_root = self.config.cmake_build_dir

        env = os.environ.copy()
        env["QT_PLUGIN_PATH"] = str(self.config.qt_plugin_dir)
        env["QML2_IMPORT_PATH"] = str(self.config.qt_qml_dir) + os.pathsep + str(build_root)
        env["PATH"] = str(self.config.qt_bin_dir) + os.pathsep + env.get("PATH", "")

        print(f"=== Running ({self.config.build_type}) ===")
        print(f">>> {executable}")
        subprocess.run([executable], env=env)  # nosec B603 — executable is the built Readary binary, path verified above

    def _find_executable(self) -> str:
        build_root = self.config.cmake_build_dir
        exe = f"{APP_NAME}.exe" if self.config.is_windows else APP_NAME

        # Multi-config (VS) puts the binary under build/<preset>/<Config>/.
        multi_config = build_root / self.config.build_type / exe
        if multi_config.exists():
            return str(multi_config)

        single_config = build_root / exe
        if single_config.exists():
            return str(single_config)

        raise BuildError(
            "Executable not found. "
            "Run `python bootstrap.py compile` first."
        )

    # ---- android ----------------------------------------------------------

    def _execute_android(self) -> None:
        adb = self._adb_path()
        apk = self._find_apk()
        if apk is None:
            raise BuildError(
                "APK not found. Run "
                "`python bootstrap.py compile -d android"
                f"{' --release' if self.config.release else ''}` first."
            )

        # Resolve target up front so errors are clearer than adb's defaults.
        target_args = self._resolve_device_args(adb)

        print(f"=== Installing APK on device ({self.config.build_type}) ===")
        print(f">>> {adb} {' '.join(target_args)} install -r {apk}")
        subprocess.run(  # nosec B603 — adb is from a managed install, args verified above
            [str(adb), *target_args, "install", "-r", str(apk)], check=True,
        )

        # Clear logcat so the stream below shows only this run.
        subprocess.run(  # nosec B603 — adb is from a managed install, args constants
            [str(adb), *target_args, "logcat", "-c"], check=False,
        )

        print(f"\n=== Launching {ANDROID_PACKAGE} ===")
        subprocess.run(  # nosec B603 — adb is from a managed install, args constants
            [
                str(adb), *target_args, "shell", "am", "start", "-n",
                f"{ANDROID_PACKAGE}/{ANDROID_ACTIVITY}",
            ], check=True,
        )

        pid = self._wait_for_pid(adb, target_args)
        if pid is None:
            print(f"\nWARNING: could not find PID for {ANDROID_PACKAGE} "
                  f"within {LOGCAT_PID_TIMEOUT_S:.0f}s. The app may have "
                  "crashed at startup; falling back to tag-based logcat.")
            self._stream_logcat_by_tag(adb, target_args)
        else:
            print(f"\n=== Streaming logcat (pid {pid}) — Ctrl+C to stop ===")
            self._stream_logcat_by_pid(adb, target_args, pid)

    def _wait_for_pid(self, adb: Path, target_args: list[str]) -> int | None:
        """Poll ``pidof`` until the app's process appears, up to a timeout."""
        deadline = time.monotonic() + LOGCAT_PID_TIMEOUT_S
        while time.monotonic() < deadline:
            result = subprocess.run(  # nosec B603 — adb is from a managed install, args constants
                [str(adb), *target_args, "shell", "pidof", ANDROID_PACKAGE],
                capture_output=True, text=True, check=False,
            )
            pid_str = result.stdout.strip().split()
            if pid_str and pid_str[0].isdigit():
                return int(pid_str[0])
            time.sleep(0.3)
        return None

    def _stream_logcat_by_pid(self, adb: Path, target_args: list[str],
                              pid: int) -> None:
        try:
            subprocess.run(  # nosec B603 — adb is from a managed install, args derived from verified pid
                [str(adb), *target_args, "logcat", f"--pid={pid}"],
                check=False,
            )
        except KeyboardInterrupt:
            print("\nStopped streaming logs. App keeps running on device.")

    def _stream_logcat_by_tag(self, adb: Path, target_args: list[str]) -> None:
        try:
            subprocess.run(  # nosec B603 — adb is from a managed install, args constants
                [
                    str(adb), *target_args, "logcat",
                    "*:S", "Readary:V", "Qt:V", "QtCore:V", "QtQml:V",
                    "AndroidRuntime:E", "DEBUG:V", "libc:E",
                ], check=False,
            )
        except KeyboardInterrupt:
            print("\nStopped streaming logs.")

    def _resolve_device_args(self, adb: Path) -> list[str]:
        """Return adb args targeting exactly one device.

        Honors ``ANDROID_SERIAL`` if set; otherwise probes ``adb devices``
        and refuses to guess when there is more than one ready device.
        """
        explicit = os.environ.get("ANDROID_SERIAL")
        devices = self._list_devices(adb)

        if not devices:
            raise BuildError(
                "No Android devices connected.\n"
                "  - Plug in a device with USB debugging enabled, "
                "or start an emulator.\n"
                f"  - Verify with: {adb} devices"
            )

        if explicit:
            if explicit not in devices:
                raise BuildError(
                    f"ANDROID_SERIAL={explicit!r} is set but that serial is "
                    f"not in `adb devices`. Visible: {sorted(devices)}"
                )
            print(f"Targeting device {explicit} (from ANDROID_SERIAL)")
            return ["-s", explicit]

        if len(devices) == 1:
            serial = next(iter(devices))
            print(f"Targeting device {serial}")
            return ["-s", serial]

        raise BuildError(
            "Multiple Android devices are connected; pick one by setting "
            "ANDROID_SERIAL. Visible:\n  "
            + "\n  ".join(sorted(devices))
            + f"\nOr disconnect the rest and run: {adb} devices"
        )

    def _list_devices(self, adb: Path) -> set[str]:
        """Serials in `device` state (skips offline/unauthorized/no-permission)."""
        result = subprocess.run(  # nosec B603 — adb is from a managed install, no user-controlled args
            [str(adb), "devices"], capture_output=True, text=True, check=False,
        )
        ready: set[str] = set()
        # adb devices output: "List of devices attached\n<serial>\t<state>\n...".
        for line in result.stdout.splitlines()[1:]:
            line = line.strip()
            if not line:
                continue
            parts = line.split("\t")
            if len(parts) == 2 and parts[1].strip() == "device":
                ready.add(parts[0])
        return ready

    def _adb_path(self) -> Path:
        adb_name = "adb.exe" if self.config.is_windows else "adb"
        managed = self.config.android_sdk_dir / "platform-tools" / adb_name
        if managed.exists():
            return managed
        found = self.shell.which("adb")
        if found:
            return Path(found)
        raise ToolNotFoundError(
            "adb not found. Run `python bootstrap.py bootstrap -d android` "
            "to install Android platform-tools."
        )

    def _find_apk(self) -> Path | None:
        build_dir = self.config.cmake_build_dir
        roots = [
            build_dir / "android-build" / "build" / "outputs" / "apk",
            build_dir / "android-Readary" / "build" / "outputs" / "apk",
        ]
        for root in roots:
            if not root.exists():
                continue
            kind = "release" if self.config.release else "debug"
            preferred = root / kind
            if preferred.exists():
                for apk in preferred.rglob("*.apk"):
                    return apk
            for apk in root.rglob("*.apk"):
                return apk
        return None
