cmake_minimum_required(VERSION 3.16)

project(Library VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Sql)

qt_standard_project_setup(REQUIRES 6.8)

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

set_source_files_properties(qml/utils/Geometry.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/utils/Styles.qml   PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/theme/Theme.qml    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(BeeLibraryCore STATIC ${LIB_SOURCES})
target_include_directories(BeeLibraryCore PUBLIC src src/models src/controllers src/core src/services)
target_link_libraries(BeeLibraryCore PUBLIC Qt6::Core Qt6::Sql Qt6::Quick)

qt_add_qml_module(BeeLibraryCore
    URI Library
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Library
    QML_FILES ${QML_FILES}
    RESOURCES db/db_scripts.qrc
)

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
# option(BUILD_TESTS "Build unit tests" ON)
# if(BUILD_TESTS)
#     enable_testing()
#     add_subdirectory(tests)
# endif()

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS BeeLibrary
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
