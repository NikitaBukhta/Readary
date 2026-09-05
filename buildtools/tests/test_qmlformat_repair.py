"""Unit tests for qmlformat resolution and the vcpkg repair it triggers.

Run from the project root:

    python -m unittest discover buildtools/tests

No vcpkg, no Qt, no network: `Shell` is faked, so the tests assert on the
argv that would be run and on which branch the code takes.
"""
from __future__ import annotations

import os
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from buildtools.commands.bootstrap import BootstrapCommand
from buildtools.config import ProjectConfig
from buildtools.errors import ToolNotFoundError
from buildtools.providers.qml_format import QmlFormatProvider
from buildtools.providers.vcpkg import VcpkgProvider


class FakeShell:
    """Records commands instead of running them; `which` finds nothing.

    Set `path_qmlformat` to stand in for another Qt on PATH.
    """

    def __init__(self):
        self.commands: list[list[str]] = []
        self.on_run = None
        self.path_qmlformat: str | None = None

    def run(self, cmd, env=None):
        self.commands.append([str(c) for c in cmd])
        if self.on_run is not None:
            self.on_run(self.commands[-1])
        return None

    def which(self, name: str):
        return self.path_qmlformat if name == "qmlformat" else None


def _config(root: Path) -> ProjectConfig:
    cfg = ProjectConfig(project_dir=root)
    # Keep the fixture self-contained: point the dependency tree inside tmp.
    object.__setattr__(cfg, "deps_dir", root / "deps")
    object.__setattr__(cfg, "vcpkg_dir", root / "vcpkg")
    return cfg


def _make_qmlformat(cfg: ProjectConfig) -> Path:
    tools = cfg.qt_tools_dir
    tools.mkdir(parents=True, exist_ok=True)
    exe = tools / ("qmlformat.exe" if cfg.is_windows else "qmlformat")
    exe.write_text("", encoding="utf-8")
    return exe


class _EnvIsolatedTestCase(unittest.TestCase):
    """Restores os.environ: `VcpkgProvider.ensure` exports VCPKG_ROOT."""

    def setUp(self):
        snapshot = os.environ.copy()
        self.addCleanup(lambda: (os.environ.clear(),
                                 os.environ.update(snapshot)))


class QmlFormatProviderTests(unittest.TestCase):
    def test_locate_returns_none_when_not_installed(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            provider = QmlFormatProvider(FakeShell(), cfg)
            self.assertIsNone(provider.locate())

    def test_locate_finds_the_vcpkg_qt_host_tool(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            expected = _make_qmlformat(cfg)
            provider = QmlFormatProvider(FakeShell(), cfg)
            self.assertEqual(provider.locate(), expected)

    def test_locate_accepts_a_qmlformat_from_path(self):
        with TemporaryDirectory() as tmp:
            shell = FakeShell()
            shell.path_qmlformat = "C:/Qt/6.8.0/msvc2022_64/bin/qmlformat.exe"
            provider = QmlFormatProvider(shell, _config(Path(tmp)))
            self.assertEqual(provider.locate(), Path(shell.path_qmlformat))

    def test_vcpkg_tool_path_ignores_path_entirely(self):
        # It names where CMake imports the tool from, installed or not.
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            shell = FakeShell()
            shell.path_qmlformat = "C:/Qt/6.8.0/msvc2022_64/bin/qmlformat.exe"
            provider = QmlFormatProvider(shell, cfg)
            self.assertEqual(provider.vcpkg_tool_path(),
                             cfg.qt_tools_dir / provider._executable_name())

    def test_ensure_raises_and_names_the_owning_port(self):
        with TemporaryDirectory() as tmp:
            provider = QmlFormatProvider(FakeShell(), _config(Path(tmp)))
            with self.assertRaises(ToolNotFoundError) as ctx:
                provider.ensure()
            self.assertIn("qtdeclarative", str(ctx.exception))

    def test_owning_port_is_qtdeclarative(self):
        # bootstrap reinstalls this port to restore the tool; vcpkg.json must
        # keep depending on it.
        self.assertEqual(QmlFormatProvider.vcpkg_port, "qtdeclarative")
        manifest = Path(__file__).resolve().parents[2] / "vcpkg.json"
        self.assertIn(QmlFormatProvider.vcpkg_port,
                      manifest.read_text(encoding="utf-8"))


class VcpkgPortStateTests(_EnvIsolatedTestCase):
    @staticmethod
    def _register(cfg: ProjectConfig, port: str, version: str = "6.8.0") -> None:
        info = cfg.deps_dir / "vcpkg" / "info"
        info.mkdir(parents=True, exist_ok=True)
        (info / f"{port}_{version}_{cfg.vcpkg_triplet}.list").write_text(
            "", encoding="utf-8")

    def test_port_is_not_installed_on_a_fresh_tree(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            provider = VcpkgProvider(FakeShell(), cfg, git=None)
            self.assertFalse(provider.is_port_installed("qtdeclarative"))

    def test_port_counts_as_installed_from_its_listing_alone(self):
        # The whole point: the files can be gone and vcpkg still says installed.
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            self._register(cfg, "qtdeclarative")
            provider = VcpkgProvider(FakeShell(), cfg, git=None)
            self.assertTrue(provider.is_port_installed("qtdeclarative"))
            self.assertFalse(provider.is_port_installed("qtsvg"))


class VcpkgRemovePortTests(_EnvIsolatedTestCase):
    def test_remove_port_forces_classic_mode_and_the_project_install_root(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            shell = FakeShell()
            # Pretend vcpkg is already cloned so `ensure` skips the clone path.
            cfg.vcpkg_dir.mkdir(parents=True)
            cfg.vcpkg_executable().write_text("", encoding="utf-8")

            VcpkgProvider(shell, cfg, git=None).remove_port("qtdeclarative")

            self.assertEqual(len(shell.commands), 1)
            argv = shell.commands[0]
            self.assertIn("remove", argv)
            self.assertIn(f"qtdeclarative:{cfg.vcpkg_triplet}", argv)
            # Without --classic the project's vcpkg.json puts the CLI in
            # manifest mode, where `remove` is rejected outright.
            self.assertIn("--classic", argv)
            self.assertIn("--recurse", argv)
            self.assertIn(f"--x-install-root={cfg.deps_dir.as_posix()}", argv)


class BootstrapRepairQtHostToolsTests(_EnvIsolatedTestCase):
    """The repair runs *before* the configure: Qt's CMake package imports
    Qt6::qmlformat by path, so a configure with the tool missing dies before any
    later step could fix it.
    """

    @staticmethod
    def _command(cfg: ProjectConfig, shell: FakeShell) -> BootstrapCommand:
        # Only the host-tool repair is under test; the rest of the graph is unused.
        cmd = BootstrapCommand.__new__(BootstrapCommand)
        cmd.config = cfg
        cmd.shell = shell
        cmd.qml_format = QmlFormatProvider(shell, cfg)
        cmd.vcpkg = VcpkgProvider(shell, cfg, git=None)
        return cmd

    @staticmethod
    def _fake_vcpkg(cfg: ProjectConfig) -> None:
        cfg.vcpkg_dir.mkdir(parents=True, exist_ok=True)
        cfg.vcpkg_executable().write_text("", encoding="utf-8")

    @staticmethod
    def _register(cfg: ProjectConfig, port: str) -> None:
        info = cfg.deps_dir / "vcpkg" / "info"
        info.mkdir(parents=True, exist_ok=True)
        (info / f"{port}_6.8.0_{cfg.vcpkg_triplet}.list").write_text(
            "", encoding="utf-8")

    def test_installed_tool_leaves_vcpkg_alone(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            _make_qmlformat(cfg)
            self._register(cfg, "qtdeclarative")
            shell = FakeShell()

            self._command(cfg, shell)._repair_qmlformat_port()

            self.assertEqual(shell.commands, [])

    def test_fresh_tree_is_left_to_the_normal_install(self):
        # Nothing registered yet: the configure's `vcpkg install` handles it,
        # and `vcpkg remove` on an absent port would just fail.
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            self._fake_vcpkg(cfg)
            shell = FakeShell()

            self._command(cfg, shell)._repair_qmlformat_port()

            self.assertEqual(shell.commands, [])

    def test_another_qt_on_path_does_not_defeat_the_repair(self):
        # Qt imports Qt6::qmlformat by absolute path, so another Qt on PATH
        # leaves the configure just as broken.
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            self._fake_vcpkg(cfg)
            self._register(cfg, "qtdeclarative")
            shell = FakeShell()
            shell.path_qmlformat = "C:/Qt/6.8.0/msvc2022_64/bin/qmlformat.exe"

            command = self._command(cfg, shell)
            self.assertIsNotNone(command.qml_format.locate())
            command._repair_qmlformat_port()

            self.assertEqual(len(shell.commands), 1)
            self.assertIn("remove", shell.commands[0])

    def test_registered_port_with_missing_tool_is_unregistered(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            self._fake_vcpkg(cfg)
            self._register(cfg, "qtdeclarative")
            shell = FakeShell()

            self._command(cfg, shell)._repair_qmlformat_port()

            self.assertEqual(len(shell.commands), 1)
            argv = shell.commands[0]
            self.assertIn("remove", argv)
            self.assertIn(f"qtdeclarative:{cfg.vcpkg_triplet}", argv)


if __name__ == "__main__":
    unittest.main()
