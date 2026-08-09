#include "core/NetworkCollector.hpp"

#include <charconv>
#include <fstream>
#include <string>

namespace gs::core {

// /proc/net/dev line (after 2 header lines):
//   "  eth0:  12345  67  0  0  0  0  0  0  99999  ..."
//   fields post-colon: rx_bytes(0), rx_pkts(1)…rx_multicast(7), tx_bytes(8)
std::vector<NetworkCollector::RawSnapshot> NetworkCollector::readProc() noexcept {
    std::vector<RawSnapshot> result;
    std::ifstream f("/proc/net/dev");
    if (!f.is_open()) return result;

    std::string line;
    std::getline(f, line);  // Inter- header
    std::getline(f, line);  // column labels

    while (std::getline(f, line)) {
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;

        const auto ns = line.find_first_not_of(' ');
        std::string name = line.substr(ns, colon - ns);

        const char* p   = line.data() + colon + 1;
        const char* end = line.data() + line.size();

        uint64_t fields[9]{};
        for (auto& field : fields) {
            while (p < end && *p == ' ') ++p;
            auto [next, ec] = std::from_chars(p, end, field);
            if (ec != std::errc{}) break;
            p = next;
        }
        result.push_back({ std::move(name), fields[0], fields[8] });
    }
    return result;
}

std::vector<NetIfaceStats> NetworkCollector::poll() noexcept {
    const auto now = std::chrono::steady_clock::now();
    auto cur = readProc();

    if (!m_seeded) {
        m_prev     = std::move(cur);
        m_prevTime = now;
        m_seeded   = true;
        return {};
    }

    const double dt = std::chrono::duration<double>(now - m_prevTime).count();
    m_prevTime = now;

    std::vector<NetIfaceStats> result;
    if (dt < 1e-6) { m_prev = std::move(cur); return result; }

    for (const auto& c : cur) {
        if (c.name == "lo") continue;
        for (const auto& prev : m_prev) {
            if (c.name != prev.name) continue;
            NetIfaceStats s;
            s.name    = c.name;
            s.rx_kbps = (c.rx >= prev.rx) ? static_cast<double>(c.rx - prev.rx) / (dt * 1024.0) : 0.0;
            s.tx_kbps = (c.tx >= prev.tx) ? static_cast<double>(c.tx - prev.tx) / (dt * 1024.0) : 0.0;
            result.push_back(std::move(s));
            break;
        }
    }

    m_prev = std::move(cur);
    return result;
}

} // namespace gs::core
