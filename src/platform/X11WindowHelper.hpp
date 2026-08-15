#pragma once

#include <QWindow>

namespace gs::platform {

class X11WindowHelper {
public:
    static void setWindowType(QWindow* window, bool isDesktop) noexcept;
    static void setClickThrough(QWindow* window, bool clickThrough) noexcept;
};

} // namespace gs::platform
