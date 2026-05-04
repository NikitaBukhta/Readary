cmake_minimum_required(VERSION 3.16)

project(Library VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Sql)

qt_standard_project_setup(REQUIRES 6.8)

# ---------------------------------------------------------------------------
# Static analysis (clang-tidy + MSVC /analyze)
# Toggle off for one-off builds:  python bootstrap.py compile --skip-analyze
# ---------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
# Core library (shared between app and tests) + QML module backing target
# ---------------------------------------------------------------------------
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
    src/core/AppEnvironment.cpp src/core/AppEnvironment.hpp
    src/core/DatabaseManager.cpp src/core/DatabaseManager.hpp
    src/core/SqlQueryBuilder.cpp src/core/SqlQueryBuilder.hpp
    src/services/*.cpp src/services/*.hpp
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
set_source_files_properties(qml/theme/Theme.qml                PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/components/ToastService.qml    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(BeeLibraryCore STATIC ${LIB_SOURCES})
target_include_directories(BeeLibraryCore PUBLIC src src/models src/controllers src/core src/services)
target_link_libraries(BeeLibraryCore PUBLIC Qt6::Core Qt6::Sql Qt6::Quick)

qt_add_qml_module(BeeLibraryCore
    URI Library
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Library
    QML_FILES ${QML_FILES}
)

target_sources(BeeLibraryCore PRIVATE db/db_scripts.qrc)

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
qt_add_executable(BeeLibrary
    src/main.cpp
    src/core/AppInitializer.cpp
    src/core/AppInitializer.hpp
    assets/app.rc
)
target_link_libraries(BeeLibrary PRIVATE BeeLibraryCore)

# ---------------------------------------------------------------------------
# Target properties
# ---------------------------------------------------------------------------
set_target_properties(BeeLibrary PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(src/tests)
endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS BeeLibrary
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
