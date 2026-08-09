cmake_minimum_required(VERSION 3.22)
project(GlassStat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Qt6 ───────────────────────────────────────────────────────────────────────
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Quick Qml)

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
)

# ── QML module — embeds QML into binary; NO_RESOURCE_TARGET_SUFFIX removed
#    (it's implicit for executable targets in Qt 6.5+) ───────────────────────
qt_add_qml_module(glassstat
    URI GlassStat
    VERSION 1.0
    QML_FILES
        src/ui/Main.qml
)

target_include_directories(glassstat PRIVATE src)

target_link_libraries(glassstat PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
)

target_compile_options(glassstat PRIVATE
    -Wall -Wextra -Wpedantic -O2
)

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS glassstat DESTINATION bin)
install(DIRECTORY config/ DESTINATION share/glassstat/config)
