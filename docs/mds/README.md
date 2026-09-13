# Documentation

| Document | Topic |
|----------|-------|
| [architecture.md](architecture.md) | High-level architecture: layers, ownership, data flow |
| [controllers.md](controllers.md) | `BookController` (form + list) and `NavigationController` |
| [models-and-filters.md](models-and-filters.md) | `BookListModel`, sort/filter proxy, Strategy pattern |
| [book-search.md](book-search.md) | Search proxy: ranking, cache, tests |
| [online-book-search.md](online-book-search.md) | Online catalogs: OpenLibrary → Google Books fallback, language filter, retry, API keys |
| [filtering.md](filtering.md) | Multi-criteria filtering (language, genre, author, publisher, pages, year, rating) across both lists |
| [description-translation.md](description-translation.md) | Translating imported descriptions into the reading language |
| [database.md](database.md) | DB layer: `DatabaseManager`, `SqlQueryBuilder`, `BookTable`, `BookDTO`, `BookStatus` |
| [qml.md](qml.md) | QML structure: pages, components, theme, geometry |
| [i18n.md](i18n.md) | Languages: `LanguageModel` + `SettingsController`, runtime retranslate, `python bootstrap.py translate` pipeline |
| [build-and-resources.md](build-and-resources.md) | CMake setup, qrc resources, app env, file logger, **static analysis (clang-tidy + MSVC `/analyze`)** |

## Conventions

- Code is C++20 / Qt 6 with QML.
- One namespace per layer, matching the top-level folder under `src/`:
  `readary::core`, `readary::services`, `readary::models`,
  `readary::models::filters`, `readary::controllers`, `readary::api`,
  `readary::qmltypes`, `readary::utils`. Sub-folders inside a layer group
  files by role and add no namespace level — see the directory map in
  [architecture.md](architecture.md#top-level-directory-map).
- Headers are included by their path below `src/`
  (`#include "services/dto/BookDTO.hpp"`).
- QML module URI: `Library`. Singleton entry points (controllers, theme,
  geometry, styles) are accessed by class name from QML.
- Naming (enforced by [`.clang-tidy`](../../.clang-tidy)):
  - Private members: `_xxx` (e.g. `_searchProxy`).
  - Anonymous-namespace globals in .cpp: `g_xxx` (e.g. `g_logFile`).
  - Classes, structs, enums: `CamelCase`. Everything else: `camelBack`.
- `Q_LOGGING_CATEGORY(lcXxx, "readary.<area>.<topic>")` for runtime logs — wrap
  in `namespace { ... }` so it gets internal linkage.
- Static analysis (clang-tidy + MSVC `/analyze`) gates every build by
  default. See [build-and-resources.md](build-and-resources.md#static-analysis)
  for what's enabled and how to skip with `--skip-analyze`.
