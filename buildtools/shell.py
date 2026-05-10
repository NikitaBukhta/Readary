import shutil
import subprocess  # nosec B404 — runs vetted toolchain binaries on the user's machine
from pathlib import Path


class Shell:
    """Runs shell commands with logging."""

    def run(self, cmd: list[str | Path],
            env: dict[str, str] | None = None) -> subprocess.CompletedProcess[bytes]:
        str_cmd = [str(c) for c in cmd]
        print(f">>> {' '.join(str_cmd)}")
        return subprocess.run(str_cmd, check=True, env=env)  # nosec B603 — argv from build code, not untrusted input

    @staticmethod
    def which(name: str) -> str | None:
        return shutil.which(name)

    def get_output(self, cmd: list[str | Path],
                   timeout: int = 5) -> str | None:
        """Run a command and return its first stdout line, or None on failure."""
        try:
            result = subprocess.run(  # nosec B603 — argv from build code, not untrusted input
                [str(c) for c in cmd],
                capture_output=True, text=True, timeout=timeout,
            )
            lines = result.stdout.strip().splitlines()
            return lines[0] if lines else None
        except (subprocess.SubprocessError, FileNotFoundError, OSError):
            return None
