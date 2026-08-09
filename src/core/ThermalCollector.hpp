#pragma once

#include <string>
#include <vector>

namespace gs::core {

struct ThermalReading {
    std::string hwmon_name;   // e.g. "coretemp", "acpitz", "amdgpu"
    float       max_temp_c = 0.0f;
};

class ThermalCollector {
public:
    ThermalCollector() = default;
    [[nodiscard]] std::vector<ThermalReading> poll() noexcept;

private:
    static std::string readSysFile(const std::string& path) noexcept;
};

} // namespace gs::core
