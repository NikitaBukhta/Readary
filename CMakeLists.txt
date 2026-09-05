cmake_minimum_required(VERSION 3.16)

project(Library VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

# Minimum Qt version — not pinned to an exact release, any newer Qt6 works.
set(QT_MINIMUM_VERSION 6.8)
find_package(Qt6 ${QT_MINIMUM_VERSION} REQUIRED COMPONENTS Quick Sql Svg Network)

qt_standard_project_setup(REQUIRES ${QT_MINIMUM_VERSION})

# ---------------------------------------------------------------------------
# Static analysis (clang-tidy + MSVC /analyze)
# Toggle off for one-off builds:  python bootstrap.py compile --skip-analyze
# Forced off on Android — clang-tidy under cross-compile is too fragile.
# ---------------------------------------------------------------------------
if(ANDROID)
    set(ENABLE_ANALYZE OFF CACHE BOOL "Disabled on Android" FORCE)
endif()
option(ENABLE_ANALYZE "Run clang-tidy and MSVC /analyze during compilation" ON)

if(ENABLE_ANALYZE)
    if(DEFINED CLANG_TIDY_EXECUTABLE AND EXISTS "${CLANG_TIDY_EXECUTABLE}")
        set(_CLANG_TIDY_BIN "${CLANG_TIDY_EXECUTABLE}")
    else()
        find_program(_CLANG_TIDY_BIN NAMES clang-tidy)
    endif()
    if(_CLANG_TIDY_BIN)
        # On MSVC we route clang-tidy through a Python wrapper that strips
        # MSVC-only switches clang-cl can't parse (dash-form -external:I etc).
        # CMake auto-appends `--extra-arg-before=--driver-mode=cl` for MSVC,
        # so we don't pass --driver-mode ourselves.
        if(MSVC AND CLANG_TIDY_WRAPPER_PYTHON AND CLANG_TIDY_WRAPPER_SCRIPT)
            set(CMAKE_CXX_CLANG_TIDY
                "${CLANG_TIDY_WRAPPER_PYTHON};${CLANG_TIDY_WRAPPER_SCRIPT};${_CLANG_TIDY_BIN}")
            message(STATUS "Static analysis: clang-tidy via wrapper "
                "(${CLANG_TIDY_WRAPPER_SCRIPT}) -> ${_CLANG_TIDY_BIN}")
        else()
            set(CMAKE_CXX_CLANG_TIDY "${_CLANG_TIDY_BIN}")
            message(STATUS "Static analysis: clang-tidy = ${_CLANG_TIDY_BIN}")
        endif()
    else()
        message(WARNING
            "ENABLE_ANALYZE=ON but clang-tidy not found. "
            "Run `python bootstrap.py bootstrap` to install it.")
    endif()
    if(MSVC)
        add_compile_options(/analyze /analyze:external-)
        message(STATUS "Static analysis: MSVC /analyze enabled")
    endif()
else()
    message(STATUS "Static analysis disabled (ENABLE_ANALYZE=OFF)")
endif()

# Always-on basic correctness warnings (independent of ENABLE_ANALYZE).
if(MSVC)
    add_compile_options(/we4189 /we4101)
endif()

# ---------------------------------------------------------------------------
# Core library (shared between app and tests) + QML module backing target
# ---------------------------------------------------------------------------
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
    src/core/AppEnvironment.cpp src/core/AppEnvironment.hpp
    src/core/DatabaseManager.cpp src/core/DatabaseManager.hpp
    src/core/SqlQueryBuilder.cpp src/core/SqlQueryBuilder.hpp
    src/services/*.cpp src/services/*.hpp
    src/utils/*.cpp src/utils/*.hpp
    src/qmltypes/*.cpp src/qmltypes/*.hpp
    src/api/*.cpp src/api/*.hpp
    src/models/*.cpp src/models/*.hpp
    src/controllers/*.cpp src/controllers/*.hpp
)

file(GLOB_RECURSE QML_FILES CONFIGURE_DEPENDS qml/*.qml)

foreach(qml_file IN LISTS QML_FILES)
    file(RELATIVE_PATH alias ${CMAKE_CURRENT_SOURCE_DIR}/qml ${qml_file})
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${alias})
endforeach()

set_source_files_properties(qml/utils/Geometry.qml             PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/utils/Styles.qml               PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/utils/Format.qml               PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/theme/Theme.qml                PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/components/ToastService.qml    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(ReadaryCore STATIC ${LIB_SOURCES})
target_include_directories(ReadaryCore PUBLIC
    src
    src/qmltypes
    src/services
    src/utils
    src/controllers
    src/models/books
    src/models/settings
)
target_link_libraries(ReadaryCore PUBLIC Qt6::Core Qt6::Sql Qt6::Quick Qt6::Svg Qt6::Network)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ReadarySecrets.cmake)
set(_SECRETS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/secrets/api_keys.env)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_SECRETS_FILE})
readary_load_secrets(${_SECRETS_FILE} ReadaryCore GOOGLE_BOOKS_API_KEY)

if(ANDROID)
    target_link_libraries(ReadaryCore PUBLIC log)
endif()

qt_add_qml_module(ReadaryCore
    URI Library
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Library
    QML_FILES ${QML_FILES}
)

target_sources(ReadaryCore PRIVATE db/db_scripts.qrc assets/fonts/fonts.qrc)

# ---------------------------------------------------------------------------
# Twemoji SVG set (vendored under assets/emoji, ~4000 files).
# Globbed into a Qt big-resource so emoji glyphs render as <Image> rather than
# relying on platform color-emoji fonts — see EmojiResolver / IconGlyph.
# CONFIGURE_DEPENDS triggers reconfigure on file additions/removals.
# ---------------------------------------------------------------------------
file(GLOB _EMOJI_SVGS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/emoji/*.svg)
qt_add_resources(ReadaryCore "emoji_resources"
    PREFIX "/emoji"
    BASE ${CMAKE_CURRENT_SOURCE_DIR}/assets/emoji
    FILES ${_EMOJI_SVGS}
)

# ---------------------------------------------------------------------------
# Translations — .qm files are produced by `python bootstrap.py translate`
# ---------------------------------------------------------------------------
set(_LIBRARY_QM_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/library_en.qm
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/library_ru.qm
    ${CMAKE_CURRENT_SOURCE_DIR}/translations/library_uk.qm
)
# Re-run configure whenever the .qm files are (re)generated. Without this, an IDE
# (e.g. CLion) that configured before `bootstrap.py translate` keeps a stale cache
# with BL_HAS_TRANSLATIONS undefined and silently ships no translations until a
# manual cache reload. Listing them as configure deps makes the next build pick them up.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_LIBRARY_QM_FILES})
set(_LIBRARY_QM_ALL_PRESENT TRUE)
foreach(_qm IN LISTS _LIBRARY_QM_FILES)
    if(NOT EXISTS "${_qm}")
        set(_LIBRARY_QM_ALL_PRESENT FALSE)
        break()
    endif()
endforeach()
if(_LIBRARY_QM_ALL_PRESENT)
    target_sources(ReadaryCore PRIVATE translations/translations.qrc)
    target_compile_definitions(ReadaryCore PUBLIC BL_HAS_TRANSLATIONS)
    message(STATUS "Translations: bundling translations/translations.qrc")
else()
    message(STATUS "Translations: .qm files missing — run "
        "`python bootstrap.py translate` to enable runtime translations")
endif()

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
set(_APP_SOURCES
    src/main.cpp
    src/core/AppInitializer.cpp
    src/core/AppInitializer.hpp
)

if(WIN32 AND NOT ANDROID)
    list(APPEND _APP_SOURCES assets/app.rc)
endif()

qt_add_executable(Readary ${_APP_SOURCES})
target_link_libraries(Readary PRIVATE ReadaryCore)

# ---------------------------------------------------------------------------
# Target properties
# ---------------------------------------------------------------------------
set_target_properties(Readary PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
)

# ---------------------------------------------------------------------------
# Desktop: make the build output self-contained so the app launches from
# anywhere (IDE run config, debugger, double-click) with NO environment set.
# windeployqt is unusable with vcpkg's split debug/release tree, so we do it
# directly: a qt.conf pointing at the vcpkg plugin/qml dirs (discovery) plus a
# copy of the vcpkg runtime DLLs next to the exe (the QML plugins' deps, e.g.
# Qt6Quick, which vcpkg's applocal does not copy). Per-config via generator
# expressions: Debug uses the .../debug subtree, Release the top-level one.
# ---------------------------------------------------------------------------
if(WIN32 AND NOT ANDROID)
    if(NOT VCPKG_INSTALLED_DIR)
        message(WARNING "VCPKG_INSTALLED_DIR not set; skipping Qt runtime "
                        "deploy — running Readary may need QT_PLUGIN_PATH/PATH.")
    else()
        set(_VCPKG_TRIPLET_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}")
        set(_QT_RUNTIME_BIN
            "$<IF:$<CONFIG:Debug>,${_VCPKG_TRIPLET_DIR}/debug/bin,${_VCPKG_TRIPLET_DIR}/bin>")
        set(_QT_PREFIX
            "$<IF:$<CONFIG:Debug>,${_VCPKG_TRIPLET_DIR}/debug/Qt6,${_VCPKG_TRIPLET_DIR}/Qt6>")
        # qt.conf next to the exe → Qt finds platform/sql/etc plugins + QML.
        file(GENERATE
            OUTPUT "$<TARGET_FILE_DIR:Readary>/qt.conf"
            CONTENT "[Paths]\nPlugins = ${_QT_PREFIX}/plugins\nQml2Imports = ${_QT_PREFIX}/qml\n")
        # Copy the Qt + dependency DLLs beside the exe so runtime-loaded
        # plugins resolve their dependencies without PATH.
        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.26)
            set(_COPY_DIR copy_directory_if_different)
        else()
            set(_COPY_DIR copy_directory)
        endif()
        add_custom_command(TARGET Readary POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E ${_COPY_DIR}
                    "${_QT_RUNTIME_BIN}" "$<TARGET_FILE_DIR:Readary>"
            COMMENT "Bundling Qt runtime DLLs next to Readary.exe"
            VERBATIM)
    endif()
endif()

if(ANDROID)
    set_target_properties(Readary PROPERTIES
        QT_ANDROID_TARGET_SDK_VERSION 34
        QT_ANDROID_MIN_SDK_VERSION 23
        QT_ANDROID_VERSION_NAME ${PROJECT_VERSION}
        QT_ANDROID_VERSION_CODE 1
    )
    # android/AndroidManifest.xml adds INTERNET permission (Qt doesn't inject it).
    if(ANDROID_PACKAGE_SOURCE_DIR AND EXISTS "${ANDROID_PACKAGE_SOURCE_DIR}")
        set_target_properties(Readary PROPERTIES
            QT_ANDROID_PACKAGE_SOURCE_DIR "${ANDROID_PACKAGE_SOURCE_DIR}"
        )
        message(STATUS "Android package source: ${ANDROID_PACKAGE_SOURCE_DIR}")
    endif()
    # Qt's tls_qopensslbackend plugin needs libcrypto/libssl — bundle them
    # via androiddeployqt since Qt doesn't ship them for Android.
    if(ANDROID_OPENSSL_EXTRA_LIBS)
        set_target_properties(Readary PROPERTIES
            QT_ANDROID_EXTRA_LIBS "${ANDROID_OPENSSL_EXTRA_LIBS}"
        )
        message(STATUS "Android extra libs: ${ANDROID_OPENSSL_EXTRA_LIBS}")
    endif()
endif()

# ---------------------------------------------------------------------------
# Tests — disabled on Android (QtTest needs desktop, no device runner).
# ---------------------------------------------------------------------------
if(ANDROID)
    set(_DEFAULT_BUILD_TESTS OFF)
else()
    set(_DEFAULT_BUILD_TESTS ON)
endif()
option(BUILD_TESTS "Build unit tests" ${_DEFAULT_BUILD_TESTS})
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(src/tests)
endif()

# ---------------------------------------------------------------------------
# Install — Android packages via APK, not CMAKE_INSTALL_PREFIX.
# ---------------------------------------------------------------------------
if(NOT ANDROID)
    include(GNUInstallDirs)
    install(TARGETS Readary
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
