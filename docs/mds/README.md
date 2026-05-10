# Documentation

| Document | Topic |
|----------|-------|
| [architecture.md](architecture.md) | High-level architecture: layers, ownership, data flow |
| [controllers.md](controllers.md) | `BookController` (form + list) and `NavigationController` |
| [models-and-filters.md](models-and-filters.md) | `BookListModel`, sort/filter proxy, Strategy pattern |
| [book-search.md](book-search.md) | Search proxy: ranking, cache, tests |
| [database.md](database.md) | DB layer: `DatabaseManager`, `SqlQueryBuilder`, `BookTable`, `BookDTO`, `BookStatus` |
| [qml.md](qml.md) | QML structure: pages, components, theme, geometry |
| [i18n.md](i18n.md) | Languages: `LanguageModel` + `SettingsController`, runtime retranslate, `python bootstrap.py translate` pipeline |
| [build-and-resources.md](build-and-resources.md) | CMake setup, qrc resources, app env, file logger, **static analysis (clang-tidy + MSVC `/analyze`)** |

## Conventions

- Code is C++17 / Qt 6.8 with QML.
- Namespaces: `bl::core`, `bl::services`, `bl::models`, `bl::models::filters`,
  `bl::controllers`.
- QML module URI: `Library`. Singleton entry points (controllers, theme,
  geometry, styles) are accessed by class name from QML.
- Naming (enforced by [`.clang-tidy`](../../.clang-tidy)):
  - Private members: `_xxx` (e.g. `_searchProxy`).
  - Anonymous-namespace globals in .cpp: `g_xxx` (e.g. `g_logFile`).
  - Classes, structs, enums: `CamelCase`. Everything else: `camelBack`.
- `Q_LOGGING_CATEGORY(lcXxx, "bl.<area>.<topic>")` for runtime logs — wrap
  in `namespace { ... }` so it gets internal linkage.
- Static analysis (clang-tidy + MSVC `/analyze`) gates every build by
  default. See [build-and-resources.md](build-and-resources.md#static-analysis)
  for what's enabled and how to skip with `--skip-analyze`.
