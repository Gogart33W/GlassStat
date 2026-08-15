#pragma once

#include <QObject>
#include <QWindow>
#include "config/ConfigManager.hpp"

namespace gs {

class WindowManager : public QObject {
    Q_OBJECT

public:
    explicit WindowManager(ConfigManager* cfg, QObject* parent = nullptr);

    Q_INVOKABLE void registerWindow(QWindow* window);

private slots:
    void applyState();

private:
    ConfigManager* m_config = nullptr;
    QWindow*       m_window = nullptr;
};

} // namespace gs
