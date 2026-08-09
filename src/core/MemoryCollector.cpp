#include "core/MemoryCollector.hpp"

#include <charconv>
#include <fstream>
#include <string>

namespace gs::core {

// "MemTotal:        16384000 kB" — skip label + colon, skip spaces, parse uint64
bool MemoryCollector::parseField(std::string_view line, std::string_view key, uint64_t& out) noexcept {
    if (!line.starts_with(key)) return false;

    const auto colon = line.find(':');
    if (colon == std::string_view::npos) return false;

    const char* p   = line.data() + colon + 1;
    const char* end = line.data() + line.size();
    while (p < end && *p == ' ') ++p;

    const auto [next, ec] = std::from_chars(p, end, out);
    return ec == std::errc{};
}

MemInfo MemoryCollector::poll() noexcept {
    MemInfo info{};
    std::ifstream f("/proc/meminfo");
    if (!f.is_open()) return info;

    // /proc/meminfo is stable-ordered; bail early once all 4 fields found.
    std::string line;
    int remaining = 4;
    while (remaining > 0 && std::getline(f, line)) {
        if (parseField(line, "MemTotal",     info.total_kib))      { --remaining; continue; }
        if (parseField(line, "MemAvailable", info.available_kib))  { --remaining; continue; }
        if (parseField(line, "SwapTotal",    info.swap_total_kib)) { --remaining; continue; }
        if (parseField(line, "SwapFree",     info.swap_free_kib))  { --remaining; continue; }
    }
    return info;
}

} // namespace gs::core
