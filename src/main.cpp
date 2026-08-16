#include "config/ConfigManager.hpp"
#include "core/SystemMonitor.hpp"
#include "scripts/ScriptRunner.hpp"
#include "ui/TrayController.hpp"
#include "ui/WindowManager.hpp"
#include "version.hpp"

#include <QApplication>
#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QScreen>
#include <QStandardPaths>
#include <QtQml>
#include <algorithm>
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("GlassStat");
    app.setApplicationName("GlassStat");
    app.setApplicationVersion("0.1.0");
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/glassstat.svg")));
    std::cerr << "[GlassStat] version " << GLASSSTAT_VERSION_STRING
              << " (" << GLASSSTAT_GIT_HASH << ")\n";

    // ── Single-instance guard ─────────────────────────────────────────────────
    const QString lockDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir().mkpath(lockDir);
    QLockFile instanceLock(lockDir + QStringLiteral("/glassstat.lock"));
    instanceLock.setStaleLockTime(0);
    if (!instanceLock.tryLock(100)) {
        std::cerr << "[GlassStat] Already running - refusing to start a second instance.\n";
        return 0;
    }

    // ── DPI-aware default scale (computed before ConfigManager loads from file)
    const double autoScale = [&]() -> double {
        if (const QScreen* s = QGuiApplication::primaryScreen()) {
            const double dpi = s->logicalDotsPerInch();
            return std::clamp(dpi / 96.0, 0.6, 2.5);
        }
        return 1.0;
    }();

    // ── Config (must be first — everything else reads from it) ───────────────
    auto* cfg = new gs::ConfigManager(gs::ConfigManager::findConfigPath(), autoScale, &app);
    // ↑ autoScale is now in m_autoScale *before* the constructor's load() runs,
    // so the very first applyData() uses the real DPI default on a fresh install.

    qmlRegisterSingletonInstance<gs::ConfigManager>("GlassStat", 1, 0, "ConfigManager", cfg);

    // ── System Monitor ────────────────────────────────────────────────────────
    auto* monitor = new gs::SystemMonitor(&app);
    monitor->setPollInterval(cfg->pollIntervalMs());
    qmlRegisterSingletonInstance<gs::SystemMonitor>("GlassStat", 1, 0, "SystemMonitor", monitor);

    // ── Script Runner ─────────────────────────────────────────────────────────
    auto* runner = new gs::ScriptRunner(&app);
    runner->setScripts(cfg->scriptDefs());
    qmlRegisterSingletonInstance<gs::ScriptRunner>("GlassStat", 1, 0, "ScriptRunner", runner);

    // ── System Tray Controller ────────────────────────────────────────────────
    auto* tray = new gs::TrayController(cfg, &app);
    qmlRegisterSingletonInstance<gs::TrayController>("GlassStat", 1, 0, "TrayController", tray);

    // ── Window Manager ────────────────────────────────────────────────────────
    auto* winMgr = new gs::WindowManager(cfg, &app);
    qmlRegisterSingletonInstance<gs::WindowManager>("GlassStat", 1, 0, "WindowManager", winMgr);

    // ── Hot-reload wiring ─────────────────────────────────────────────────────
    QObject::connect(cfg, &gs::ConfigManager::configChanged, monitor,
        [cfg, monitor]() { monitor->setPollInterval(cfg->pollIntervalMs()); });

    QObject::connect(cfg, &gs::ConfigManager::configChanged, runner,
        [cfg, runner]() { runner->setScripts(cfg->scriptDefs()); });

    // ── QML engine ────────────────────────────────────────────────────────────
    QQmlApplicationEngine engine;
    engine.loadFromModule("GlassStat", "Main");

    if (engine.rootObjects().isEmpty())
        return 1;

    return QApplication::exec();
}
