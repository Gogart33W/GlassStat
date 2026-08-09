# GlassStat 🔮

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg?style=flat-square&logo=c%2B%2B)
![Qt6](https://img.shields.io/badge/Qt-6.11-green.svg?style=flat-square&logo=qt)
![Arch Linux](https://img.shields.io/badge/Platform-Arch%20Linux-blueviolet.svg?style=flat-square&logo=arch-linux)
![License](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)

**GlassStat** is a modern, lightweight, frameless desktop system monitor widget for Linux. Engineered with zero-overhead C++20 kernel file parsing (`/proc`, `/sys`) and hardware-accelerated Qt6 QML, it delivers real-time system metrics with a sleek dark glassmorphism design.

---

## ✨ Features

- **⚡ Real-time Core Metrics**:
  - **CPU**: Per-core utilization, total load %, and core temperature monitoring with high-temp warnings (>85°C).
  - **RAM & Swap**: Real-time memory usage (MiB / GiB) and swap space tracking.
  - **Network**: Active interface filtering with dynamic RX/TX speed auto-scaling (KB/s, MB/s, GB/s).
  - **Thermal**: Automatic hardware sensor detection (`hwmon`) formatted in a compact 2-column grid.
  - **GPU Support**: Auto-detects AMD (`amdgpu` hwmon / DRM sysfs) and NVIDIA (`nvidia-smi` rate-limited) usage, VRAM, and temperatures.

- **🪟 Desktop Widget Mode & UX**:
  - Frameless, desktop-docked widget (`Qt.WindowStaysOnBottomHint | Qt.Tool`) with transparent backdrop.
  - Smooth native window dragging (`Window.startSystemMove()`).
  - Interactive collapsible sections with Mini Mode toggle for ultra-compact system monitoring.

- **⚙️ Hot-Reloadable TOML Configuration**:
  - Customize colors, opacity, fonts, and polling intervals dynamically via `config/default_config.toml`.
  - Built-in `QFileSystemWatcher` applies config updates instantly without application restart.

- **💻 Custom Script Execution Engine**:
  - Asynchronously run custom shell commands (`uptime`, `kernel`, `df`, etc.) and render output directly inside the widget.

---

## 🛠️ Building from Source

### Prerequisites

Ensure the following dependencies are installed on Arch Linux:

```bash
sudo pacman -S base-devel cmake ninja qt6-base qt6-declarative
```

### Build Instructions

1. **Clone the repository**:
   ```bash
   git clone https://github.com/Gogart33W/GlassStat.git
   cd GlassStat
   ```

2. **Configure with CMake**:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   ```

3. **Compile**:
   ```bash
   ninja -C build
   ```

4. **Run GlassStat**:
   ```bash
   ./build/glassstat
   ```

---

## 📐 Architecture & Internal Design

For detailed technical specification, data flow diagrams, and subsystem breakdown, refer to [ARCHITECTURE.md](file:///home/gogart/Projects/GlassStat/ARCHITECTURE.md).

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).
