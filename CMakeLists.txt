cmake_minimum_required(VERSION 3.22)
project(GlassStat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Git-based auto-version ────────────────────────────────────────────────────
find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GLASSSTAT_BUILD_NUMBER
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GLASSSTAT_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if(NOT GLASSSTAT_BUILD_NUMBER OR GLASSSTAT_BUILD_NUMBER STREQUAL "")
    set(GLASSSTAT_BUILD_NUMBER "0")
endif()
if(NOT GLASSSTAT_GIT_HASH OR GLASSSTAT_GIT_HASH STREQUAL "")
    set(GLASSSTAT_GIT_HASH "unknown")
endif()
set(GLASSSTAT_VERSION_BASE "1.0.0")
set(GLASSSTAT_VERSION_FULL "${GLASSSTAT_VERSION_BASE}.${GLASSSTAT_BUILD_NUMBER}")
message(STATUS "GlassStat version: ${GLASSSTAT_VERSION_FULL} (${GLASSSTAT_GIT_HASH})")

configure_file(
    ${CMAKE_SOURCE_DIR}/src/version.hpp.in
    ${CMAKE_BINARY_DIR}/generated/version.hpp
    @ONLY
)

# ── Qt6 & XCB Dependencies ───────────────────────────────────────────────────
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Quick Qml Widgets Svg QuickControls2)
find_package(PkgConfig REQUIRED)
pkg_check_modules(XCB REQUIRED xcb xcb-shape)

# Suppress QML import URI policy warning (QTP0001)
qt_policy(SET QTP0001 NEW)
# Suppress qmldir extra directory warning (QTP0004)
qt_policy(SET QTP0004 NEW)

qt_standard_project_setup()

# ── Executable ────────────────────────────────────────────────────────────────
qt_add_executable(glassstat
    src/main.cpp
    src/core/CpuCollector.cpp
    src/core/MemoryCollector.cpp
    src/core/NetworkCollector.cpp
    src/core/ThermalCollector.cpp
    src/core/GpuCollector.cpp
    src/core/SystemMonitor.cpp
    src/config/ConfigManager.cpp
    src/scripts/ScriptRunner.cpp
    src/ui/TrayController.cpp
    src/ui/WindowManager.cpp
    src/platform/X11WindowHelper.cpp
)

qt_add_resources(glassstat "app_icons"
    PREFIX "/"
    FILES icons/glassstat.svg
)

# ── QML module — embeds QML into binary ───────────────────────────────────────
qt_add_qml_module(glassstat
    URI GlassStat
    VERSION 1.0
    QML_FILES
        src/ui/Main.qml
        src/ui/SparklineCanvas.qml
        src/ui/SettingsWindow.qml
)

target_include_directories(glassstat PRIVATE src ${XCB_INCLUDE_DIRS} ${CMAKE_BINARY_DIR}/generated)

target_link_libraries(glassstat PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::Widgets
    Qt6::Svg
    Qt6::QuickControls2
    ${XCB_LIBRARIES}
)

target_compile_options(glassstat PRIVATE
    -Wall -Wextra -Wpedantic -O2
)

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS glassstat DESTINATION bin)
install(DIRECTORY config/ DESTINATION share/glassstat/config)
install(FILES packaging/glassstat.desktop DESTINATION share/applications)
install(FILES icons/glassstat.svg DESTINATION share/icons/hicolor/scalable/apps)
install(FILES icons/glassstat.svg DESTINATION share/pixmaps RENAME glassstat.svg)
