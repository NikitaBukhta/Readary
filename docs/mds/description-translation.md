# Description translation

Books imported from the online catalogs come with descriptions in whatever
language the source happened to have — usually English. This translates them
into the reading language **before** the book is written to the DB, so a book is
stored once, already readable.

```
GlobalBookSearchController::openBook(isbn)
   ├─ already in the library? ──────────────────────────▶ bookImportRequested   (DB copy wins)
   └─ fetchDescription(workKey)
        └─ onDescriptionReady
             ├─ detected == target, or inconclusive ────▶ bookImportRequested
             └─ translate(description, target, id)
                  └─ onTranslationReady ───────────────▶ bookImportRequested
```

The import is **deferred**, not patched afterwards: the DB write happens once,
in `onTranslationReady`, with the translated text already in the DTO.

## Detecting a language

`src/api/translate/LanguageDetector.hpp` — free functions, no QObject, script
based. `detectScript` tallies letters by `QChar::script()` (digits and
punctuation are ignored) and returns the dominant one.

Two entry points, deliberately different in how much they'll guess:

| | Latin | Cyrillic | no letters |
|---|---|---|---|
| `detectLanguage` (a body of text) | `"en"` | `"uk"` / `"ru"` | empty |
| `detectQueryLanguage` (a search query) | **empty** | `"uk"` / `"ru"` | empty |

Cyrillic splits on letters that exist in Ukrainian but not Russian — `і ї є ґ`
in either case. Ukrainian text that happens to use none of them reads as `ru`;
that's the known limit of the approach, and it costs a filter that's slightly
too broad rather than a wrong result.

The asymmetry on Latin is the point. For a **description**, `"en"` is a fair
default (imported descriptions are overwhelmingly English) and a wrong guess
costs one redundant translation. For a **search query**, a wrong guess becomes a
`langRestrict`/`language:` filter that hides valid German or French editions, so
the query path refuses to guess and sends no filter at all — see
[online-book-search.md](online-book-search.md#language-filter).

Detection returning **empty must not trigger a translation**:

```cpp
const QString detected = api::detectLanguage(description);
if (target.isEmpty() || detected.isEmpty() || detected == target) {
  return false;   // keep the description as-is, import immediately
}
```

## Translating

`api::ITranslator` is the seam:

```cpp
virtual void translate(const QString &text, const QString &targetLang, quint64 requestId) = 0;
signals: void translationReady(quint64 requestId, QString translated);
```

`requestId` is echoed back so the controller can drop the answer to a request it
has already superseded (`_pendingTranslateId`). The controller assigns the id
*before* calling `translate`, which is what makes a synchronous reply — the
empty-input shortcut — safe.

`GoogleTranslator` implements it over the keyless `gtx` endpoint:

```
https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=<lang>&dt=t&q=<text>
```

No credentials, no quota to manage. The trade-off is an undocumented response
shape — a nested array whose first element holds the translated segments, which
a long text arrives split across and which `parseTranslation` re-joins in order.
The endpoint also answers 403 without a browser-like `User-Agent`.

**Every path completes.** An HTTP error, an unparseable body, or empty input all
emit `translationReady(requestId, {})` rather than staying silent; the caller
keeps the untranslated description and the import proceeds. A translator that
can fail silently would strand the import forever.

Target language comes from `models::LanguageModel::localeCode(current())` —
`"en"` / `"ru"` / `"uk"`, see [i18n.md](i18n.md).

## Files & tests

| File | |
|---|---|
| `src/api/translate/ITranslator.hpp` | the seam |
| `src/api/translate/GoogleTranslator.{hpp,cpp}` | keyless gtx client |
| `src/api/translate/LanguageDetector.{hpp,cpp}` | script-based detection |
| `src/api/translate/LanguageConverter.{hpp,cpp}` | ISO 639-1 ↔ MARC, via `QLocale` |
| `src/controllers/GlobalBookSearchController.cpp` | `maybeTranslateDescription`, deferred import |

```bash
ctest --test-dir build/debug -V -R "LanguageDetector|GoogleTranslator|LanguageConverter|GlobalBookSearchController"
```

`GoogleTranslatorTest` drives a local `QTcpServer` through `setEndpoint()`; no
test touches the real network.
