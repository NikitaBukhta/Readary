# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Readary — a Qt 6 / QML application for managing a personal book library (desktop + Android). C++ backing layer, QML frontend, SQLite storage, and an online book-discovery feature that fans out across public catalog APIs.

## Build & run

All workflows go through the `bootstrap.py` wrapper (never invoke CMake/vcpkg directly for routine work — the wrapper manages the vcpkg toolchain, presets, and analysis gate):

```bash
python bootstrap.py bootstrap          # one-time: install deps (CMake, vcpkg, Qt)
python bootstrap.py translate          # generate translations/library_<lang>.{ts,qm}
python bootstrap.py compile            # configure + build (Debug, WITH static-analysis gate)
python bootstrap.py compile --skip-analyze   # fast iteration — skip clang-tidy + /analyze
python bootstrap.py run                # launch the app
python bootstrap.py analyze            # standalone clang-tidy pass, no compile
python bootstrap.py test               # build + run all autotests
python bootstrap.py test -k BookTable  # only suites matching a regex
python bootstrap.py test -l            # list registered suites, run none
python bootstrap.py package            # build an Inno Setup installer
```

- Add `--release` to `bootstrap`/`compile`/`run` for a release build.
- Add `-d android` to `bootstrap`/`compile`/`run` to cross-compile and deploy to a device. Static analysis and tests are forced OFF on Android. See `README.md` for the full Android toolchain story.
- Build output lands in `build/<preset>/` (presets: `debug`, `release`, `android-*`). Generator is Ninja Multi-Config on desktop.

### Running a single test

Tests are Qt Test executables under `src/tests/`, registered with CTest. The
CTest name is the file name without the `Test` suffix.

```bash
python bootstrap.py test -k BookSearchProxyModel   # build + run one suite
python bootstrap.py test -k BookSearchProxyModel --no-build
python bootstrap.py test --python                  # + the buildtools unittest suite
# or run build/debug/.../BookSearchProxyModelTest.exe directly for Qt Test flags
```

## Static-analysis gate (important)

`compile` runs **MSVC `/analyze` + clang-tidy** on every TU with `WarningsAsErrors: '*'` — any finding fails the build. Use `--skip-analyze` while iterating, but code must pass the gate before it's done. Key conventions enforced by the profile (see `docs/mds/build-and-resources.md` for the full list):

- Naming: private members `_xxx`, anonymous-namespace globals `g_xxx`, classes/enums CamelCase, everything else camelBack.
- Wrap `Q_LOGGING_CATEGORY(...)` in an anonymous namespace (`misc-use-internal-linkage`).
- `Q_UNUSED(name)` for unused params; prefer `u"..."_s` over deprecated `_qs`.
- clang-tidy under MSVC is routed through `buildtools/clang_tidy_wrapper.py`, which skips Qt-autogen TUs and re-lifts MSVC include/std flags that clang-cl silently drops. Tests use a relaxed `src/tests/.clang-tidy` (MOC/Qt-Test idioms break several checks).

## Architecture

Strict downward-dependency layering. QML talks ONLY to controllers (never models/services directly):

```
QML (Library module)  →  Controllers (singletons)  →  Models  →  Services  →  Core (DB)
                                                         ↘ qmltypes (Q_GADGET DTO wrappers at the controller↔QML boundary)
```

- **Controllers** (`src/controllers/`) are QML singletons and the only QML-visible entry points: `BookController` (book form/list state, characters, reading timer), `NavigationController` (stack router), `SettingsController` (owns `LanguageModel` + `FontModel`), `BookDiscoveryController` (online search/import), `BookStatisticsController` (per-book reading statistics). Models are exposed as read-only properties on controllers.
- **Singleton wiring pattern**: C++ instance is constructed by `AppInitializer`, registered via `setInstance()`; the QML `create()` factory returns that instance with `CppOwnership`. (Note: Qt prefers a default ctor over `create()` — keep singleton classes free of default-arg ctors.)
- **`AppInitializer`** (`src/core/app/`) is the composition root: `initDatabase()` → `initModels()` → `registerQmlTypes()`. All QObjects are parented to it. **Connect-placement rule**: intra-domain connects live inside the owning controller; cross-domain connects live in `AppInitializer` (so controllers don't include each other).
- **Book list data flow**: one `BookListModel` (DB source) → four `BookSortFilterProxyModel` (one per `ListKind`, configured by a filter Strategy) → `BookSearchProxyModel` (free-text + relevance ranking) → QML. The active category's proxy is swapped in as the search proxy's source.
- **`src/qmltypes/`** holds Q_GADGET value types (e.g. `BookDTOObject`) that adapt plain service DTOs for QML — keeps the data layer free of moc.

### Where files live

Each top-level folder under `src/` is one layer **and** one namespace (`readary::core`, `::services`, `::models`, `::controllers`, `::api`, `::qmltypes`, `::utils`). Sub-folders group by role and add **no** namespace level:

```
src/core/        app/ (AppEnvironment, AppInitializer)  db/ (DatabaseManager, SqlQueryBuilder)
src/services/    dto/  storage/  caching/  pdf/  filtering/  emoji/  statistics/
src/models/      books/{list,proxy,filters,details}/  settings/
src/api/         bookSearch/  translate/
src/tests/       mirrors the layout above; shared helpers in support/
qml/components/  base/ buttons/ input/ display/ feedback/ lists/ navigation/ overlays/
```

- **Includes are path-qualified from `src`**: `#include "services/dto/BookDTO.hpp"`, not a bare basename. `src` is the only public include dir; CMake adds each header folder privately as well, because moc emits bare-basename includes into `qmltyperegistrations.cpp`. Tests also get `src/tests`, hence `#include "support/TempLibrary.hpp"`.
- **Adding a folder needs no CMake edit** — the layer globs are `GLOB_RECURSE` and the private include list is derived from them. Registering a new *test* file does (`src/tests/CMakeLists.txt`).
- **QML sub-folders are cosmetic**: every `.qml` under `qml/` lands in the one `Library` module URI under its file name, so components are used as `SurfaceCard { }` wherever they sit. Only the page URLs in `NavigationController` name a path.

Read these before non-trivial work in the corresponding area — they are kept current:

| Doc | Covers |
|-----|--------|
| `docs/mds/architecture.md` | Layering, object lifecycle, directory map |
| `docs/mds/controllers.md` | Full QML-visible API of each controller |
| `docs/mds/models-and-filters.md` | Filter Strategy pattern |
| `docs/mds/book-search.md` | Search proxy ranking algorithm + score-cache caveats |
| `docs/mds/online-book-search.md` | Online catalogs: OpenLibrary → Google Books fallback, language filter, retry, API keys |
| `docs/mds/filtering.md` | Multi-criteria filtering across the local library and the online search |
| `docs/mds/description-translation.md` | Translating imported book descriptions into the UI language |
| `docs/mds/database.md` | SQLite schema |
| `docs/mds/i18n.md` | Translation pipeline (`tr`/`qsTr` → auto-translate → `.qm` → runtime) |
| `docs/mds/build-and-resources.md` | qrc bundling, static-analysis details, logging |
| `docs/mds/qml.md` | QML component conventions |

## Skills

Project skills live in `.claude/skills/`. Invoke with `/<name>`; they also
trigger on their own when the task matches.

| Skill | Use it for |
|-------|-----------|
| `ping-pong` | Implementing a feature or fixing a bug as a red/green rally, closed by an adversarial review round. |
| `verify` | The definition-of-done gate: format → analysis build → qmllint → tests. Run before calling any change complete. |
| `fix-gate` | Triaging `/analyze` + clang-tidy warnings-as-errors. |
| `add-test` | Writing, registering and running a single Qt Test. |
| `add-feature` | Anything spanning layers — new controller/model/service/API client. |
| `qml-component` | Any work under `qml/`. |
| `db-change` | Schema changes (there is no migration runner — read it first). |
| `sync-docs` | Keeping `docs/mds/*.md` in step with the code. |
| `commit` | Staging and committing in this repo's style. |

## Things that bite

- **Static-lib resource init**: qrc resources in the `ReadaryCore` static lib can be stripped by the linker. `main.cpp` calls `Q_INIT_RESOURCE(db_scripts)` to force it; do the same for any new static-lib qrc.
- **DB scripts** load from qrc at startup: `:/db/init.sql` always, `:/db/test_data.sql` only in debug builds (`#ifndef QT_NO_DEBUG`).
- **Translations** are bundled only when all three `.qm` files exist (`BL_HAS_TRANSLATIONS`). A fresh checkout builds without them (UI shows source strings) — run `python bootstrap.py translate` after adding/changing translatable strings.
- **QML signal params** accept only basic QML types — `qint64` breaks the component ("Invalid signal parameter type").
- **Android color emoji**: bundled COLR/CBDT fonts don't render; emoji go through vendored Twemoji SVGs (`assets/emoji/`) + `EmojiResolver`.
- **Tests** use `QTEST_GUILESS_MAIN` (not `QTEST_MAIN`, which needs a platform plugin next to the test exe).
