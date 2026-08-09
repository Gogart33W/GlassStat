#include "config/ConfigManager.hpp"
#include "core/SystemMonitor.hpp"
#include "scripts/ScriptRunner.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName("GlassStat");
    app.setApplicationName("GlassStat");
    app.setApplicationVersion("0.1.0");

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

    return QGuiApplication::exec();
}
