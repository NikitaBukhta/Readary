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
python bootstrap.py compile
python bootstrap.py run
```

Use `--release` for a release build:

```bash
python bootstrap.py bootstrap --release
python bootstrap.py compile --release
python bootstrap.py run --release
```

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
