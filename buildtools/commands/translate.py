"""Self-contained translation pipeline.

Scans C++ (`tr(...)`) and QML (`qsTr(...)`) sources, auto-translates the
captured strings into Russian and Ukrainian via `deep-translator` (Google
backend), writes Qt-Linguist `.ts` files preserving any human edits already
present, and finally compiles them to `.qm` via `lrelease` (from qttools).

We do NOT call `lupdate`: the vcpkg `qttools` port only ships
`lupdate-pro.exe` which expects a qmake `.pro` project and silently skips
the QML scan when fed plain source files.  A lean Python scanner gives us
deterministic, debuggable behaviour and full control over auto-translation.
"""
from __future__ import annotations

import html
import re
import sys
import sysconfig
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from pathlib import Path

from buildtools.commands.base import Command
from buildtools.config import ProjectConfig
from buildtools.errors import BuildError
from buildtools.providers.linguist import LinguistProvider
from buildtools.shell import Shell
from buildtools.venv_manager import VenvManager


@dataclass(frozen=True)
class _MessageKey:
    context: str
    source: str
    comment: str  # disambiguation comment; "" if none


@dataclass
class _Message:
    key: _MessageKey
    locations: list[tuple[Path, int]] = field(default_factory=list)
    is_plural: bool = False


# ---- Source scanner ---------------------------------------------------------
# tr / qsTr / QT_TR_NOOP / qsTrId — handle simple "..." literals with optional
# disambiguation comment ("...", "...") and optional plural count argument.
# Concatenations and tr(qVariable) calls are out of scope (lupdate skips them
# too).  Multi-line string concatenation is handled by allowing optional
# adjacent string literals to be merged ("a" "b").
_STRING = r'"((?:[^"\\]|\\.)*)"'
_TR_RE = re.compile(
    r"\b(?P<fn>tr|qsTr|QT_TR_NOOP|QT_TRANSLATE_NOOP)\s*\(\s*"
    + _STRING
    + r"(?:\s*,\s*" + _STRING + r")?"
    + r"(?P<plural>\s*,\s*[^)]+)?",
)


def _decode(literal: str) -> str:
    """Convert a C/QML string-literal payload into its runtime value."""
    return (
        literal.replace(r"\n", "\n")
        .replace(r"\t", "\t")
        .replace(r"\r", "\r")
        .replace(r'\"', '"')
        .replace(r"\'", "'")
        .replace(r"\\", "\\")
    )


# ---- Context resolution -----------------------------------------------------
_NAMESPACE_RE = re.compile(r"^\s*namespace\s+([A-Za-z_][\w:]*)\s*\{", re.MULTILINE)
_CLASS_RE = re.compile(r"^\s*class\s+([A-Za-z_]\w*)\s*[:\{]", re.MULTILINE)


def _cpp_context(file_path: Path, source: str) -> str:
    """Approximate `bl::ns::ClassName` from a single-class C++ TU.

    Captures every `namespace X { ... namespace Y::Z { ...` line and the
    first `class Foo : ...` declaration. Sufficient for this codebase where
    every controller/model lives in its own file under `bl::<area>::`.
    """
    namespaces: list[str] = []
    for m in _NAMESPACE_RE.finditer(source):
        namespaces.extend(part for part in m.group(1).split("::") if part)
    cls = _CLASS_RE.search(source)
    if cls:
        return "::".join([*namespaces, cls.group(1)])
    return file_path.stem


def _qml_context(file_path: Path) -> str:
    # Qt's runtime context for QML qsTr() is the basename without extension.
    return file_path.stem


# ---- .ts XML I/O ------------------------------------------------------------
_LANGUAGE_TAGS = {"en": "en_US", "ru": "ru_RU", "uk": "uk_UA"}
_LRELEASE_NUMERUS_FORMS = {"en": 2, "ru": 3, "uk": 3}


def _read_existing(ts_path: Path) -> dict[_MessageKey, tuple[str, list[str]]]:
    """Return {key: (translation, numerusforms)} for already-translated msgs.

    `translation` is the singular form; `numerusforms` carries plural forms
    when the message is numerus="yes".  Empty / type="unfinished" entries
    are skipped so they get retranslated on the next run.
    """
    if not ts_path.exists():
        return {}

    out: dict[_MessageKey, tuple[str, list[str]]] = {}
    try:
        tree = ET.parse(ts_path)
    except ET.ParseError:
        return out

    for ctx in tree.getroot().findall("context"):
        ctx_name_el = ctx.find("name")
        ctx_name = ctx_name_el.text if ctx_name_el is not None else ""
        if not ctx_name:
            continue
        for msg in ctx.findall("message"):
            src_el = msg.find("source")
            tr_el = msg.find("translation")
            if src_el is None or tr_el is None or not src_el.text:
                continue
            comment_el = msg.find("comment")
            comment = comment_el.text if comment_el is not None and comment_el.text else ""
            key = _MessageKey(context=ctx_name, source=src_el.text, comment=comment)

            if tr_el.get("type") == "unfinished":
                continue
            forms = [nf.text or "" for nf in tr_el.findall("numerusform")]
            text = (tr_el.text or "").strip()
            if forms:
                out[key] = ("", forms)
            elif text:
                out[key] = (text, [])
    return out


def _xml_escape(text: str) -> str:
    return html.escape(text, quote=False)


def _write_ts(
    ts_path: Path,
    lang_tag: str,
    contexts: dict[str, list[_Message]],
    translations: dict[_MessageKey, tuple[str, list[str]]],
    project_dir: Path,
) -> None:
    """Render contexts to a Qt-Linguist .ts file."""
    lines: list[str] = [
        '<?xml version="1.0" encoding="utf-8"?>',
        "<!DOCTYPE TS>",
        f'<TS version="2.1" language="{lang_tag}">',
    ]

    for ctx_name in sorted(contexts.keys()):
        messages = contexts[ctx_name]
        lines.append("<context>")
        lines.append(f"    <name>{_xml_escape(ctx_name)}</name>")
        for msg in messages:
            numerus_attr = ' numerus="yes"' if msg.is_plural else ""
            lines.append(f"    <message{numerus_attr}>")
            for loc_file, loc_line in msg.locations:
                rel = loc_file.relative_to(project_dir).as_posix()
                lines.append(
                    f'        <location filename="../{rel}" line="{loc_line}"/>',
                )
            lines.append(
                f"        <source>{_xml_escape(msg.key.source)}</source>",
            )
            if msg.key.comment:
                lines.append(
                    f"        <comment>{_xml_escape(msg.key.comment)}</comment>",
                )

            tr_value, tr_forms = translations.get(msg.key, ("", []))
            if msg.is_plural:
                if not tr_forms:
                    lines.append('        <translation type="unfinished">')
                    for _ in range(_LRELEASE_NUMERUS_FORMS.get(lang_tag[:2], 2)):
                        lines.append("            <numerusform></numerusform>")
                    lines.append("        </translation>")
                else:
                    lines.append("        <translation>")
                    for form in tr_forms:
                        lines.append(
                            f"            <numerusform>{_xml_escape(form)}</numerusform>",
                        )
                    lines.append("        </translation>")
            else:
                if tr_value:
                    lines.append(
                        f"        <translation>{_xml_escape(tr_value)}</translation>",
                    )
                else:
                    lines.append('        <translation type="unfinished"></translation>')
            lines.append("    </message>")
        lines.append("</context>")

    lines.append("</TS>")
    ts_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


# ---- Auto-translator --------------------------------------------------------
# Qt placeholders (%1, %n, %L1, ...) get garbled by Google Translate. We swap
# them out for opaque sentinels before the call and restore them afterwards.
_PLACEHOLDER_RE = re.compile(r"%(?:L?\d+|n)")


def _protect_placeholders(text: str) -> tuple[str, list[str]]:
    placeholders: list[str] = []

    def sub(match: re.Match[str]) -> str:
        placeholders.append(match.group(0))
        return f"@@PH{len(placeholders) - 1}@@"

    return _PLACEHOLDER_RE.sub(sub, text), placeholders


def _restore_placeholders(text: str, placeholders: list[str]) -> str:
    for i, ph in enumerate(placeholders):
        # Google sometimes mangles spaces around our sentinels; tolerate that.
        text = re.sub(rf"@@\s*PH\s*{i}\s*@@", ph, text, flags=re.IGNORECASE)
    return text


class _Translator:
    """Lazy wrapper around deep_translator.GoogleTranslator with caching."""

    def __init__(self) -> None:
        from deep_translator import GoogleTranslator  # type: ignore[import-not-found]

        self._GoogleTranslator = GoogleTranslator
        self._cache: dict[tuple[str, str], str] = {}

    def translate(self, text: str, target: str) -> str:
        if target == "en" or not text.strip():
            return text
        cached = self._cache.get((text, target))
        if cached is not None:
            return cached

        protected, phs = _protect_placeholders(text)
        try:
            out = self._GoogleTranslator(source="en", target=target).translate(protected)
        except Exception as exc:  # noqa: BLE001 — surface any backend failure as warning
            print(f"  WARN: translate '{text[:40]}...' -> {target} failed: {exc}")
            return ""
        if out is None:
            return ""
        restored = _restore_placeholders(out, phs)
        self._cache[(text, target)] = restored
        return restored


# ---- Command ----------------------------------------------------------------
class TranslateCommand(Command):
    """Update .ts and compile .qm translation files (no `lupdate` needed)."""

    name = "translate"
    summary = "Update .ts and compile .qm translation files"

    LANGUAGES: tuple[str, ...] = ("en", "ru", "uk")

    _CPP_EXTS: tuple[str, ...] = (".cpp", ".cxx", ".cc", ".hpp", ".h")
    _QML_EXTS: tuple[str, ...] = (".qml", ".js")

    def __init__(
        self,
        config: ProjectConfig,
        shell: Shell,
        venv_mgr: VenvManager,
        linguist: LinguistProvider,
    ):
        self.config = config
        self.shell = shell
        self.venv_mgr = venv_mgr
        self.linguist = linguist

    def execute(self) -> None:
        translations_dir = self.config.project_dir / "translations"
        translations_dir.mkdir(exist_ok=True)

        contexts = self._scan_sources()
        if not contexts:
            print("No translatable strings found.")
            return

        total = sum(len(msgs) for msgs in contexts.values())
        print(f"\n=== Scanned {total} string(s) across "
              f"{len(contexts)} context(s) ===")

        self._ensure_translator_lib()
        translator = _Translator()

        for lang in self.LANGUAGES:
            ts_path = translations_dir / f"library_{lang}.ts"
            existing = _read_existing(ts_path)
            translations = self._build_translations(
                contexts, existing, translator, lang,
            )
            _write_ts(
                ts_path,
                _LANGUAGE_TAGS[lang],
                contexts,
                translations,
                self.config.project_dir,
            )
            print(f"  {ts_path.name}: "
                  f"{sum(1 for v in translations.values() if v[0] or v[1])} translated, "
                  f"{sum(1 for k in self._iter_keys(contexts) if k not in translations)} unfinished")

        lrelease_path = self.linguist.lrelease()
        print(f"\n=== Compiling .qm files (lrelease: {lrelease_path}) ===")
        for lang in self.LANGUAGES:
            ts = translations_dir / f"library_{lang}.ts"
            qm = ts.with_suffix(".qm")
            self.shell.run([lrelease_path, ts, "-qm", qm])

        print("\nTranslation pipeline complete.")
        print(f"  Edit/refine translations in {translations_dir}")
        print("  then re-run `python bootstrap.py translate` to recompile.")

    # ---- internals ----------------------------------------------------------
    def _ensure_translator_lib(self) -> None:
        # bootstrap.py runs under the user's system Python, but `deep-translator`
        # is installed into the project venv. Splice the venv's site-packages
        # into sys.path before attempting the import.
        self._activate_venv_imports()
        try:
            import deep_translator  # noqa: F401
            return
        except ImportError:
            pass
        print("Installing deep-translator into venv...")
        self.venv_mgr.install("deep-translator")
        self._activate_venv_imports()
        try:
            import deep_translator  # noqa: F401
        except ImportError as exc:
            raise BuildError(
                "deep-translator install completed but the package is still "
                "not importable from the venv at " f"{self.config.venv_dir}.",
            ) from exc

    def _activate_venv_imports(self) -> None:
        venv_dir = self.config.venv_dir
        if not venv_dir.exists():
            return
        if self.config.is_windows:
            candidates = [venv_dir / "Lib" / "site-packages"]
        else:
            py_tag = f"python{sys.version_info.major}.{sys.version_info.minor}"
            candidates = [
                venv_dir / "lib" / py_tag / "site-packages",
                venv_dir / "lib" / "site-packages",
            ]
        for path in candidates:
            if path.is_dir():
                str_path = str(path)
                if str_path not in sys.path:
                    sys.path.insert(0, str_path)
                return
        # Fall back to sysconfig — handles unusual layouts like uv-managed venvs.
        scheme = sysconfig.get_paths(scheme="venv" if "venv" in sysconfig.get_scheme_names() else "posix_prefix")
        purelib = Path(scheme["purelib"])
        if purelib.is_dir():
            sys.path.insert(0, str(purelib))

    def _scan_sources(self) -> dict[str, list[_Message]]:
        contexts: dict[str, dict[_MessageKey, _Message]] = {}

        src_root = self.config.project_dir / "src"
        if src_root.exists():
            for path in self._walk(src_root, self._CPP_EXTS):
                source = self._read(path)
                ctx = _cpp_context(path, source)
                self._scan_into(contexts, ctx, path, source)

        qml_root = self.config.project_dir / "qml"
        if qml_root.exists():
            for path in self._walk(qml_root, self._QML_EXTS):
                source = self._read(path)
                ctx = _qml_context(path)
                self._scan_into(contexts, ctx, path, source)

        return {ctx: list(msgs.values()) for ctx, msgs in contexts.items()}

    @staticmethod
    def _walk(root: Path, exts: tuple[str, ...]) -> list[Path]:
        out: list[Path] = []
        for ext in exts:
            out.extend(root.rglob(f"*{ext}"))
        return sorted(out)

    @staticmethod
    def _read(path: Path) -> str:
        try:
            return path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            return path.read_text(encoding="utf-8", errors="ignore")

    @staticmethod
    def _scan_into(
        contexts: dict[str, dict[_MessageKey, _Message]],
        ctx: str,
        path: Path,
        source: str,
    ) -> None:
        ctx_messages = contexts.setdefault(ctx, {})
        for match in _TR_RE.finditer(source):
            raw = match.group(2)
            if raw is None:
                continue
            text = _decode(raw)
            if not text:
                continue
            comment_raw = match.group(3)
            comment = _decode(comment_raw) if comment_raw else ""
            line = source.count("\n", 0, match.start()) + 1
            is_plural = match.group("plural") is not None and "%n" in text

            key = _MessageKey(context=ctx, source=text, comment=comment)
            msg = ctx_messages.get(key)
            if msg is None:
                msg = _Message(key=key, is_plural=is_plural)
                ctx_messages[key] = msg
            else:
                msg.is_plural = msg.is_plural or is_plural
            msg.locations.append((path, line))

    def _build_translations(
        self,
        contexts: dict[str, list[_Message]],
        existing: dict[_MessageKey, tuple[str, list[str]]],
        translator: _Translator,
        lang: str,
    ) -> dict[_MessageKey, tuple[str, list[str]]]:
        out: dict[_MessageKey, tuple[str, list[str]]] = {}
        plural_count = _LRELEASE_NUMERUS_FORMS.get(lang, 2)

        for messages in contexts.values():
            for msg in messages:
                preserved = existing.get(msg.key)
                if preserved is not None:
                    out[msg.key] = preserved
                    continue

                if msg.is_plural:
                    translated = translator.translate(msg.key.source, lang)
                    if not translated:
                        continue
                    out[msg.key] = ("", [translated] * plural_count)
                else:
                    translated = translator.translate(msg.key.source, lang)
                    if translated:
                        out[msg.key] = (translated, [])
        return out

    @staticmethod
    def _iter_keys(contexts: dict[str, list[_Message]]):
        for messages in contexts.values():
            for msg in messages:
                yield msg.key
