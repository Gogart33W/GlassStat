#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace gs::core {

struct GpuInfo {
    int         usage_pct      = -1;   // -1 = unavailable
    float       temp_c         = 0.0f;
    int         vram_used_mib  = 0;
    int         vram_total_mib = 0;
    std::string vendor;                // "AMD" | "NVIDIA" | ""
};

class GpuCollector {
public:
    GpuCollector();

    [[nodiscard]] GpuInfo     poll()      noexcept;
    [[nodiscard]] bool        supported() const noexcept { return m_backend != Backend::None; }
    [[nodiscard]] std::string vendor()    const noexcept;

private:
    enum class Backend { None, AMD, NVIDIA };

    Backend     m_backend    = Backend::None;
    std::string m_hwmon_path;   // AMD: hwmon dir path
    std::string m_drm_path;     // AMD: DRM card device path

    // NVIDIA: avoid popen every tick — poll every kNvidiaIntervalSec seconds
    static constexpr int kNvidiaIntervalSec = 3;
    std::chrono::steady_clock::time_point m_lastNvidiaTime{};
    GpuInfo                               m_cachedNvidia{};

    void    detectBackend() noexcept;
    GpuInfo pollAMD()       noexcept;
    GpuInfo pollNVIDIA()    noexcept;

    static std::string readSysFile(const std::string& path) noexcept;
    static int         readInt(const std::string& path, int fallback = 0) noexcept;
    static uint64_t    readUInt64(const std::string& path) noexcept;
};

} // namespace gs::core
