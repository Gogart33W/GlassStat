#include "config/ConfigManager.hpp"
#include "core/SystemMonitor.hpp"
#include "scripts/ScriptRunner.hpp"
#include "ui/TrayController.hpp"
#include "ui/WindowManager.hpp"

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QStandardPaths>
#include <QtQml>
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("GlassStat");
    app.setApplicationName("GlassStat");
    app.setApplicationVersion("0.1.0");
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/glassstat.svg")));

    // ── Single-instance guard ─────────────────────────────────────────────────
    const QString lockDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir().mkpath(lockDir);
    QLockFile instanceLock(lockDir + QStringLiteral("/glassstat.lock"));
    instanceLock.setStaleLockTime(0);
    if (!instanceLock.tryLock(100)) {
        std::cerr << "[GlassStat] Already running - refusing to start a second instance.\n";
        return 0;
    }

    // ── Config (must be first — everything else reads from it) ───────────────
    auto* cfg = new gs::ConfigManager(gs::ConfigManager::findConfigPath(), &app);
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
