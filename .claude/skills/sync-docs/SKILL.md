---
name: sync-docs
description: Keep docs/mds/*.md in step with a code change — pick the right doc, update only what the change touched, keep the relative source links and tables intact, and fix stale details you pass through. Use after changing a controller's QML-visible API, a model/filter, the DB schema, the QML tree, the build/analysis setup, or the online-search pipeline; also when the user asks to update or check the docs.
---

# Syncing `docs/mds/`

These docs are treated as current, and `CLAUDE.md` points future sessions at
them. A code change that outdates one is an unfinished change.

## Which doc

| You changed | Update |
|---|---|
| layering, object lifetime, a new directory | `architecture.md` |
| a controller's `Q_PROPERTY` / `Q_INVOKABLE` / signals | `controllers.md` |
| a model, proxy, or filter Strategy | `models-and-filters.md` |
| the search ranking or its score cache | `book-search.md` |
| OpenLibrary/Google Books, fallback, retry, API keys | `online-book-search.md` |
| filter criteria shared by local + online search | `filtering.md` |
| description translation pipeline | `description-translation.md` |
| schema, `BookTable`, `SqlQueryBuilder` | `database.md` |
| `tr`/`qsTr` pipeline, `LanguageModel`, `.ts`/`.qm` | `i18n.md` |
| CMake, qrc bundling, clang-tidy profile, logging | `build-and-resources.md` |
| any `.qml` file added/renamed/removed, component conventions | `qml.md` |

Cross-cutting change → update each affected doc, not just the closest one.
A brand-new subsystem gets its own `docs/mds/<topic>.md` **and** a row in the
`CLAUDE.md` doc table.

## House style

- Keep the existing structure — prose + tables + fenced code, not a changelog.
  Rewrite the affected paragraph; never append an "Update:" note.
- Source references are **relative links** back into the tree, e.g.
  `[BookTable](../../src/services/BookTable.hpp)` from a file in `docs/mds/`.
  If you move or rename a file, fix the links that point at it.
- `qml.md` carries a literal tree of `qml/` with a one-line description per
  file — add/remove/rename the line, keep the column alignment.
- Document behaviour and the *why* (the docs are full of "this exists because…"
  notes — that is the valuable part). Do not paste code that will drift.
- Wrap at the width already used (~72–78 cols) so diffs stay small.

## Known drift — fix opportunistically

Several docs still write the C++ namespace as `bl::` (`bl::models::LanguageModel`).
The code uses **`readary::`**. If you are editing a section that contains one,
correct it; don't launch a repo-wide rename as part of an unrelated change.

Sanity check after editing:

```bash
grep -rn "bl::" docs/mds/            # stale namespace
grep -roE "\]\(\.\./\.\./[^)]+\)" docs/mds/ | sort -u | head -40   # link targets to eyeball
```

## Scope

Update the docs for what the change actually did. Don't rewrite a doc you
merely read, and don't expand scope into a docs refactor without asking.
