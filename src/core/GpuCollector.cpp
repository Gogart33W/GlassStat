#include "core/GpuCollector.hpp"

#include <charconv>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace gs::core {

namespace fs = std::filesystem;

std::string GpuCollector::readSysFile(const std::string& path) noexcept {
    std::ifstream f(path);
    std::string s;
    if (f.is_open()) std::getline(f, s);
    return s;
}

int GpuCollector::readInt(const std::string& path, int fallback) noexcept {
    const auto s = readSysFile(path);
    if (s.empty()) return fallback;
    int v = fallback;
    std::from_chars(s.data(), s.data() + s.size(), v);
    return v;
}

uint64_t GpuCollector::readUInt64(const std::string& path) noexcept {
    const auto s = readSysFile(path);
    uint64_t v = 0;
    std::from_chars(s.data(), s.data() + s.size(), v);
    return v;
}

std::string GpuCollector::vendor() const noexcept {
    switch (m_backend) {
        case Backend::AMD:    return "AMD";
        case Backend::NVIDIA: return "NVIDIA";
        default:              return "";
    }
}

GpuCollector::GpuCollector() { detectBackend(); }

void GpuCollector::detectBackend() noexcept {
    std::error_code ec;

    // ── AMD via hwmon ─────────────────────────────────────────────────────────
    for (const auto& entry : fs::directory_iterator("/sys/class/hwmon", ec)) {
        if (ec) { ec.clear(); break; }
        if (readSysFile(entry.path().string() + "/name") != "amdgpu") continue;

        m_hwmon_path = entry.path().string();

        // Locate the DRM card by presence of gpu_busy_percent in its device dir
        for (const auto& card : fs::directory_iterator("/sys/class/drm", ec)) {
            if (ec) { ec.clear(); break; }
            const std::string cname = card.path().filename().string();
            // Skip connector entries like "card0-HDMI-A-1"
            if (cname.rfind("card", 0) != 0 || cname.find('-') != std::string::npos) continue;
            const std::string busy = card.path().string() + "/device/gpu_busy_percent";
            if (fs::exists(busy, ec) && !ec) {
                m_drm_path = card.path().string() + "/device";
                break;
            }
            ec.clear();
        }

        m_backend = Backend::AMD;
        return;
    }

    // ── NVIDIA via nvidia-smi ─────────────────────────────────────────────────
    std::FILE* p = popen("nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null", "r");
    if (!p) return;
    char buf[128]{};
    const bool ok = std::fgets(buf, sizeof(buf), p) != nullptr && buf[0] != '\0' && buf[0] != '\n';
    pclose(p);
    if (ok) m_backend = Backend::NVIDIA;
}

GpuInfo GpuCollector::pollAMD() noexcept {
    GpuInfo info;
    info.vendor = "AMD";

    // Temperature: temp1_input in millidegrees
    const auto raw_temp = readSysFile(m_hwmon_path + "/temp1_input");
    if (!raw_temp.empty()) {
        int32_t md = 0;
        if (std::from_chars(raw_temp.data(), raw_temp.data() + raw_temp.size(), md).ec == std::errc{})
            info.temp_c = static_cast<float>(md) / 1000.0f;
    }

    if (!m_drm_path.empty()) {
        info.usage_pct = readInt(m_drm_path + "/gpu_busy_percent", -1);

        const uint64_t used  = readUInt64(m_drm_path + "/mem_info_vram_used");
        const uint64_t total = readUInt64(m_drm_path + "/mem_info_vram_total");
        info.vram_used_mib  = static_cast<int>(used  / (1024ULL * 1024ULL));
        info.vram_total_mib = static_cast<int>(total / (1024ULL * 1024ULL));
    }
    return info;
}

GpuInfo GpuCollector::pollNVIDIA() noexcept {
    GpuInfo info;
    info.vendor = "NVIDIA";

    std::FILE* pipe = popen(
        "nvidia-smi --query-gpu=utilization.gpu,temperature.gpu,memory.used,memory.total"
        " --format=csv,noheader,nounits 2>/dev/null", "r");
    if (!pipe) return info;

    char buf[256]{};
    const bool got = std::fgets(buf, sizeof(buf), pipe) != nullptr;
    pclose(pipe);
    if (!got) return info;

    // Format: "42, 65, 1024, 8192"
    const char* p   = buf;
    const char* end = buf + std::strlen(buf);

    auto parseField = [&](auto& out) {
        while (p < end && (*p == ' ' || *p == '\t')) ++p;
        auto [next, ec] = std::from_chars(p, end, out);
        if (ec == std::errc{}) p = next;
        while (p < end && *p != ',') ++p;
        if (p < end) ++p;
    };

    int tmp = 0;
    parseField(info.usage_pct);
    parseField(tmp);  info.temp_c = static_cast<float>(tmp);
    parseField(info.vram_used_mib);
    parseField(info.vram_total_mib);
    return info;
}

GpuInfo GpuCollector::poll() noexcept {
    switch (m_backend) {
        case Backend::AMD: return pollAMD();
        case Backend::NVIDIA: {
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - m_lastNvidiaTime).count();
            if (elapsed >= kNvidiaIntervalSec) {
                m_cachedNvidia   = pollNVIDIA();
                m_lastNvidiaTime = now;
            }
            return m_cachedNvidia;
        }
        default: return {};
    }
}

} // namespace gs::core
