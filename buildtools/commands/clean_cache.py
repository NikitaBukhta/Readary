import shutil
from pathlib import Path

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig


def _dir_size(path: Path) -> int:
    """Total bytes under `path` (best-effort; unreadable entries are skipped)."""
    total = 0
    for p in path.rglob("*"):
        try:
            if p.is_file():
                total += p.stat().st_size
        except OSError:
            pass
    return total


class CleanCacheCommand(Command):
    """Reclaims vcpkg scratch/cache space without touching installed deps.

    Bootstrap builds Qt from source through vcpkg, which leaves large
    intermediate trees behind that the working build never reads again:
    buildtrees (object files), packages (per-port staging), and the downloads
    source-tarball cache. This removes those. The installed deps the project
    links against (deps_dir) are left intact — use `clean` to drop those.

    --deep additionally clears the vcpkg binary cache; the next bootstrap then
    rebuilds every port from source instead of restoring prebuilt archives.
    """

    name = "clean-cache"
    summary = "Remove vcpkg build caches (buildtrees/packages/downloads)"

    def __init__(self, config: ProjectConfig):
        self.config = config

    def execute(self) -> None:
        # Regenerable scratch — always safe to drop, reclaims the most space.
        targets: dict[str, Path] = {
            "Buildtrees":         self.config.vcpkg_buildtrees_dir,
            "Buildtrees (in-tree)": self.config.vcpkg_default_buildtrees_dir,
            "Packages":           self.config.vcpkg_packages_dir,
            "Downloads":          self.config.vcpkg_downloads_dir,
        }
        if self.config.clean_cache_deep:
            targets["Binary cache"] = self.config.vcpkg_binary_cache_dir

        freed = 0
        removed = []
        for label, path in targets.items():
            if not path.exists():
                continue
            size = _dir_size(path)
            print(f"  Removing {label}: {path} ({size / 1024**3:.2f} GB)")
            shutil.rmtree(path, ignore_errors=True)
            freed += size
            removed.append(label)

        if removed:
            print(f"\nCleaned: {', '.join(removed)}")
            print(f"Reclaimed ~{freed / 1024**3:.2f} GB.")
        else:
            print("Nothing to clean.")

        if not self.config.clean_cache_deep:
            print("\n(vcpkg binary cache kept for fast re-bootstrap; pass "
                  "--deep to clear it too.)")
