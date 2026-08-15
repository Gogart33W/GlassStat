#include "platform/X11WindowHelper.hpp"

#if defined(Q_OS_LINUX)
#include <QGuiApplication>
#include <qnativeinterface.h>
#include <xcb/xcb.h>
#include <xcb/shape.h>
#include <cstdlib>
#include <cstring>
#endif

namespace gs::platform {

void X11WindowHelper::setWindowType([[maybe_unused]] QWindow* window, [[maybe_unused]] bool isDesktop) noexcept {
#if defined(Q_OS_LINUX)
    if (!window || !qApp) return;

    auto* x11App = qApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!x11App) return;

    xcb_connection_t* conn = x11App->connection();
    if (!conn) return;

    const xcb_window_t winId = static_cast<xcb_window_t>(window->winId());
    if (winId == 0) return;

    const char* atomNetWmWindowType = "_NET_WM_WINDOW_TYPE";
    const char* atomTypeName = isDesktop ? "_NET_WM_WINDOW_TYPE_DESKTOP" : "_NET_WM_WINDOW_TYPE_NORMAL";

    xcb_intern_atom_cookie_t cookieProp = xcb_intern_atom(conn, 0, std::strlen(atomNetWmWindowType), atomNetWmWindowType);
    xcb_intern_atom_cookie_t cookieType = xcb_intern_atom(conn, 0, std::strlen(atomTypeName), atomTypeName);

    xcb_intern_atom_reply_t* replyProp = xcb_intern_atom_reply(conn, cookieProp, nullptr);
    xcb_intern_atom_reply_t* replyType = xcb_intern_atom_reply(conn, cookieType, nullptr);

    if (replyProp && replyType) {
        xcb_atom_t typeAtom = replyType->atom;
        xcb_change_property(conn, XCB_PROP_MODE_REPLACE, winId, replyProp->atom, XCB_ATOM_ATOM, 32, 1, &typeAtom);
        xcb_flush(conn);
    }

    if (replyProp) std::free(replyProp);
    if (replyType) std::free(replyType);
#endif
}

void X11WindowHelper::setClickThrough([[maybe_unused]] QWindow* window, [[maybe_unused]] bool clickThrough) noexcept {
#if defined(Q_OS_LINUX)
    if (!window || !qApp) return;

    auto* x11App = qApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!x11App) return;

    xcb_connection_t* conn = x11App->connection();
    if (!conn) return;

    const xcb_window_t winId = static_cast<xcb_window_t>(window->winId());
    if (winId == 0) return;

    if (clickThrough) {
        // Set empty input shape region -> mouse clicks pass through window to desktop/wallpaper
        xcb_shape_rectangles(conn, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_INPUT, XCB_CLIP_ORDERING_UNSORTED, winId, 0, 0, 0, nullptr);
    } else {
        // Restore input shape mask
        xcb_shape_mask(conn, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_INPUT, winId, 0, 0, XCB_NONE);
    }
    xcb_flush(conn);
#endif
}

} // namespace gs::platform
