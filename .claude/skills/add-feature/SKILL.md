---
name: add-feature
description: Add a feature across Readary's layers without breaking the architecture — where the code goes (QML → controller → model → service → core), the QML-singleton create()/setInstance() wiring, the AppInitializer connect-placement rule, and the Q_GADGET DTO boundary. Use when starting anything that spans more than one file or introduces a new controller, model, service, API client or page.
---

# Adding a feature across layers

## The layering (dependencies point downward, never up)

```
QML (module URI `Library`)
  └─ Controllers (QML singletons)   src/controllers/   readary::controllers
       └─ Models (QAbstractItemModel, proxies)  src/models/   readary::models
            └─ Services (DTOs, table access, caches)  src/services/  readary::services
                 └─ Core (DatabaseManager, SqlQueryBuilder)  src/core/  readary::core
       ↘ src/qmltypes/ — Q_GADGET value wrappers at the controller↔QML boundary
         src/api/ — network catalog clients (readary::api)
```

**QML talks only to controllers.** Never expose a model or service to QML
directly; a controller exposes it as a read-only `Q_PROPERTY`. Never let a
lower layer include a higher one, and never let `src/services` or `src/core`
pull in Qt Quick/QML — tests depend on that.

## 1. Decide the layer before writing anything

| The thing you are adding | Layer |
|---|---|
| new screen or widget | `qml/pages/…` or `qml/components/…` → the **`qml-component`** skill |
| state or an action QML invokes | a controller (existing one first — resist a new singleton) |
| a list QML renders | a model, exposed via a controller property |
| filtering/sorting over an existing list | a `BookSortFilterProxyModel` **Strategy**, not a new model — see [`docs/mds/models-and-filters.md`](../../../docs/mds/models-and-filters.md) |
| persistence, table access, plain data | a service + DTO |
| remote catalog / HTTP | `src/api/` — see [`docs/mds/online-book-search.md`](../../../docs/mds/online-book-search.md) |
| new column/table | the **`db-change`** skill first |

Read the relevant `docs/mds/*.md` before non-trivial work — they are kept
current and will save you re-deriving the design.

## 2. Adding a new QML-visible controller

Header (`src/controllers/FooController.hpp`):

```cpp
class FooController : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY(readary::models::FooModel *fooModel READ fooModel CONSTANT FINAL)
public:
  explicit FooController(QObject *parent = nullptr);
  static FooController *create(QQmlEngine *engine, QJSEngine *scriptEngine);
  static void setInstance(FooController *instance);
private:
  static FooController *s_instance;
};
```

Source — copy [`NavigationController.cpp`](../../../src/controllers/NavigationController.cpp) exactly:

```cpp
FooController *FooController::create(QQmlEngine *engine, QJSEngine *scriptEngine) {
  Q_UNUSED(engine)
  Q_UNUSED(scriptEngine)
  Q_ASSERT_X(s_instance, "FooController::create", "setInstance() must be called before the QML engine loads");
  QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
  return s_instance;
}
```

Gotcha: Qt prefers a **default constructor** over `create()` when one exists.
Keep singleton classes free of default-argument constructors that make them
default-constructible, or QML will build a second, unwired instance.

Then in [`AppInitializer`](../../../src/core/AppInitializer.cpp) (the composition root):

- construct it in `initModels()`, **parented to `this`** (no manual `delete`);
- register it in `registerQmlTypes()` via `FooController::setInstance(_fooController);`.

## 3. The connect-placement rule

- **Intra-domain** signal/slot connects (a controller wiring its own models)
  live **inside that controller**.
- **Cross-domain** connects live in **`AppInitializer`** — so controllers never
  include each other. Look at how `bookSaved → BookListModel::refresh` and
  `bookImportRequested` are wired there.

## 4. Data at the QML boundary

Plain service DTOs (`BookDTO`, `ReadingSessionDTO`) stay moc-free. To hand one
to QML, wrap it as a `Q_GADGET` value type in `src/qmltypes/` (see
`BookDTOObject`). Do not add `Q_OBJECT`/`Q_GADGET` to the service DTO itself.

## 5. Logging

One category per file, in an anonymous namespace (`misc-use-internal-linkage`
enforces this):

```cpp
namespace {
Q_LOGGING_CATEGORY(lcFoo, "readary.controllers.foo")
}
```

## 6. Before you call it done

- new user-facing strings → `tr()` / `qsTr()`, then `python bootstrap.py translate`;
- a test for anything with logic → the **`add-test`** skill;
- the matching `docs/mds/*.md` → the **`sync-docs`** skill;
- the full gate → the **`verify`** skill.
