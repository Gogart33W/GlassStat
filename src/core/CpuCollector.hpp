#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace gs::core {

// Raw jiffies snapshot from one /proc/stat line
struct CpuJiffies {
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;

    [[nodiscard]] uint64_t total() const noexcept {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }
    [[nodiscard]] uint64_t active() const noexcept {
        return total() - idle - iowait;
    }
};

class CpuCollector {
public:
    CpuCollector() = default;

    // Returns [0.0, 100.0] overall CPU usage since last call.
    // First call always returns 0.0 (no prior delta available).
    [[nodiscard]] double poll() noexcept;

    // Per-core usage; index 0 = aggregate, 1..N = core0..coreN-1
    [[nodiscard]] const std::vector<double>& perCorePoll() noexcept;

private:
    static CpuJiffies parseLine(std::string_view line) noexcept;

    std::vector<CpuJiffies> m_prev;
    std::vector<double>     m_usage;  // cached result for perCorePoll
};

} // namespace gs::core
