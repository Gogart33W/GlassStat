#pragma once

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QSystemTrayIcon>

#include "config/ConfigManager.hpp"

namespace gs {

class TrayController : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isLocked        READ isLocked        WRITE setLocked        NOTIFY lockedChanged)
    Q_PROPERTY(bool isWidgetVisible READ isWidgetVisible WRITE setWidgetVisible NOTIFY widgetVisibleChanged)
    Q_PROPERTY(bool isTrayAvailable READ isTrayAvailable CONSTANT)

public:
    explicit TrayController(ConfigManager* cfgManager, QObject* parent = nullptr);

    bool isLocked()        const noexcept { return m_locked; }
    bool isWidgetVisible() const noexcept { return m_widgetVisible; }
    bool isTrayAvailable() const noexcept { return m_trayAvailable; }

public slots:
    void setLocked(bool locked);
    void setWidgetVisible(bool visible);
    void toggleWidgetVisible();
    void toggleLocked();

signals:
    void lockedChanged(bool locked);
    void widgetVisibleChanged(bool visible);
    void preferencesRequested();

private:
    void createTrayIcon();
    void updateMenuStates();
    static QIcon appIcon();
    static QIcon generateAppIcon();

    ConfigManager*    m_cfgManager = nullptr;
    QSystemTrayIcon*  m_trayIcon   = nullptr;
    QMenu*            m_trayMenu   = nullptr;

    QAction*          m_showHideAction     = nullptr;
    QAction*          m_lockAction         = nullptr;
    QAction*          m_preferencesAction  = nullptr;
    QAction*          m_modeDesktopAction  = nullptr;
    QAction*          m_modeFloatingAction = nullptr;
    QAction*          m_modeTopAction      = nullptr;
    QAction*          m_clickThroughAction = nullptr;
    QActionGroup*     m_modeActionGroup    = nullptr;

    bool m_locked        = false;
    bool m_widgetVisible = true;
    bool m_trayAvailable = false;
};

} // namespace gs
