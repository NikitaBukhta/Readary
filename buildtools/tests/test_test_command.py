"""Unit tests for `python bootstrap.py test` — the autotest runner.

Run from the project root:

    python -m unittest discover buildtools/tests

No CMake, no Qt, no build tree beyond a stub `CTestTestfile.cmake`: `Shell` is
faked, so the tests assert on the argv each option produces and on which steps
run at all.
"""
from __future__ import annotations

import os
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from buildtools.commands.test import TestCommand
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError


class FakeShell:
    """Records commands instead of running them."""

    def __init__(self):
        self.commands: list[list[str]] = []

    def run(self, cmd, env=None):
        self.commands.append([str(c) for c in cmd])
        return None

    @staticmethod
    def which(name: str):
        return None


class FakeCMake:
    """Stands in for CMakeProvider without resolving a real toolchain."""

    def __init__(self):
        self.cmake = Path("cmake.exe")
        self.ctest = Path("ctest.exe")

    def ensure(self) -> Path:
        return self.cmake

    def ensure_ctest(self) -> Path:
        return self.ctest


class FakeMsvc:
    """`env()` returning None is the documented non-Windows answer."""

    def __init__(self, env: dict[str, str] | None = None):
        self._env = env

    def env(self):
        return self._env


def _config(root: Path, **kwargs) -> ProjectConfig:
    cfg = ProjectConfig(project_dir=root, **kwargs)
    object.__setattr__(cfg, "deps_dir", root / "deps")
    return cfg


def _make_build_tree(cfg: ProjectConfig) -> Path:
    build_dir = cfg.cmake_build_dir
    build_dir.mkdir(parents=True, exist_ok=True)
    (build_dir / "CTestTestfile.cmake").write_text("", encoding="utf-8")
    return build_dir


def _command(cfg: ProjectConfig, shell: FakeShell) -> TestCommand:
    return TestCommand(cfg, shell, FakeCMake(), FakeMsvc())


def _ctest_argv(shell: FakeShell) -> list[str] | None:
    for argv in shell.commands:
        if argv[0].endswith("ctest.exe"):
            return argv
    return None


def _build_argv(shell: FakeShell) -> list[str] | None:
    for argv in shell.commands:
        if argv[0].endswith("cmake.exe"):
            return argv
    return None


def _python_argv(shell: FakeShell) -> list[str] | None:
    for argv in shell.commands:
        if "unittest" in argv:
            return argv
    return None


class TestCommandGuardsTests(unittest.TestCase):
    def test_android_target_is_refused(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp), target="android")
            shell = FakeShell()

            with self.assertRaises(BuildError) as ctx:
                _command(cfg, shell).execute()

            self.assertIn("desktop-only", str(ctx.exception))
            self.assertEqual(shell.commands, [])

    def test_unconfigured_build_tree_names_the_command_to_run(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            shell = FakeShell()

            with self.assertRaises(BuildError) as ctx:
                _command(cfg, shell).execute()

            self.assertIn("compile", str(ctx.exception))
            self.assertEqual(shell.commands, [])

    def test_release_build_tree_is_suggested_for_a_release_run(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp), release=True)

            with self.assertRaises(BuildError) as ctx:
                _command(cfg, FakeShell()).execute()

            self.assertIn("--release", str(ctx.exception))


class TestCommandDefaultRunTests(unittest.TestCase):
    def test_builds_before_running(self):
        # The whole point of building here: a stale executable would otherwise
        # be tested and reported as green.
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            _make_build_tree(cfg)
            shell = FakeShell()

            _command(cfg, shell).execute()

            self.assertEqual(len(shell.commands), 2)
            self.assertTrue(shell.commands[0][0].endswith("cmake.exe"))
            self.assertTrue(shell.commands[1][0].endswith("ctest.exe"))

    def test_build_targets_the_configs_preset(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            _make_build_tree(cfg)
            shell = FakeShell()

            _command(cfg, shell).execute()

            argv = _build_argv(shell)
            self.assertIn("--build", argv)
            self.assertIn("--preset", argv)
            self.assertIn(cfg.cmake_preset, argv)
            self.assertIn(cfg.build_type, argv)

    def test_ctest_reports_failures_inline(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            build_dir = _make_build_tree(cfg)
            shell = FakeShell()

            _command(cfg, shell).execute()

            argv = _ctest_argv(shell)
            self.assertIn("--output-on-failure", argv)
            self.assertIn("--test-dir", argv)
            self.assertIn(str(build_dir), argv)
            self.assertIn("-C", argv)
            self.assertIn(cfg.build_type, argv)

    def test_no_filter_means_no_selection_flag(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            _make_build_tree(cfg)
            shell = FakeShell()

            _command(cfg, shell).execute()

            self.assertNotIn("-R", _ctest_argv(shell))


class TestCommandOptionsTests(unittest.TestCase):
    def test_filter_becomes_a_ctest_name_regex(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp), test_filter="BookTable")
            _make_build_tree(cfg)
            shell = FakeShell()

            _command(cfg, shell).execute()

            argv = _ctest_argv(shell)
            self.assertIn("-R", argv)
            self.assertEqual(argv[argv.index("-R") + 1], "BookTable")

    def test_no_build_skips_the_build_step(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp), skip_test_build=True)
            _make_build_tree(cfg)
            shell = FakeShell()

            _command(cfg, shell).execute()

            self.assertIsNone(_build_argv(shell))
            self.assertIsNotNone(_ctest_argv(shell))

    def test_list_neither_builds_nor_runs(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp), list_tests=True)
            _make_build_tree(cfg)
            shell = FakeShell()

            _command(cfg, shell).execute()

            self.assertIsNone(_build_argv(shell))
            argv = _ctest_argv(shell)
            self.assertIn("-N", argv)
            self.assertNotIn("--output-on-failure", argv)

    def test_list_honours_the_filter(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp), list_tests=True, test_filter="Book")
            _make_build_tree(cfg)
            shell = FakeShell()

            _command(cfg, shell).execute()

            argv = _ctest_argv(shell)
            self.assertIn("-N", argv)
            self.assertIn("-R", argv)

    def test_release_run_uses_the_release_tree(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp), release=True)
            build_dir = _make_build_tree(cfg)
            shell = FakeShell()

            _command(cfg, shell).execute()

            argv = _ctest_argv(shell)
            self.assertIn(str(build_dir), argv)
            self.assertIn("Release", argv)


class TestCommandPythonSuiteTests(unittest.TestCase):
    def test_python_only_skips_the_build_tree_entirely(self):
        # No CTestTestfile.cmake here: the buildtools suite needs no build.
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp), python_tests=True, test_python_only=True)
            shell = FakeShell()

            _command(cfg, shell).execute()

            self.assertEqual(len(shell.commands), 1)
            argv = _python_argv(shell)
            self.assertIn("discover", argv)
            self.assertIn("buildtools/tests", argv)

    def test_python_flag_runs_the_suite_alongside_ctest(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp), python_tests=True)
            _make_build_tree(cfg)
            shell = FakeShell()

            _command(cfg, shell).execute()

            self.assertIsNotNone(_python_argv(shell))
            self.assertIsNotNone(_build_argv(shell))
            self.assertIsNotNone(_ctest_argv(shell))
            # The cheap suite runs first, so a broken build system is reported
            # before the long build starts.
            self.assertIs(shell.commands[0], _python_argv(shell))

    def test_python_suite_falls_back_to_the_running_interpreter(self):
        # A fresh checkout has no venv yet.
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp), python_tests=True, test_python_only=True)
            shell = FakeShell()

            _command(cfg, shell).execute()

            self.assertTrue(Path(_python_argv(shell)[0]).exists())


class TestEnvironmentTests(unittest.TestCase):
    def test_qt_runtime_is_put_on_the_path_for_the_test_binaries(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            shell = FakeShell()

            env = _command(cfg, shell)._test_env()

            self.assertTrue(env["PATH"].startswith(str(cfg.qt_bin_dir)))
            self.assertEqual(env["QT_PLUGIN_PATH"], str(cfg.qt_plugin_dir))
            # QTEST_GUILESS_MAIN still constructs a QCoreApplication; offscreen
            # keeps anything that reaches for a display off the developer's.
            self.assertEqual(env["QT_QPA_PLATFORM"], "offscreen")

    def test_test_env_does_not_mutate_the_process_environment(self):
        with TemporaryDirectory() as tmp:
            cfg = _config(Path(tmp))
            before = os.environ.get("QT_QPA_PLATFORM")

            _command(cfg, FakeShell())._test_env()

            self.assertEqual(os.environ.get("QT_QPA_PLATFORM"), before)


if __name__ == "__main__":
    unittest.main()
