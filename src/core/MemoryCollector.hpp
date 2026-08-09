#pragma once

#include <cstdint>
#include <string_view>

namespace gs::core {

struct MemInfo {
    uint64_t total_kib      = 0;
    uint64_t available_kib  = 0;
    uint64_t swap_total_kib = 0;
    uint64_t swap_free_kib  = 0;

    [[nodiscard]] double ramPercent() const noexcept {
        if (total_kib == 0) return 0.0;
        return 100.0 * (1.0 - static_cast<double>(available_kib) / static_cast<double>(total_kib));
    }
    [[nodiscard]] double swapPercent() const noexcept {
        if (swap_total_kib == 0) return 0.0;
        return 100.0 * (1.0 - static_cast<double>(swap_free_kib) / static_cast<double>(swap_total_kib));
    }
    [[nodiscard]] int usedMiB()  const noexcept { return static_cast<int>((total_kib - available_kib) / 1024); }
    [[nodiscard]] int totalMiB() const noexcept { return static_cast<int>(total_kib / 1024); }
};

class MemoryCollector {
public:
    MemoryCollector() = default;
    [[nodiscard]] MemInfo poll() noexcept;

private:
    // Returns true iff `line` starts with `key` and `out` was populated.
    static bool parseField(std::string_view line, std::string_view key, uint64_t& out) noexcept;
};

} // namespace gs::core
