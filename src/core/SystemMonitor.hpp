#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>

#include "core/CpuCollector.hpp"
#include "core/GpuCollector.hpp"
#include "core/MemoryCollector.hpp"
#include "core/NetworkCollector.hpp"
#include "core/ThermalCollector.hpp"

namespace gs {

class SystemMonitor : public QObject {
    Q_OBJECT

    // ── CPU ───────────────────────────────────────────────────────────────────
    Q_PROPERTY(double       cpuTotal       READ cpuTotal       NOTIFY metricsChanged)
    Q_PROPERTY(double       cpuTemperature READ cpuTemperature NOTIFY metricsChanged)
    Q_PROPERTY(QVariantList cpuCores       READ cpuCores       NOTIFY metricsChanged)
    // ── Memory ────────────────────────────────────────────────────────────────
    Q_PROPERTY(double       ramPercent  READ ramPercent  NOTIFY metricsChanged)
    Q_PROPERTY(double       swapPercent READ swapPercent NOTIFY metricsChanged)
    Q_PROPERTY(int          ramUsedMiB  READ ramUsedMiB  NOTIFY metricsChanged)
    Q_PROPERTY(int          ramTotalMiB READ ramTotalMiB NOTIFY metricsChanged)
    // ── Network ───────────────────────────────────────────────────────────────
    Q_PROPERTY(QVariantList networkIfaces  READ networkIfaces  NOTIFY metricsChanged)
    // ── Thermal ───────────────────────────────────────────────────────────────
    Q_PROPERTY(QVariantList thermalSensors READ thermalSensors NOTIFY metricsChanged)
    // ── GPU ───────────────────────────────────────────────────────────────────
    Q_PROPERTY(bool    hasGpu      READ hasGpu      CONSTANT)
    Q_PROPERTY(QString gpuVendor   READ gpuVendor   CONSTANT)
    Q_PROPERTY(int     gpuUsage    READ gpuUsage    NOTIFY metricsChanged)
    Q_PROPERTY(double  gpuTemp     READ gpuTemp     NOTIFY metricsChanged)
    Q_PROPERTY(int     gpuVramUsed READ gpuVramUsed NOTIFY metricsChanged)
    Q_PROPERTY(int     gpuVramTotal READ gpuVramTotal NOTIFY metricsChanged)

public:
    explicit SystemMonitor(QObject* parent = nullptr);

    double       cpuTotal()       const noexcept { return m_cpuTotal;  }
    double       cpuTemperature() const noexcept { return m_cpuTemp;   }
    QVariantList cpuCores()       const          { return m_cpuCores;  }
    double       ramPercent()   const noexcept { return m_ramPct;    }
    double       swapPercent()  const noexcept { return m_swapPct;   }
    int          ramUsedMiB()   const noexcept { return m_ramUsed;   }
    int          ramTotalMiB()  const noexcept { return m_ramTotal;  }
    QVariantList networkIfaces()   const       { return m_netIfaces; }
    QVariantList thermalSensors()  const       { return m_thermalSensors; }
    bool         hasGpu()       const noexcept { return m_hasGpu;    }
    QString      gpuVendor()    const          { return m_gpuVendor; }
    int          gpuUsage()     const noexcept { return m_gpuUsage;  }
    double       gpuTemp()      const noexcept { return m_gpuTemp;   }
    int          gpuVramUsed()  const noexcept { return m_gpuVramUsed;  }
    int          gpuVramTotal() const noexcept { return m_gpuVramTotal; }

    void setPollInterval(int ms) noexcept;

signals:
    void metricsChanged();

private slots:
    void tick();

private:
    QTimer                 m_timer;
    core::CpuCollector     m_cpu;
    core::MemoryCollector  m_mem;
    core::NetworkCollector m_net;
    core::ThermalCollector m_thermal;
    core::GpuCollector     m_gpu;

    double       m_cpuTotal  = 0.0;
    double       m_cpuTemp   = 0.0;
    QVariantList m_cpuCores;
    double       m_ramPct    = 0.0;
    double       m_swapPct   = 0.0;
    int          m_ramUsed   = 0;
    int          m_ramTotal  = 0;
    QVariantList m_netIfaces;
    QVariantList m_thermalSensors;
    bool         m_hasGpu        = false;
    QString      m_gpuVendor;
    int          m_gpuUsage      = -1;
    double       m_gpuTemp       = 0.0;
    int          m_gpuVramUsed   = 0;
    int          m_gpuVramTotal  = 0;
};

} // namespace gs
