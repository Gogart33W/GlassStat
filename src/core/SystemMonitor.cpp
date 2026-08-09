#include "core/SystemMonitor.hpp"

#include <QStringLiteral>

namespace gs {

SystemMonitor::SystemMonitor(QObject* parent) : QObject(parent) {
    static_cast<void>(m_cpu.perCorePoll());  // seed first /proc/stat snapshot

    m_hasGpu    = m_gpu.supported();
    m_gpuVendor = QString::fromStdString(m_gpu.vendor());

    m_timer.setInterval(1000);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &SystemMonitor::tick);
    m_timer.start();
}

void SystemMonitor::tick() {
    // ── CPU ──────────────────────────────────────────────────────────────────
    const auto& cores = m_cpu.perCorePoll();
    if (!cores.empty()) {
        m_cpuTotal = cores[0];
        m_cpuCores.clear();
        m_cpuCores.reserve(static_cast<qsizetype>(cores.size() - 1));
        for (std::size_t i = 1; i < cores.size(); ++i)
            m_cpuCores.append(cores[i]);
    }

    // ── Memory ───────────────────────────────────────────────────────────────
    const auto mem = m_mem.poll();
    m_ramPct   = mem.ramPercent();
    m_swapPct  = mem.swapPercent();
    m_ramUsed  = mem.usedMiB();
    m_ramTotal = mem.totalMiB();

    // ── Network ──────────────────────────────────────────────────────────────
    m_netIfaces.clear();
    for (const auto& s : m_net.poll()) {
        QVariantMap m;
        m[QStringLiteral("name")] = QString::fromStdString(s.name);
        m[QStringLiteral("rx")]   = s.rx_kbps;
        m[QStringLiteral("tx")]   = s.tx_kbps;
        m_netIfaces.append(std::move(m));
    }

    // ── Thermal ──────────────────────────────────────────────────────────────
    m_thermalSensors.clear();
    double cpuTempCandidate = 0.0;
    double maxTempCandidate = 0.0;

    for (const auto& s : m_thermal.poll()) {
        QVariantMap m;
        m[QStringLiteral("name")] = QString::fromStdString(s.hwmon_name);
        m[QStringLiteral("temp")] = static_cast<double>(s.max_temp_c);
        m_thermalSensors.append(std::move(m));

        const double temp = static_cast<double>(s.max_temp_c);
        if (temp > maxTempCandidate) maxTempCandidate = temp;

        const QString n = QString::fromStdString(s.hwmon_name).toLower();
        if (n.contains(QLatin1String("coretemp")) ||
            n.contains(QLatin1String("k10temp"))  ||
            n.contains(QLatin1String("zenpower")) ||
            n.contains(QLatin1String("cpu"))      ||
            n.contains(QLatin1String("acpitz"))) {
            if (temp > cpuTempCandidate) cpuTempCandidate = temp;
        }
    }
    m_cpuTemp = (cpuTempCandidate > 0.0) ? cpuTempCandidate : maxTempCandidate;

    // ── GPU ──────────────────────────────────────────────────────────────────
    if (m_hasGpu) {
        const auto g = m_gpu.poll();
        m_gpuUsage     = g.usage_pct;
        m_gpuTemp      = static_cast<double>(g.temp_c);
        m_gpuVramUsed  = g.vram_used_mib;
        m_gpuVramTotal = g.vram_total_mib;
    }

    emit metricsChanged();
}

void SystemMonitor::setPollInterval(int ms) noexcept {
    if (ms > 0) m_timer.setInterval(ms);
}

} // namespace gs
