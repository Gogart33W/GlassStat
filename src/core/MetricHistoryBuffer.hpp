#pragma once

#include <QVariantList>
#include <cstddef>
#include <deque>

namespace gs::core {

template <std::size_t Capacity = 60>
class MetricHistoryBuffer {
public:
    void push(double value) noexcept {
        if (m_data.size() >= Capacity) {
            m_data.pop_front();
        }
        m_data.push_back(value);
    }

    [[nodiscard]] QVariantList toVariantList() const {
        QVariantList list;
        list.reserve(static_cast<qsizetype>(m_data.size()));
        for (double v : m_data) {
            list.append(v);
        }
        return list;
    }

    [[nodiscard]] bool empty() const noexcept { return m_data.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return m_data.size(); }

private:
    std::deque<double> m_data;
};

} // namespace gs::core
