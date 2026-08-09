# GlassStat — System Architecture & Design Specification

## 1. System Overview
**GlassStat** is a high-performance, frameless, desktop-widget system monitor engineered natively for Linux (Arch Linux, X11 & Wayland). It combines zero-overhead system file parsing in modern C++20 with a hardware-accelerated Qt6 QML glassmorphism interface.

---

## 2. Architectural Separation of Concerns

The codebase strictly enforces modularity across four distinct core subsystems:

```text
GlassStat/
├── config/
│   └── default_config.toml     # TOML theme settings, polling interval, custom scripts
├── src/
│   ├── main.cpp                # App bootstrapping & QML singleton registrations
│   ├── core/                   # Subsystem 1: Low-level C++ System Collectors & Backend
│   │   ├── CpuCollector.hpp / .cpp
│   │   ├── MemoryCollector.hpp / .cpp
│   │   ├── NetworkCollector.hpp / .cpp
│   │   ├── ThermalCollector.hpp / .cpp
│   │   ├── GpuCollector.hpp / .cpp
│   │   └── SystemMonitor.hpp / .cpp   # Central QObject aggregator & QML property bridge
│   ├── config/                 # Subsystem 2: Configuration & Hot-Reload Engine
│   │   └── ConfigManager.hpp / .cpp  # TOML parser & QFileSystemWatcher hot-reload
│   ├── scripts/                # Subsystem 3: Async Script Execution Engine
│   │   └── ScriptRunner.hpp / .cpp   # Non-blocking QProcess runner for user bash scripts
│   └── ui/                     # Subsystem 4: Hardware-Accelerated QML Frontend
│       └── Main.qml              # Desktop widget, glassmorphism, drag & collapsible UX
└── CMakeLists.txt              # Modern CMake build manifest
```

---

## 3. Subsystem Detailed Responsibilities

### `src/core/` — System Collectors & Monitor Backend
- **Direct Linux API Parsing**: Directly reads `/proc/stat`, `/proc/meminfo`, `/proc/net/dev`, `/sys/class/hwmon/*`, `/sys/class/drm/*` using zero-copy `std::string_view` and `std::from_chars`.
- **CpuCollector**: Computes per-core and total CPU usage deltas over polling intervals.
- **MemoryCollector**: Calculates RAM (used/total) and Swap percentages from raw kernel pages.
- **NetworkCollector**: Computes real-time RX/TX transfer speeds (KB/s, MB/s) using high-resolution `std::chrono::steady_clock`.
- **ThermalCollector**: Scans hardware monitoring sensors (`hwmon`) and extracts core temperatures (°C).
- **GpuCollector**: Dual-backend support for AMD (DRM sysfs / `amdgpu` hwmon) and NVIDIA (`nvidia-smi` CSV with 3-second rate-limiting to prevent main thread stall).
- **SystemMonitor**: Aggregates all collectors into a `QObject` singleton, exposing reactive `Q_PROPERTY` attributes (`cpuTotal`, `cpuTemperature`, `cpuCores`, `ramPercent`, `networkIfaces`, `thermalSensors`, `gpuUsage`, etc.) to QML.

### `src/config/` — Configuration & Hot-Reload Engine
- **ConfigManager**: Parses TOML configurations without external heavy dependencies.
- **File System Watcher**: Utilizes `QFileSystemWatcher` to monitor `default_config.toml` changes in real-time with atomic-write debouncing (120ms). Automatically updates colors, fonts, opacity, and script definitions on save without app restart.

### `src/scripts/` — Async Script Execution Engine
- **ScriptRunner**: Asynchronously spawns `/bin/bash -c "<cmd>"` calls using `QProcess` for custom user commands defined in TOML.
- **Non-blocking Execution**: Runs periodically (default 5s) with a 10s hard timeout safety guard to prevent hanging child processes. Streams `stdout`/`stderr` and exit status directly to QML widgets.

### `src/ui/` — QML Desktop Widget Frontend
- **Desktop Widget Mode**: Window flags set to `Qt.FramelessWindowHint | Qt.WindowStaysOnBottomHint | Qt.Tool` with transparent window background (`color: "transparent"`).
- **Glassmorphism Design**: Custom rounded container (`radius: 16`), semi-transparent dark backdrop (`#1A1A24B3` or configurable `uiOpacity`), and subtle accent borders.
- **Native Window Drag**: Smooth desktop drag & drop using `root.startSystemMove()`.
- **Collapsible UX & Mini Mode**: Clickable headers for CPU, RAM, NET, TEMP, GPU, and SCRIPTS sections with expand/collapse states and compact Mini Mode toggle.

---

## 4. Performance & Engineering Constraints
- **Zero Memory Allocation in Polling Loops**: String parsing avoids Heap allocations by leveraging stack buffers and `std::from_chars`.
- **Thread Safety**: UI remains strictly at 60 FPS while background polling and process management run asynchronously.