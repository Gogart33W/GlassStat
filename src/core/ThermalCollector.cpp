#include "core/ThermalCollector.hpp"

#include <charconv>
#include <filesystem>
#include <fstream>

namespace gs::core {

namespace fs = std::filesystem;

std::string ThermalCollector::readSysFile(const std::string& path) noexcept {
    std::ifstream f(path);
    std::string s;
    if (f.is_open()) std::getline(f, s);
    return s;
}

std::vector<ThermalReading> ThermalCollector::poll() noexcept {
    std::vector<ThermalReading> result;
    std::error_code ec;

    for (const auto& hwmon : fs::directory_iterator("/sys/class/hwmon", ec)) {
        if (ec) { ec.clear(); break; }

        const std::string base = hwmon.path().string();
        const std::string name = readSysFile(base + "/name");
        if (name.empty()) continue;

        float max_temp = 0.0f;
        bool  found    = false;

        // Scan all temp*_input files; values are millidegrees Celsius.
        for (const auto& file : fs::directory_iterator(base, ec)) {
            if (ec) { ec.clear(); break; }
            const std::string fname = file.path().filename().string();
            if (!fname.starts_with("temp") || !fname.ends_with("_input")) continue;

            const auto raw = readSysFile(file.path().string());
            if (raw.empty()) continue;

            int32_t millideg = 0;
            if (std::from_chars(raw.data(), raw.data() + raw.size(), millideg).ec != std::errc{}) continue;

            const float t = static_cast<float>(millideg) / 1000.0f;
            if (t > max_temp) max_temp = t;
            found = true;
        }

        if (found) result.push_back({ name, max_temp });
    }
    return result;
}

} // namespace gs::core
