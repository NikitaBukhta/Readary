"""Unit tests for the translate pipeline (no network, no Qt tools required).

Run from the project root:

    python -m unittest discover buildtools/tests

The tests cover only the pure-Python parts of `translate.py`:
- the `tr/qsTr` regex and string decoding
- C++/QML context resolution
- placeholder protection (Qt %1/%n round-trip through translation)
- existing-`.ts` parsing (preservation of manual translations)
- `.ts` XML generation (round-trip).

Auto-translation (deep-translator network call) and lrelease invocation
are intentionally out of scope.
"""
from __future__ import annotations

import unittest
import xml.etree.ElementTree as ET
from dataclasses import replace
from pathlib import Path
from tempfile import TemporaryDirectory

from buildtools.commands.translate import (
    TranslateCommand,
    _Message,
    _MessageKey,
    _cpp_context,
    _decode,
    _protect_placeholders,
    _qml_context,
    _read_existing,
    _restore_placeholders,
    _TR_RE,
    _write_ts,
)


# ---- Regex / scanner --------------------------------------------------------
class TrRegexTests(unittest.TestCase):
    def _captures(self, text: str):
        m = _TR_RE.search(text)
        if m is None:
            return None
        return {
            "fn": m.group("fn"),
            "source": m.group(2),
            "comment": m.group(3),
            "plural": m.group("plural"),
        }

    def test_qsTr_simple(self):
        c = self._captures('text: qsTr("Hello")')
        self.assertEqual(c["fn"], "qsTr")
        self.assertEqual(c["source"], "Hello")
        self.assertIsNone(c["comment"])
        self.assertIsNone(c["plural"])

    def test_qsTr_with_comment(self):
        c = self._captures('qsTr("Save", "verb on the toolbar")')
        self.assertEqual(c["source"], "Save")
        self.assertEqual(c["comment"], "verb on the toolbar")

    def test_qsTr_with_plural_count(self):
        c = self._captures('qsTr("%n book(s)", "", count)')
        self.assertEqual(c["source"], "%n book(s)")
        self.assertEqual(c["comment"], "")
        self.assertIsNotNone(c["plural"])

    def test_tr_with_escaped_quote(self):
        c = self._captures(r'tr("He said \"hi\"")')
        self.assertEqual(c["fn"], "tr")
        self.assertEqual(c["source"], r"He said \"hi\"")

    def test_QT_TR_NOOP(self):
        c = self._captures('QT_TR_NOOP("Untranslated literal")')
        self.assertEqual(c["fn"], "QT_TR_NOOP")
        self.assertEqual(c["source"], "Untranslated literal")

    def test_word_boundary_does_not_match_method_named_translate(self):
        # `subTr(...)` and `myTr(...)` must not match — the \b at the start of
        # the pattern guards against that.
        self.assertIsNone(_TR_RE.search('subTr("nope")'))
        self.assertIsNone(_TR_RE.search('mytr("nope")'))

    def test_finds_multiple_calls_on_same_line(self):
        line = 'a: qsTr("One"); b: qsTr("Two")'
        matches = list(_TR_RE.finditer(line))
        self.assertEqual(len(matches), 2)
        self.assertEqual(matches[0].group(2), "One")
        self.assertEqual(matches[1].group(2), "Two")


class DecodeTests(unittest.TestCase):
    def test_newline(self):
        self.assertEqual(_decode(r"line1\nline2"), "line1\nline2")

    def test_quote(self):
        self.assertEqual(_decode(r'He said \"hi\"'), 'He said "hi"')

    def test_backslash(self):
        self.assertEqual(_decode(r"a\\b"), "a\\b")

    def test_no_op_on_plain_text(self):
        self.assertEqual(_decode("Hello"), "Hello")


# ---- Context resolution -----------------------------------------------------
class CppContextTests(unittest.TestCase):
    def test_namespace_plus_class(self):
        src = """
namespace bl::controllers {
class BookController : public QObject {
public:
  void foo() { tr("hi"); }
};
}
"""
        self.assertEqual(
            _cpp_context(Path("BookController.cpp"), src),
            "bl::controllers::BookController",
        )

    def test_split_namespace_components(self):
        # Nested namespaces, each opened on its own line — the shape the
        # scanner's line-anchored regexes are written for.
        src = """
namespace bl {
namespace models {
class LanguageModel : public QObject {
public:
  void foo() { tr("hi"); }
};
}
}
"""
        self.assertEqual(
            _cpp_context(Path("LanguageModel.cpp"), src),
            "bl::models::LanguageModel",
        )

    def test_no_class_falls_back_to_filename_stem(self):
        src = "namespace bl::services {} // no class declaration"
        self.assertEqual(_cpp_context(Path("helpers.cpp"), src), "helpers")

    def test_no_namespace_returns_bare_class(self):
        src = "class Standalone : public QObject {};"
        self.assertEqual(_cpp_context(Path("foo.cpp"), src), "Standalone")


class QmlContextTests(unittest.TestCase):
    def test_uses_basename_without_extension(self):
        self.assertEqual(_qml_context(Path("qml/Main.qml")), "Main")
        self.assertEqual(
            _qml_context(Path("qml/pages/mainPage/MainPage.qml")),
            "MainPage",
        )


# ---- Placeholder protection -------------------------------------------------
class PlaceholderTests(unittest.TestCase):
    def test_round_trip_single_placeholder(self):
        protected, phs = _protect_placeholders("Hello %1!")
        self.assertNotIn("%1", protected)
        self.assertEqual(_restore_placeholders(protected, phs), "Hello %1!")

    def test_round_trip_multiple_placeholders(self):
        protected, phs = _protect_placeholders("Open %1/%2 in %3")
        self.assertEqual(len(phs), 3)
        self.assertEqual(
            _restore_placeholders(protected, phs),
            "Open %1/%2 in %3",
        )

    def test_round_trip_localized_n_arg(self):
        protected, phs = _protect_placeholders("%L1 books out of %n")
        self.assertEqual(set(phs), {"%L1", "%n"})
        self.assertEqual(
            _restore_placeholders(protected, phs),
            "%L1 books out of %n",
        )

    def test_restore_tolerates_spacing_in_sentinels(self):
        # Google Translate sometimes pads our sentinels; the restorer must
        # cope with `@@ PH 0 @@` and similar deformations.
        _, phs = _protect_placeholders("Hello %1")
        deformed = "Привет @@ PH 0 @@"
        self.assertEqual(_restore_placeholders(deformed, phs), "Привет %1")

    def test_no_placeholders_round_trip(self):
        protected, phs = _protect_placeholders("Just text")
        self.assertEqual(phs, [])
        self.assertEqual(_restore_placeholders(protected, phs), "Just text")


# ---- Existing .ts parsing ---------------------------------------------------
_TS_FIXTURE = """<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru_RU">
<context>
    <name>BookController</name>
    <message>
        <location filename="../src/controllers/BookController.cpp" line="42"/>
        <source>Failed to update book status.</source>
        <translation>Не удалось обновить статус книги.</translation>
    </message>
    <message>
        <location filename="../src/controllers/BookController.cpp" line="80"/>
        <source>Pending review</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>MainPage</name>
    <message numerus="yes">
        <source>%n book(s)</source>
        <translation>
            <numerusform>%n книга</numerusform>
            <numerusform>%n книги</numerusform>
            <numerusform>%n книг</numerusform>
        </translation>
    </message>
</context>
</TS>
"""


class ReadExistingTests(unittest.TestCase):
    def test_returns_empty_when_file_missing(self):
        self.assertEqual(_read_existing(Path("/no/such/file.ts")), {})

    def test_keeps_finished_translation(self):
        with TemporaryDirectory() as tmp:
            p = Path(tmp) / "library_ru.ts"
            p.write_text(_TS_FIXTURE, encoding="utf-8")
            existing = _read_existing(p)

        key = _MessageKey(
            context="BookController",
            source="Failed to update book status.",
            comment="",
        )
        self.assertIn(key, existing)
        text, forms = existing[key]
        self.assertEqual(text, "Не удалось обновить статус книги.")
        self.assertEqual(forms, [])

    def test_skips_unfinished_translation(self):
        with TemporaryDirectory() as tmp:
            p = Path(tmp) / "library_ru.ts"
            p.write_text(_TS_FIXTURE, encoding="utf-8")
            existing = _read_existing(p)

        unfinished_key = _MessageKey(
            context="BookController", source="Pending review", comment="",
        )
        self.assertNotIn(unfinished_key, existing)

    def test_preserves_numerusforms(self):
        with TemporaryDirectory() as tmp:
            p = Path(tmp) / "library_ru.ts"
            p.write_text(_TS_FIXTURE, encoding="utf-8")
            existing = _read_existing(p)

        plural_key = _MessageKey(context="MainPage", source="%n book(s)", comment="")
        self.assertIn(plural_key, existing)
        _, forms = existing[plural_key]
        self.assertEqual(forms, ["%n книга", "%n книги", "%n книг"])


# ---- .ts XML writing --------------------------------------------------------
class WriteTsTests(unittest.TestCase):
    def _round_trip(self, contexts, translations):
        # The directory outlives the helper: callers read the .ts back through
        # `_read_existing`, which answers "nothing translated" for a missing
        # file rather than raising.
        tmp = TemporaryDirectory()
        self.addCleanup(tmp.cleanup)

        project = Path(tmp.name)
        ts_path = project / "translations" / "library_ru.ts"
        ts_path.parent.mkdir()
        _write_ts(ts_path, "ru_RU", self._anchor(contexts, project),
                  translations, project)
        return ts_path.read_text(encoding="utf-8"), ts_path

    @staticmethod
    def _anchor(contexts, project: Path):
        """Rebase the fixtures' locations onto the temporary project root.

        The scanner always hands `_write_ts` absolute paths, and `_write_ts`
        turns them back into project-relative ones. Fixtures spell the
        locations relative because that reads better; anchoring them here is
        what makes them the shape the writer is given in a real run.
        """
        return {
            ctx: [
                replace(msg, locations=[(project / loc, line)
                                        for loc, line in msg.locations])
                for msg in messages
            ]
            for ctx, messages in contexts.items()
        }

    def test_emits_well_formed_xml(self):
        key = _MessageKey(context="BookController", source="Hello", comment="")
        msg = _Message(key=key, locations=[(Path("src/foo.cpp"), 7)])
        contexts = {"BookController": [msg]}
        translations = {key: ("Привет", [])}

        xml_text, _ = self._round_trip(contexts, translations)
        # ET.fromstring will raise on malformed XML.
        root = ET.fromstring(xml_text)
        self.assertEqual(root.tag, "TS")
        self.assertEqual(root.attrib.get("language"), "ru_RU")

    def test_round_trip_preserves_translation(self):
        key = _MessageKey(context="BookController", source="Hello", comment="")
        msg = _Message(key=key, locations=[(Path("src/foo.cpp"), 7)])
        contexts = {"BookController": [msg]}
        translations = {key: ("Привет", [])}

        _, ts_path = self._round_trip(contexts, translations)
        existing = _read_existing(ts_path)
        self.assertIn(key, existing)
        self.assertEqual(existing[key], ("Привет", []))

    def test_unfinished_when_no_translation(self):
        key = _MessageKey(context="C", source="No tr yet", comment="")
        msg = _Message(key=key, locations=[(Path("src/foo.cpp"), 1)])
        xml_text, _ = self._round_trip({"C": [msg]}, {})

        root = ET.fromstring(xml_text)
        translation_el = root.find("./context/message/translation")
        self.assertIsNotNone(translation_el)
        self.assertEqual(translation_el.attrib.get("type"), "unfinished")

    def test_emits_numerus_for_plural_messages(self):
        key = _MessageKey(context="MainPage", source="%n book(s)", comment="")
        msg = _Message(
            key=key,
            locations=[(Path("qml/Main.qml"), 3)],
            is_plural=True,
        )
        contexts = {"MainPage": [msg]}
        translations = {key: ("", ["%n книга", "%n книги", "%n книг"])}

        xml_text, _ = self._round_trip(contexts, translations)
        root = ET.fromstring(xml_text)
        message_el = root.find("./context/message")
        self.assertEqual(message_el.attrib.get("numerus"), "yes")
        forms = [nf.text for nf in message_el.findall("./translation/numerusform")]
        self.assertEqual(forms, ["%n книга", "%n книги", "%n книг"])

    def test_xml_escapes_special_chars(self):
        key = _MessageKey(
            context="C", source='5 < 10 & "ok"', comment="",
        )
        msg = _Message(key=key, locations=[(Path("src/foo.cpp"), 1)])
        translations = {key: ('"да" & 1 < 2', [])}
        xml_text, _ = self._round_trip({"C": [msg]}, translations)
        # Must parse cleanly even with reserved characters.
        root = ET.fromstring(xml_text)
        src_el = root.find("./context/message/source")
        tr_el = root.find("./context/message/translation")
        self.assertEqual(src_el.text, '5 < 10 & "ok"')
        self.assertEqual(tr_el.text, '"да" & 1 < 2')


# ---- Full scan over a fake project tree -------------------------------------
class ScanIntegrationTests(unittest.TestCase):
    """Drives `_scan_into` through TranslateCommand to ensure the regex,
    context resolver and message aggregator work together."""

    def _make_project(self, tmp: Path) -> Path:
        (tmp / "src" / "controllers").mkdir(parents=True)
        (tmp / "qml" / "components").mkdir(parents=True)

        (tmp / "src" / "controllers" / "BookController.cpp").write_text(
            'namespace bl::controllers {\n'
            'class BookController : public QObject {\n'
            '  void foo() { tr("Failed."); tr("Pending review"); }\n'
            '};\n'
            '}\n',
            encoding="utf-8",
        )
        (tmp / "qml" / "components" / "BottomNavBar.qml").write_text(
            'import QtQuick\n'
            'Rectangle {\n'
            '    property string a: qsTr("Library")\n'
            '    property string b: qsTr("Search")\n'
            '    property string n: qsTr("%n book(s)", "", count)\n'
            '}\n',
            encoding="utf-8",
        )
        return tmp

    def test_scan_picks_cpp_and_qml_strings(self):
        with TemporaryDirectory() as tmp:
            project = self._make_project(Path(tmp))

            class _FakeConfig:
                def __init__(self, project_dir):
                    self.project_dir = project_dir

            cmd = TranslateCommand(
                config=_FakeConfig(project),
                shell=None,
                venv_mgr=None,
                linguist=None,
            )
            contexts = cmd._scan_sources()  # noqa: SLF001 — testing internals

        self.assertIn("bl::controllers::BookController", contexts)
        cpp_sources = {m.key.source for m in contexts["bl::controllers::BookController"]}
        self.assertEqual(cpp_sources, {"Failed.", "Pending review"})

        self.assertIn("BottomNavBar", contexts)
        qml_sources = {m.key.source for m in contexts["BottomNavBar"]}
        self.assertEqual(qml_sources, {"Library", "Search", "%n book(s)"})

        plural_msg = next(
            m for m in contexts["BottomNavBar"] if m.key.source == "%n book(s)"
        )
        self.assertTrue(plural_msg.is_plural)


if __name__ == "__main__":
    unittest.main()
