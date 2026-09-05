---
name: verify
description: Run Readary's full definition-of-done gate before declaring any change complete — format, the /analyze + clang-tidy build gate, qmllint, and the CTest suite — then triage what fails. Use after finishing an edit to C++, QML, CMake or buildtools, or when the user asks to "verify", "check it builds", "run the tests", or "is this done".
---

# Verify (definition of done)

A change is **not done** until this gate is green. `--skip-analyze` is for
iterating, never for finishing.

## The gate, cheapest first

Run each step; stop at the first failure, fix, and restart from that step.

### 0. Scope check — what did I touch?

```bash
git status --short
```

| Touched | Extra step you owe |
|---|---|
| new/changed `tr()` / `qsTr()` strings | `python bootstrap.py translate` (step 4) |
| `src/**` behaviour | a test in `src/tests/` — see the `add-test` skill |
| schema, controller API, filters, QML tree | a doc under `docs/mds/` — see `sync-docs` |
| new `.qml` / `.cpp` file | nothing — the root `CMakeLists.txt` globs them with `CONFIGURE_DEPENDS`. A new **QML singleton** still needs its `set_source_files_properties(... QT_QML_SINGLETON_TYPE TRUE)` line; a new **source directory** outside the globbed set needs adding to `LIB_SOURCES` |
| new qrc in a static lib | `Q_INIT_RESOURCE(...)` in `main.cpp`, or the linker strips it |
| new test file | `readary_add_test(...)` in `src/tests/CMakeLists.txt` (not globbed) |

### 1. Format

```bash
python bootstrap.py format          # clang-format over src/, qmlformat over qml/
```

Never hand-format instead — `compile` runs qmlformat itself and the diff will
come back.

### 2. Build with the analysis gate ON

```bash
python bootstrap.py compile 2>&1 | grep -nE "error:|error C[0-9]|FAILED|Build complete|ERROR:" | head -50
```

Any clang-tidy or `/analyze` finding fails the build (`WarningsAsErrors: '*'`).
When it fails, switch to the **`fix-gate`** skill — do not reach for `NOLINT`
first, and do not "fix" it by adding `--skip-analyze`.

Fast inner loop while still writing code (not a substitute for the above):

```bash
python bootstrap.py compile --skip-analyze
```

### 3. QML lint

```bash
./venv/Scripts/cmake.exe --build build/debug --target all_qmllint
```

Catches unqualified property access and bad imports that `compile` lets
through.

### 4. Translations (only if step 0 said so)

```bash
python bootstrap.py translate       # rewrites translations/library_{en,ru,uk}.{ts,qm}
```

Then re-run step 2 — the build auto-reconfigures when the `.qm` set changes.
Commit the regenerated `.ts`/`.qm` together with the strings.

### 5. Tests

```bash
python bootstrap.py test
```

or, to keep the output readable / target one suite:

```bash
./venv/Scripts/ctest.exe --test-dir build/debug -C Debug --output-on-failure
./venv/Scripts/ctest.exe --test-dir build/debug -C Debug -R BookSearchProxy --output-on-failure
```

A Qt Test binary prints `Totals: N passed, M failed, K skipped`. Read that
line — a zero CTest exit with `0 passed` means the suite did not run.

## Reporting

Report exactly what ran and what came back. If you skipped a step (no device
for Android, network-backed test offline), say which and why. Never claim the
gate is green off a `--skip-analyze` build.

## Optional, when the change is user-visible

```bash
python bootstrap.py run             # add -d android to deploy to a device
```

Use the `run` skill for driving/screenshotting the app.
