---
name: fix-gate
description: Diagnose and fix failures from Readary's MSVC /analyze + clang-tidy gate — naming-convention errors, cppcoreguidelines/modernize/performance findings, C4189/C4101 unused locals, and the clang-tidy wrapper's Qt-autogen quirks. Use when `python bootstrap.py compile` or `analyze` fails on a warning-as-error rather than a real compile error.
---

# Fixing the static-analysis gate

`compile` runs MSVC `/analyze` **and** clang-tidy on every TU with
`WarningsAsErrors: '*'` — one finding fails the build. The profile lives in
[`.clang-tidy`](../../../.clang-tidy); tests use the relaxed
[`src/tests/.clang-tidy`](../../../src/tests/.clang-tidy).

## 1. Get the findings, not the wall of output

```bash
python bootstrap.py analyze 2>&1 | grep -nE "error:|warning:" | head -40
```

`analyze` is the standalone clang-tidy pass — much faster than a full
`compile` when you are only chasing tidy findings. `/analyze`-only findings
(`warning C6xxx`) come from `compile`:

```bash
python bootstrap.py compile 2>&1 | grep -nE "error C[0-9]|warning C[0-9]|error:" | head -40
```

## 2. Fix, in this order of preference

1. **Change the code** so the finding is moot. This is almost always right.
2. **Narrow the construct** (const-ref parameter, `std::move`, `= default`).
3. `NOLINTNEXTLINE(check-name)` **with a comment saying why** — only when the
   check is genuinely wrong here (a Qt/moc idiom, a false positive). Never a
   bare `NOLINT`, never file-wide.
4. Editing `.clang-tidy` — only with the user's agreement, and add the
   reasoning as a comment the way the existing disables do.

Never "fix" a finding by adding `--skip-analyze`.

## 3. Findings this profile produces most often

| Finding | The fix here |
|---|---|
| `readability-identifier-naming` | private members `_name`, anon-namespace globals `g_name`, classes/structs/enums `CamelCase`, functions/vars/params/constants `camelBack`. Global constants are deliberately unconstrained. |
| `misc-use-internal-linkage` | wrap `Q_LOGGING_CATEGORY(...)`, file-local helpers and constants in an **anonymous namespace** (see any existing `.cpp`). |
| `C4189` / `C4101` (unused local) | delete it, or `Q_UNUSED(name)` for an unused parameter. |
| `bugprone-unused-local-non-trivial-variable` | the Qt-type/`*Model*`/`*Controller*` gap-filler for the above. Genuine RAII guards (`QMutexLocker`, `QSignalBlocker`, `QScopedValueRollback`, …) are already excluded — if yours is a new guard type, that is a legitimate `ExcludeTypes` addition. |
| `google-build-using-namespace` | use a per-operator using-declaration: `using Qt::StringLiterals::operator""_s;` — never `using namespace`. |
| `modernize-use-*` / deprecated `_qs` | prefer `u"..."_s`. |
| `performance-unnecessary-value-param` | take `const QString &` / `const QList<T> &`. |
| `cppcoreguidelines-special-member-functions` | a sole `= default` dtor is allowed; missing move ops are allowed. Anything else needs the full rule of five. |
| `readability-function-cognitive-complexity` (>30) | extract a helper into the anonymous namespace. |
| `cppcoreguidelines-init-variables` | initialise at declaration; `modernize-use-default-member-init` wants **assignment** form (`qint64 id = 0;`). |

## 4. Wrapper quirks (they are not your bug)

clang-tidy runs through
[`buildtools/clang_tidy_wrapper.py`](../../../buildtools/clang_tidy_wrapper.py):

- It **skips Qt-autogen TUs** (`*_autogen/`, `moc_*`, `qmlcache`,
  `*_qmltyperegistrations.cpp`, anything under `build/`). A finding pointing
  into generated code means the filter missed a new pattern — widen the
  wrapper, do not annotate the generated file.
- It **re-lifts MSVC include/std flags** that clang-cl drops. A flood of
  `file not found` on Qt headers means a flag was lost — check the wrapper
  before touching the code.
- `HeaderFilterRegex` limits findings to `src/` — a finding in a vcpkg or Qt
  header means an include path leaked in.

## 5. Confirm

Re-run the full gate via the **`verify`** skill. A clean `--skip-analyze`
build proves nothing about this gate.
