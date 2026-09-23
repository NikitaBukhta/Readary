"""Unit tests for `--skip-vcpkg` on `bootstrap` and `compile`.

Run from the project root:

    python -m unittest discover buildtools/tests

`Shell` is faked, so the tests assert on the configure argv each case produces
and on whether a configure runs at all. The build tree is a stub
`CMakeCache.txt` in a temporary directory.
"""
from __future__ import annotations

import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from buildtools.cli import CLI
from buildtools.commands.bootstrap import BootstrapCommand
from buildtools.commands.compile import CompileCommand
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError


class FakeShell:
    """Records commands instead of running them."""

    def __init__(self):
        self.commands: list[list[str]] = []

    def run(self, cmd, env=None):
        self.commands.append([str(c) for c in cmd])
        return None


class FakeClangTidy:
    @staticmethod
    def ensure() -> Path:
        return Path("clang-tidy.exe")


class FakeVcpkg:
    def __init__(self, installed: set[str]):
        self._installed = installed

    def is_port_installed(self, port: str) -> bool:
        return port in self._installed


def _config(root: Path, **kwargs) -> ProjectConfig:
    cfg = ProjectConfig(project_dir=root, **kwargs)
    object.__setattr__(cfg, "deps_dir", root / "deps")
    return cfg


def _write_cache(cfg: ProjectConfig, **variables: str) -> None:
    build_dir = cfg.cmake_build_dir
    build_dir.mkdir(parents=True, exist_ok=True)
    lines = [f"{name}:BOOL={value}" for name, value in variables.items()]
    (build_dir / "CMakeCache.txt").write_text("\n".join(lines) + "\n",
                                              encoding="utf-8")


def _compile(cfg: ProjectConfig, shell: FakeShell) -> CompileCommand:
    return CompileCommand(cfg, shell, None, None, None, FakeClangTidy(), None,
                          None, None, None, None)


def _bootstrap(cfg: ProjectConfig, vcpkg: FakeVcpkg) -> BootstrapCommand:
    return BootstrapCommand(cfg, FakeShell(), None, None, vcpkg, None, None,
                            None, None, None, None, None, None, None)


def _define(argv: list[str], name: str) -> str | None:
    prefix = f"-D{name}="
    for arg in argv:
        if arg.startswith(prefix):
            return arg[len(prefix):]
    return None


class CompileSyncTest(unittest.TestCase):

    def setUp(self):
        self._tmp = TemporaryDirectory()
        self.root = Path(self._tmp.name)

    def tearDown(self):
        self._tmp.cleanup()

    def _sync(self, cfg: ProjectConfig) -> FakeShell:
        shell = FakeShell()
        _compile(cfg, shell)._sync_cache_settings(Path("cmake.exe"), None)
        return shell

    def test_matching_cache_does_not_reconfigure(self):
        cfg = _config(self.root)
        _write_cache(cfg, ENABLE_ANALYZE="ON", VCPKG_MANIFEST_INSTALL="ON")

        self.assertEqual(self._sync(cfg).commands, [])

    def test_skip_vcpkg_reconfigures_with_the_install_off(self):
        cfg = _config(self.root, skip_vcpkg=True)
        _write_cache(cfg, ENABLE_ANALYZE="ON", VCPKG_MANIFEST_INSTALL="ON")

        commands = self._sync(cfg).commands

        self.assertEqual(len(commands), 1)
        self.assertEqual(_define(commands[0], "VCPKG_MANIFEST_INSTALL"), "OFF")
        # The analysis setting is re-stated, not dropped.
        self.assertEqual(_define(commands[0], "ENABLE_ANALYZE"), "ON")

    def test_skip_vcpkg_again_is_a_no_op(self):
        cfg = _config(self.root, skip_vcpkg=True)
        _write_cache(cfg, ENABLE_ANALYZE="ON", VCPKG_MANIFEST_INSTALL="OFF")

        self.assertEqual(self._sync(cfg).commands, [])

    def test_plain_compile_turns_an_earlier_off_back_on(self):
        # Left at OFF, CMake's own mid-build reconfigure would keep skipping
        # vcpkg.json — a newly added dependency would never install.
        cfg = _config(self.root)
        _write_cache(cfg, ENABLE_ANALYZE="ON", VCPKG_MANIFEST_INSTALL="OFF")

        commands = self._sync(cfg).commands

        self.assertEqual(len(commands), 1)
        self.assertEqual(_define(commands[0], "VCPKG_MANIFEST_INSTALL"), "ON")

    def test_missing_cache_entry_counts_as_the_default_on(self):
        cfg = _config(self.root)
        _write_cache(cfg, ENABLE_ANALYZE="ON")

        self.assertEqual(self._sync(cfg).commands, [])

    def test_analyze_change_carries_the_manifest_setting(self):
        cfg = _config(self.root, skip_analyze=True, skip_vcpkg=True)
        _write_cache(cfg, ENABLE_ANALYZE="ON", VCPKG_MANIFEST_INSTALL="OFF")

        commands = self._sync(cfg).commands

        self.assertEqual(len(commands), 1)
        self.assertEqual(_define(commands[0], "ENABLE_ANALYZE"), "OFF")
        self.assertEqual(_define(commands[0], "VCPKG_MANIFEST_INSTALL"), "OFF")


class BootstrapGuardTest(unittest.TestCase):

    def setUp(self):
        self._tmp = TemporaryDirectory()
        self.root = Path(self._tmp.name)

    def tearDown(self):
        self._tmp.cleanup()

    def test_skip_vcpkg_without_installed_deps_fails_early(self):
        cfg = _config(self.root, skip_vcpkg=True)

        with self.assertRaises(BuildError) as raised:
            _bootstrap(cfg, FakeVcpkg(installed=set()))._require_installed_deps()

        self.assertIn("--skip-vcpkg", str(raised.exception))

    def test_skip_vcpkg_with_installed_deps_proceeds(self):
        cfg = _config(self.root, skip_vcpkg=True)

        _bootstrap(cfg, FakeVcpkg(installed={"qtbase"}))._require_installed_deps()


class CliTest(unittest.TestCase):

    def test_flag_parses_on_bootstrap_and_compile(self):
        parser = CLI().parser
        for command in ("bootstrap", "compile"):
            with self.subTest(command=command):
                self.assertTrue(
                    parser.parse_args([command, "--skip-vcpkg"]).skip_vcpkg)
                self.assertFalse(parser.parse_args([command]).skip_vcpkg)


if __name__ == "__main__":
    unittest.main()
