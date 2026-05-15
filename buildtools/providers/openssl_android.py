from pathlib import Path

from buildtools.config import ProjectConfig
from buildtools.errors import ToolNotFoundError
from buildtools.providers.base import ToolProvider
from buildtools.providers.git import GitProvider
from buildtools.shell import Shell


class OpensslAndroidProvider(ToolProvider):
    """Clones KDAB's prebuilt libcrypto/libssl for Android — Qt's tls_qopensslbackend
    plugin needs them but Qt doesn't ship them. KDAB lays them out under ssl_3/<abi>/."""

    _REPO = "https://github.com/KDAB/android_openssl"

    def __init__(self, shell: Shell, config: ProjectConfig, git: GitProvider):
        super().__init__(shell)
        self.config = config
        self.git = git

    def ensure(self) -> Path:
        repo_dir = self._repo_dir()
        if not repo_dir.exists():
            self._clone(repo_dir)

        for abi in self.config.android_abis:
            abi_dir = self._abi_dir(repo_dir, abi)
            if not abi_dir.is_dir():
                raise ToolNotFoundError(
                    f"KDAB android_openssl checkout is missing the {abi}"
                    f" directory at {abi_dir}. Re-run with a clean checkout."
                )
            crypto, ssl = self._libs_at(abi_dir)
            if not crypto.exists() or not ssl.exists():
                raise ToolNotFoundError(
                    f"OpenSSL .so files missing for {abi} after clone:\n"
                    f"  crypto: {crypto}\n  ssl:    {ssl}"
                )
        return repo_dir

    def libs_for(self, abi: str) -> tuple[Path, Path]:
        return self._libs_at(self._abi_dir(self._repo_dir(), abi))

    def all_libs(self) -> list[Path]:
        # androiddeployqt picks each lib's ABI from its parent dir name,
        # so a flat list across ABIs is enough.
        out: list[Path] = []
        for abi in self.config.android_abis:
            crypto, ssl = self.libs_for(abi)
            out.append(crypto)
            out.append(ssl)
        return out

    # ---- internals --------------------------------------------------------

    def _repo_dir(self) -> Path:
        return self.config.android_root / "android_openssl"

    def _abi_dir(self, repo_dir: Path, abi: str) -> Path:
        return repo_dir / "ssl_3" / abi

    @staticmethod
    def _libs_at(abi_dir: Path) -> tuple[Path, Path]:
        return abi_dir / "libcrypto_3.so", abi_dir / "libssl_3.so"

    def _clone(self, dest: Path) -> None:
        self.git.ensure()
        dest.parent.mkdir(parents=True, exist_ok=True)
        print(f"Cloning KDAB android_openssl into {dest}...")
        self.shell.run([
            "git", "clone", "--depth=1", self._REPO, str(dest),
        ])
