# Documentation

| Document | Topic |
|----------|-------|
| [architecture.md](architecture.md) | High-level architecture: layers, ownership, data flow |
| [controllers.md](controllers.md) | `BookController` (form + list) and `NavigationController` |
| [models-and-filters.md](models-and-filters.md) | `BookListModel`, sort/filter proxy, Strategy pattern |
| [book-search.md](book-search.md) | Search proxy: ranking, cache, tests |
| [database.md](database.md) | DB layer: `DatabaseManager`, `SqlQueryBuilder`, `BookTable`, `BookDTO`, `BookStatus` |
| [qml.md](qml.md) | QML structure: pages, components, theme, geometry |
| [build-and-resources.md](build-and-resources.md) | CMake setup, qrc resources, app env, file logger |

## Conventions

- Code is C++17 / Qt 6.8 with QML.
- Namespaces: `bl::core`, `bl::services`, `bl::models`, `bl::models::filters`,
  `bl::controllers`.
- QML module URI: `Library`. Singleton entry points (controllers, theme,
  geometry, styles) are accessed by class name from QML.
- Member fields prefixed with `_` (e.g. `_searchProxy`).
- `Q_LOGGING_CATEGORY(lcXxx, "bl.<area>.<topic>")` for runtime logs.
