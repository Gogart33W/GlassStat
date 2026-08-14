#include "ui/TrayController.hpp"

#include <QCoreApplication>
#include <iostream>

namespace gs {

TrayController::TrayController(ConfigManager* cfgManager, QObject* parent)
    : QObject(parent), m_cfgManager(cfgManager)
{
    m_trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();

    if (!m_trayAvailable) {
        std::cerr << "[TrayController] System tray is not available on this environment. Fallback active.\n";
        return;
    }

    createTrayIcon();
}

QIcon TrayController::generateAppIcon() {
    QPixmap pix(32, 32);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Dark glass background circle
    p.setBrush(QColor(139, 92, 246, 220)); // Purple accent
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(2, 2, 28, 28, 8, 8);

    // Inner symbol
    p.setPen(QPen(Qt::white, 2.5));
    p.drawArc(8, 8, 16, 16, 45 * 16, 270 * 16);
    p.drawLine(16, 16, 24, 16);

    return QIcon(pix);
}

void TrayController::createTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(generateAppIcon(), this);
    m_trayIcon->setToolTip(QStringLiteral("GlassStat System Monitor"));

    m_trayMenu = new QMenu();

    // Show/Hide Action
    m_showHideAction = m_trayMenu->addAction(QStringLiteral("Hide Widget"), this, &TrayController::toggleWidgetVisible);

    // Lock Position Action
    m_lockAction = m_trayMenu->addAction(QStringLiteral("Lock Position"), this, &TrayController::toggleLocked);
    m_lockAction->setCheckable(true);
    m_lockAction->setChecked(m_locked);

    m_trayMenu->addSeparator();

    // Themes Submenu
    QMenu* themeMenu = m_trayMenu->addMenu(QStringLiteral("Theme Preset"));
    if (m_cfgManager) {
        themeMenu->addAction(QStringLiteral("Dark Purple"), [this]() { m_cfgManager->setPreset(QStringLiteral("dark")); });
        themeMenu->addAction(QStringLiteral("Cyberpunk Neon"), [this]() { m_cfgManager->setPreset(QStringLiteral("cyberpunk")); });
        themeMenu->addAction(QStringLiteral("Emerald Glass"), [this]() { m_cfgManager->setPreset(QStringLiteral("emerald")); });
        themeMenu->addAction(QStringLiteral("Sunset Amber"), [this]() { m_cfgManager->setPreset(QStringLiteral("amber")); });
    }

    m_trayMenu->addSeparator();

    // Quit Action
    m_trayMenu->addAction(QStringLiteral("Quit GlassStat"), []() {
        QCoreApplication::quit();
    });

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            toggleWidgetVisible();
        }
    });

    m_trayIcon->show();
}

void TrayController::setLocked(bool locked) {
    if (m_locked == locked) return;
    m_locked = locked;
    if (m_lockAction) m_lockAction->setChecked(m_locked);
    emit lockedChanged(m_locked);
}

void TrayController::setWidgetVisible(bool visible) {
    if (m_widgetVisible == visible) return;
    m_widgetVisible = visible;
    if (m_showHideAction) {
        m_showHideAction->setText(m_widgetVisible ? QStringLiteral("Hide Widget") : QStringLiteral("Show Widget"));
    }
    emit widgetVisibleChanged(m_widgetVisible);
}

void TrayController::toggleWidgetVisible() {
    setWidgetVisible(!m_widgetVisible);
}

void TrayController::toggleLocked() {
    setLocked(!m_locked);
}

} // namespace gs
