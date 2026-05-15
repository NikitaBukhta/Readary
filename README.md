# BeeLibrary

Qt 6.8 QML desktop application for managing a personal book library.

## Prerequisites

- **Visual Studio 2022** (with "Desktop development with C++" workload) — **desktop build only**
- **Python 3.12+**
- **Git**

CMake and vcpkg will be installed automatically by the bootstrap script if not found on the system.

For Android cross-compilation see the [Android](#android) section — JDK,
SDK, NDK, and Qt for Android are all auto-downloaded; Visual Studio is not
required.

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

## Android

Pass `-d android` to `bootstrap`, `compile`, and `run` to cross-compile and
deploy to a connected Android device. The `-d/--device` flag picks the
target — without it, builds default to the host platform (so on Windows,
omitting `-d` is the same as `-d windows`). Everything except the host
operating system itself is downloaded automatically:

| Component                          | Source                         | Pinned version       |
| :--------------------------------- | :----------------------------- | :------------------- |
| JDK                                | Eclipse Temurin (Adoptium API) | 17                   |
| Android command-line tools         | `dl.google.com`                | 11076708             |
| Android platform                   | `sdkmanager`                   | API 34               |
| Android build-tools                | `sdkmanager`                   | 34.0.0               |
| Android NDK                        | `sdkmanager`                   | r26b (26.1.10909125) |
| Qt for Android (host + arm64-v8a)  | `aqtinstall`                   | 6.8.0                |

Everything lands under `~/BeeLibrary-dependencies/android/` so it never
mixes with the desktop vcpkg tree.

### Host requirements

These cannot be auto-installed and must be present before bootstrap:

- **Python 3.12+** and **Git** (same as desktop)
- A working **internet connection** to reach `api.adoptium.net`,
  `dl.google.com`, and Qt's `download.qt.io` mirror
- ~10 GB of free disk space under `~/BeeLibrary-dependencies/android/`
- (For `run -d android`) **USB debugging enabled** on a connected Android
  device, or an emulator running, so `adb` can reach it. With multiple
  devices visible, set `ANDROID_SERIAL` to the serial from `adb devices`
  to pick one explicitly

You do **not** need Android Studio, an existing JDK, or a system Qt install.

### Build & deploy

```bash
python bootstrap.py bootstrap -d android   # ~5–10 min first time
python bootstrap.py translate              # optional, same as desktop
python bootstrap.py compile   -d android   # produces a Debug APK
python bootstrap.py run       -d android   # adb install + launch
```

For a release APK:

```bash
python bootstrap.py bootstrap -d android --release
python bootstrap.py compile   -d android --release
python bootstrap.py run       -d android --release
```

The first `bootstrap -d android` downloads the entire toolchain.
Subsequent runs detect what's already on disk and skip it. Static
analysis (`clang-tidy` / `/analyze`) is forced off for Android — it
doesn't play well with cross-compilation under Qt's Android toolchain.

### Choosing an ABI

By default the build targets `arm64-v8a` only — covers virtually every
modern phone, fastest first-bootstrap. Override with `--abi`:

```bash
# x86_64 emulator only
python bootstrap.py bootstrap -d android --abi x86_64

# Phones (arm64) + emulators (x86_64) — one fat APK
python bootstrap.py bootstrap -d android --abi arm64-v8a x86_64

# Universal: every supported ABI in one APK
python bootstrap.py bootstrap -d android --abi all
```

Choices: `arm64-v8a`, `armeabi-v7a`, `x86_64`, `x86`, plus the alias
`all` (expands to every supported ABI). Multiple ABIs are bundled into
one **fat APK** with `lib/<abi>/...` per slice — Android picks the
right one at install time. The first ABI in the list doubles as Qt's
toolchain anchor; ordering is otherwise irrelevant. Each ABI adds
~200 MB of Qt download under
`~/BeeLibrary-dependencies/android/qt/6.8.0/android_<abi>/`.

Each ABI combo has its own build directory
(`build/android-debug-arm64-v8a/`, `build/android-debug-arm64-v8a_x86_64/`,
etc.) so configurations don't clobber each other. Bootstrap is
idempotent — only missing ABIs are downloaded; `compile` and `run`
must be invoked with the same `--abi` set as the matching `bootstrap`.

### What's targeted by default

- **ABI**: `arm64-v8a` (override via `--abi`, see above).
- **Min SDK**: API 23 (Android 6.0) — Qt 6.8's lower bound.
- **Target SDK**: API 34.
- **Package**: `org.qtproject.example.BeeLibrary` (Qt's default
  template). Override via a custom `AndroidManifest.xml` and
  `QT_ANDROID_PACKAGE_SOURCE_DIR` if you need a different application id.

Resulting APK lives at:

```text
build/android-{debug,release}/android-build/build/outputs/apk/{debug,release}/*.apk
```

## Clean

Remove all dependencies, build directories, and venv:

```bash
python bootstrap.py clean
```

This wipes the Android toolchain (JDK, SDK, NDK, Qt for Android) too — it
all lives under `~/BeeLibrary-dependencies/`.

Run `python bootstrap.py help` for full command reference and status.
