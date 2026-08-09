#include "core/CpuCollector.hpp"

#include <charconv>
#include <fstream>
#include <string>
#include <vector>

namespace gs::core {

// /proc/stat line: "cpu  <user> <nice> <system> <idle> <iowait> <irq> <softirq> <steal> ..."
// We skip the label token and parse the first 8 numeric fields.
CpuJiffies CpuCollector::parseLine(std::string_view line) noexcept {
    CpuJiffies j{};
    uint64_t* fields[] = {
        &j.user, &j.nice, &j.system, &j.idle,
        &j.iowait, &j.irq, &j.softirq, &j.steal
    };

    // skip label (e.g. "cpu", "cpu0" …)
    auto pos = line.find(' ');
    if (pos == std::string_view::npos) return j;

    const char* p   = line.data() + pos;
    const char* end = line.data() + line.size();

    for (auto* dst : fields) {
        while (p < end && *p == ' ') ++p;
        auto [next, ec] = std::from_chars(p, end, *dst);
        if (ec != std::errc{}) break;
        p = next;
    }
    return j;
}

double CpuCollector::poll() noexcept {
    static_cast<void>(perCorePoll());
    return m_usage.empty() ? 0.0 : m_usage[0];
}

const std::vector<double>& CpuCollector::perCorePoll() noexcept {
    std::ifstream f("/proc/stat");
    if (!f.is_open()) return m_usage;

    std::vector<CpuJiffies> cur;
    cur.reserve(m_prev.empty() ? 17 : m_prev.size());

    std::string line;
    while (std::getline(f, line)) {
        if (line.compare(0, 3, "cpu") != 0) break;
        cur.push_back(parseLine(line));
    }

    if (m_prev.empty()) {
        m_prev = std::move(cur);
        m_usage.assign(m_prev.size(), 0.0);
        return m_usage;
    }

    const std::size_t n = std::min(cur.size(), m_prev.size());
    m_usage.resize(n);

    for (std::size_t i = 0; i < n; ++i) {
        const uint64_t dtotal  = cur[i].total()  - m_prev[i].total();
        const uint64_t dactive = cur[i].active() - m_prev[i].active();
        // Guard against stale reads or kernel tick wrap
        m_usage[i] = (dtotal > 0)
            ? 100.0 * static_cast<double>(dactive) / static_cast<double>(dtotal)
            : 0.0;
    }

    m_prev = std::move(cur);
    return m_usage;
}

} // namespace gs::core
