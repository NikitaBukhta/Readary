# Build and resources

CMake-based, Qt 6.8+. Two top-level targets:

- `BeeLibraryCore` — STATIC library that holds all C++ code, the QML module,
  and bundled resources.
- `BeeLibrary` — executable that links `BeeLibraryCore` and provides
  `main.cpp` + `AppInitializer`.

Tests live in `src/tests/` as a separate sub-project guarded by
`option(BUILD_TESTS "Build unit tests" ON)`.

## CMakeLists overview

Key bits in [CMakeLists.txt](../../CMakeLists.txt):

- `set(CMAKE_AUTORCC ON)` — `.qrc` files added to a target are compiled
  automatically. Used for the SQL scripts qrc.
- `qt_add_library(BeeLibraryCore STATIC ${LIB_SOURCES})` — globs everything
  under `src/` (excluding `main.cpp`, `AppInitializer.cpp` which belong to
  the exe).
- `qt_add_qml_module(BeeLibraryCore URI Library QML_FILES ${QML_FILES})` —
  registers the QML module. QML singletons (`Geometry`, `Styles`, `Theme`)
  get `QT_QML_SINGLETON_TYPE TRUE` set per-source.
- `target_sources(BeeLibraryCore PRIVATE db/db_scripts.qrc)` — adds the SQL
  qrc to the library so AUTORCC compiles it.
- `qt_add_executable(BeeLibrary src/main.cpp src/core/AppInitializer.cpp ...)`
  — main exe.

The `src/models/*.cpp src/models/*.hpp` glob is `GLOB_RECURSE`, so adding a
nested folder (e.g. `src/models/filters/`) is picked up automatically.

## qrc resources

Two resource sets are bundled:

1. **QML files** — auto-managed by `qt_add_qml_module` based on `QML_FILES`.
   Accessible at `qrc:/qt/qml/Library/...`.
2. **SQL scripts** — declared in [db/db_scripts.qrc](../../db/db_scripts.qrc):

   ```xml
   <qresource prefix="/db">
     <file>init.sql</file>
     <file>test_data.sql</file>
   </qresource>
   ```

   Accessed at `:/db/init.sql` and `:/db/test_data.sql` from
   `DatabaseManager::runScript`.

### `Q_INIT_RESOURCE` quirk

Resources added to a **static** library may have their initializer dropped
by the linker (no symbol from the qrc translation unit is referenced from
the exe, so the static-init function is collected as dead code). Symptom:
`QFile(":/db/init.sql").open(...)` returns "No such file or directory".

Fix lives in [src/main.cpp](../../src/main.cpp):

```cpp
int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    // Resources from a static library can be stripped by the linker; force init.
    Q_INIT_RESOURCE(db_scripts);

    bl::core::AppEnvironment::installFileLogger();
    ...
}
```

`Q_INIT_RESOURCE(db_scripts)` references the qrc's init function explicitly,
keeping it in the link.

## App environment

[`AppEnvironment`](../../src/core/AppEnvironment.cpp) handles paths and
logging.

| Function | Purpose |
|----------|---------|
| `dataPath()` | App data directory (`%LOCALAPPDATA%/BeeLibrary` on Windows) |
| `databasePath()` | `<dataPath>/beelibrary.db` |
| `logFilePath()` | Time-stamped `<dataPath>/log_DD.MM.YYYY-HH.MM.SS.log` |
| `installFileLogger()` | Installs a `QtMessageHandler` that writes every message to the log file (and stderr in debug). Cleans up logs older than 7 days. |
| `shutdownFileLogger()` | Restores default handler, flushes and closes the file. |

The file logger is global (single static `QFile`, mutex-guarded). Output
format: `ISO timestamp [LEVEL] category: message`.

## Logging filter

[`main.cpp`](../../src/main.cpp) sets log filter rules per build type:

```cpp
#ifdef QT_NO_DEBUG
    QLoggingCategory::setFilterRules("bl.*.debug=false\nbl.*.info=false");
#else
    QLoggingCategory::setFilterRules("bl.*.debug=true");
#endif
```

- **Release** — `bl.*.debug` and `bl.*.info` suppressed; only warnings,
  criticals and fatals reach the log file.
- **Debug** — `bl.*.debug` explicitly enabled (Qt defaults debug categories
  to off unless declared otherwise).

This means `qCDebug(lcDb)` etc. only show up in the log file in debug builds.

## Build commands

The repo uses CMake presets (see [CMakePresets.json](../../CMakePresets.json)
if present) and a `bootstrap.py` wrapper:

```
python bootstrap.py compile      # configure + build (debug)
```

For tests:

```
cmake --build build --target BookSearchProxyModelTest
ctest --test-dir build -V -R BookSearchProxy
```

## File map

| File | Purpose |
|------|---------|
| [CMakeLists.txt](../../CMakeLists.txt) | Top-level build config |
| [src/main.cpp](../../src/main.cpp) | Entry point + resource init + logging filter |
| [src/core/AppEnvironment.hpp](../../src/core/AppEnvironment.hpp) / [.cpp](../../src/core/AppEnvironment.cpp) | Paths, file logger |
| [src/core/AppInitializer.hpp](../../src/core/AppInitializer.hpp) / [.cpp](../../src/core/AppInitializer.cpp) | Bootstraps DB → models → controllers → QML engine |
| [db/db_scripts.qrc](../../db/db_scripts.qrc) | qrc manifest for SQL scripts |
