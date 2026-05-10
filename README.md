# BeeLibrary

Qt 6.8 QML desktop application for managing a personal book library.

## Prerequisites

- **Visual Studio 2022** (with "Desktop development with C++" workload)
- **Python 3.12+**
- **Git**

CMake and vcpkg will be installed automatically by the bootstrap script if not found on the system.

## Build & Run

```bash
python bootstrap.py bootstrap
python bootstrap.py translate    # generate .ts/.qm translation files
python bootstrap.py compile
python bootstrap.py run
```

Use `--release` for a release build:

```bash
python bootstrap.py bootstrap --release
python bootstrap.py translate
python bootstrap.py compile --release
python bootstrap.py run --release
```

> The `translate` step scans `src/**/*.{cpp,hpp}` and `qml/**/*.qml` for
> `tr()` / `qsTr()` calls, auto-translates them via `deep-translator`
> (Google backend) into Russian and Ukrainian, and emits
> `translations/library_<lang>.{ts,qm}`. The `.qm` files are bundled into
> the binary at compile time and loaded at runtime by `LanguageModel`.
> Re-run `translate` whenever you add or change translatable strings.
> Skipping it on a fresh checkout is fine — the build still configures,
> but the UI will show source-language strings only.

`compile` runs **clang-tidy + MSVC `/analyze`** as a gate by default.
Skip the analyzers for fast iteration:

```bash
python bootstrap.py compile --skip-analyze
python bootstrap.py analyze              # standalone clang-tidy pass
```

See [docs/mds/build-and-resources.md](docs/mds/build-and-resources.md#static-analysis)
for what's enabled and the project's clang-tidy quirks.

## Tests

```bash
python bootstrap.py test
```

## CMake Flags

Pass CMake variables via `-D` during bootstrap:

```bash
python bootstrap.py bootstrap -DBUILD_TESTS=OFF
```

## Create Installer

```bash
python bootstrap.py package
```

Inno Setup will be installed automatically if not found on the system.

## Clean

Remove all dependencies, build directories, and venv:

```bash
python bootstrap.py clean
```

Run `python bootstrap.py help` for full command reference and status.
