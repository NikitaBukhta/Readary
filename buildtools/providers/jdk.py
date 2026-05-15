import os
import shutil
import tempfile
import urllib.request
import zipfile
from pathlib import Path

from buildtools.config import JDK_FEATURE_VERSION, ProjectConfig
from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.shell import Shell


class JdkProvider(ToolProvider):
    """Ensures a JDK 17 for Gradle. Order: managed install, JAVA_HOME, then download Temurin."""

    _ADOPTIUM_URL = (
        "https://api.adoptium.net/v3/binary/latest/"
        "{feature}/ga/{os}/{arch}/jdk/hotspot/normal/eclipse?project=jdk"
    )

    def __init__(self, shell: Shell, config: ProjectConfig):
        super().__init__(shell)
        self.config = config

    def ensure(self) -> Path:
        existing = self._probe_existing()
        if existing:
            return existing

        return self._download_and_extract()

    def java_home(self) -> Path:
        return self.ensure()

    # ---- internals --------------------------------------------------------

    def _probe_existing(self) -> Path | None:
        managed = self.config.jdk_dir
        if self._is_valid_jdk(managed):
            return managed

        env_home = os.environ.get("JAVA_HOME")
        if env_home:
            candidate = Path(env_home)
            if self._is_valid_jdk(candidate):
                if self._jdk_major(candidate) >= int(JDK_FEATURE_VERSION):
                    return candidate
                print(f"  Ignoring JAVA_HOME={candidate} (version too old, "
                      f"need >= {JDK_FEATURE_VERSION})")
        return None

    def _is_valid_jdk(self, path: Path) -> bool:
        if not path.is_dir():
            return False
        javac = path / "bin" / ("javac.exe" if self.config.is_windows else "javac")
        return javac.exists()

    def _jdk_major(self, jdk_home: Path) -> int:
        java = jdk_home / "bin" / ("java.exe" if self.config.is_windows else "java")
        # `java -version` writes to stderr — call subprocess directly so we get both streams.
        import subprocess  # nosec B404 — local helper, args derived from verified JDK install
        try:
            res = subprocess.run(  # nosec B603 — argv from build code, not untrusted input
                [str(java), "-version"],
                capture_output=True, text=True, timeout=5,
            )
        except (OSError, subprocess.SubprocessError):
            return 0
        out = (res.stderr or "") + (res.stdout or "")
        import re
        m = re.search(r'version "(\d+)', out)
        return int(m.group(1)) if m else 0

    def _download_and_extract(self) -> Path:
        target_root = self.config.android_root
        target_root.mkdir(parents=True, exist_ok=True)
        url = self._ADOPTIUM_URL.format(
            feature=JDK_FEATURE_VERSION,
            os=self.config.adoptium_os,
            arch=self.config.adoptium_arch,
        )
        archive_suffix = ".zip" if self.config.is_windows else ".tar.gz"

        print(f"Downloading Temurin JDK {JDK_FEATURE_VERSION} from Adoptium...")
        with tempfile.NamedTemporaryFile(
            suffix=archive_suffix, delete=False,
        ) as tf:
            archive_path = Path(tf.name)
        try:
            self._download(url, archive_path)
            print(f"  Extracting to {target_root}...")
            extracted_root = self._extract(archive_path, target_root)
        finally:
            archive_path.unlink(missing_ok=True)

        # Adoptium extracts to e.g. "jdk-17.0.10+7"; rename to a stable "jdk-17".
        managed = self.config.jdk_dir
        if managed.exists():
            shutil.rmtree(managed)
        extracted_root.rename(managed)

        if not self._is_valid_jdk(managed):
            raise ToolNotFoundError(
                f"Extracted JDK at {managed} does not look valid "
                "(missing bin/javac)."
            )
        print(f"  JDK ready at {managed}")
        return managed

    def _download(self, url: str, dest: Path) -> None:
        req = urllib.request.Request(  # noqa: S310 — fixed Adoptium API endpoint, not user input
            url, headers={"User-Agent": "BeeLibrary-bootstrap"},
        )
        with urllib.request.urlopen(req) as resp:  # noqa: S310 nosec B310
            with dest.open("wb") as out:
                shutil.copyfileobj(resp, out)

    def _extract(self, archive: Path, dest_root: Path) -> Path:
        """Extract archive into dest_root and return its top-level dir."""
        before = {p for p in dest_root.iterdir() if p.is_dir()}
        if archive.suffix == ".zip":
            with zipfile.ZipFile(archive) as zf:
                zf.extractall(dest_root)
        else:
            import tarfile
            with tarfile.open(archive, "r:gz") as tf:
                tf.extractall(dest_root)  # noqa: S202 — Adoptium-signed tarball
        after = {p for p in dest_root.iterdir() if p.is_dir()}
        new_dirs = after - before
        if not new_dirs:
            raise ToolNotFoundError(
                "Failed to extract JDK archive (no new top-level dir)"
            )
        for d in new_dirs:
            if d.name.startswith("jdk-"):
                return d
        return next(iter(new_dirs))
