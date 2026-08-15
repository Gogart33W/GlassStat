#include "ui/WindowManager.hpp"
#include "platform/X11WindowHelper.hpp"

namespace gs {

WindowManager::WindowManager(ConfigManager* cfg, QObject* parent)
    : QObject(parent), m_config(cfg)
{
    if (m_config) {
        connect(m_config, &ConfigManager::windowModeChanged, this, &WindowManager::applyState);
        connect(m_config, &ConfigManager::clickThroughChanged, this, &WindowManager::applyState);
        connect(m_config, &ConfigManager::configChanged, this, &WindowManager::applyState);
    }
}

void WindowManager::registerWindow(QWindow* window) {
    if (!window) return;
    m_window = window;

    // Apply state on initial window creation
    applyState();
}

void WindowManager::applyState() {
    if (!m_window || !m_config) return;

    const QString mode = m_config->windowMode();
    const bool clickThrough = m_config->clickThrough();

    // 1. Qt Window Flags
    Qt::WindowFlags flags = Qt::FramelessWindowHint;
    if (mode == QStringLiteral("desktop")) {
        flags |= Qt::WindowStaysOnBottomHint;
    } else if (mode == QStringLiteral("top")) {
        flags |= Qt::WindowStaysOnTopHint;
    }

    if (m_window->flags() != flags) {
        m_window->setFlags(flags);
    }

    // 2. Native X11 Atom Property & Click-Through
    if (m_config->isX11()) {
        platform::X11WindowHelper::setWindowType(m_window, mode == QStringLiteral("desktop"));

        // Click-Through is strictly enabled ONLY when in desktop mode AND clickThrough is true
        const bool effectiveClickThrough = (mode == QStringLiteral("desktop")) && clickThrough;
        platform::X11WindowHelper::setClickThrough(m_window, effectiveClickThrough);
    }
}

} // namespace gs
