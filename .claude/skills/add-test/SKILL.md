---
name: add-test
description: Write, register and run a Qt Test for Readary — the QTEST_GUILESS_MAIN file layout, the readary_add_test() CMake registration, network-backed tests against the FakeHttpServer stub, and the exact build/ctest commands for a single suite. Use when adding or fixing tests under src/tests/, or when the user asks to test a class, model, service, controller or API client.
---

# Adding a test

Tests are Qt Test executables under `src/tests/`, one binary per suite,
registered with CTest. They link `ReadaryCore`, so anything in
`src/{core,services,models,controllers,api,utils,qmltypes}` is testable
directly — the data layer has no QML dependency by design.

## 1. Place the file

Mirror the source tree: `src/tests/<layer>/<ClassName>Test.cpp`
(`models/`, `services/`, `controllers/`, `api/`, `utils/`). Shared helpers
live in `src/tests/support/` and are included as `#include "support/…"`.

## 2. File template

Model it on an existing one — [`src/tests/services/SearchCacheTest.cpp`](../../../src/tests/services/SearchCacheTest.cpp)
is a good short example.

```cpp
#include "services/SearchCache.hpp"

#include <QTest>

using Qt::StringLiterals::operator""_s;
using readary::services::SearchCache;

namespace {
// fixtures / builders live here
}

class SearchCacheTest : public QObject {
  Q_OBJECT

private slots:
  void init();                 // per-test setup
  void cleanup();              // per-test teardown
  void storesAndReadsBack();
  void expiresAfterTtl_data(); // table-driven: QTest::addColumn / newRow
  void expiresAfterTtl();
};

// ... definitions ...

QTEST_GUILESS_MAIN(SearchCacheTest)
#include "SearchCacheTest.moc"
```

Non-negotiables:

- **`QTEST_GUILESS_MAIN`**, never `QTEST_MAIN` — the latter needs a platform
  plugin sitting next to the test exe and fails in CI.
- The trailing `#include "<ClassName>Test.moc"` — the class is defined in the
  `.cpp`, so moc output must be included.
- Namespace is `readary::…`. Some `docs/mds/*` still say `bl::` — the code is
  authoritative.
- Name the suite `<ClassName>Test`; CTest strips the trailing `Test` for the
  test name.

## 3. Register it

`src/tests/CMakeLists.txt` is **not** globbed — add a line:

```cmake
readary_add_test(services/SearchCacheTest.cpp)
readary_add_test(api/GoogleBooksSearchAPITest.cpp LIBS Qt6::Network)   # extra libs
```

## 4. Network-backed tests

Never hit the real API in a test. Drive the client against the local
`QTcpServer` stub in [`src/tests/support/FakeHttpServer.hpp`](../../../src/tests/support/FakeHttpServer.hpp),
and link `LIBS Qt6::Network`. See `OpenLibrarySearchAPITest.cpp` /
`GoogleBooksSearchAPITest.cpp` for the pattern (canned bodies, status codes,
retry and timeout paths).

## 5. Build and run just this suite

```bash
./venv/Scripts/cmake.exe --build build/debug --config Debug --target SearchCacheTest
./venv/Scripts/ctest.exe --test-dir build/debug -C Debug -R SearchCache --output-on-failure
```

The binary is at `build/debug/Debug/SearchCacheTest.exe` and takes Qt Test
flags directly (`-v2`, `-functions`, `-o out.txt,txt`) when you need detail
CTest swallows. Read the `Totals: N passed, M failed, K skipped` line — a
suite that ran zero functions is a failure, not a pass.

Whole suite: `python bootstrap.py test`.

## 6. Style notes

- `src/tests/.clang-tidy` relaxes naming, `misc-use-internal-linkage` and a few
  others because MOC/Qt-Test idioms break them — but the gate still runs, so
  finish with the **`verify`** skill.
- Prefer table-driven `_data()` slots over near-duplicate test functions.
- Assert on observable behaviour. Where a test must mirror a private detail
  (e.g. a cache key), say so in a comment the way `SearchCacheTest` does.
