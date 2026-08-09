#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace gs::core {

struct NetIfaceStats {
    std::string name;
    double rx_kbps = 0.0;
    double tx_kbps = 0.0;
};

class NetworkCollector {
public:
    NetworkCollector() = default;
    // First call seeds the baseline and returns empty; subsequent calls return deltas.
    [[nodiscard]] std::vector<NetIfaceStats> poll() noexcept;

private:
    struct RawSnapshot {
        std::string name;
        uint64_t    rx = 0;
        uint64_t    tx = 0;
    };

    static std::vector<RawSnapshot> readProc() noexcept;

    std::vector<RawSnapshot>              m_prev;
    std::chrono::steady_clock::time_point m_prevTime;
    bool                                  m_seeded = false;
};

} // namespace gs::core
