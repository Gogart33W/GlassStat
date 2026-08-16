import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import GlassStat 1.0

Window {
    id: settingsWin
    title: "GlassStat — Preferences"
    width: 440
    height: mainCol.implicitHeight + 80
    minimumWidth: 380
    minimumHeight: 420
    modality: Qt.NonModal
    flags: Qt.Dialog

    // ── Snapshot values captured when window opens (for Cancel revert) ────────
    property double snapScale:   1.0
    property double snapOpacity: 0.85
    property string snapMode:    "floating"
    property bool   snapCT:      false
    property string snapAccent:  "#8b5cf6"

    function captureSnapshot() {
        snapScale   = ConfigManager.uiScale
        snapOpacity = ConfigManager.uiOpacity
        snapMode    = ConfigManager.windowMode
        snapCT      = ConfigManager.clickThrough
        snapAccent  = ConfigManager.accentColor
    }

    function revertToSnapshot() {
        ConfigManager.setUiScale(snapScale)
        ConfigManager.setUiOpacity(snapOpacity)
        ConfigManager.setWindowMode(snapMode)
        ConfigManager.setClickThrough(snapCT)
        ConfigManager.setAccentColor(snapAccent)
    }

    Component.onCompleted: captureSnapshot()

    onVisibleChanged: { if (visible) captureSnapshot() }

    // ── Background ────────────────────────────────────────────────────────────
    color: "#0f0f1a"

    // ── Title bar ────────────────────────────────────────────────────────────
    Rectangle {
        id: titleBar
        width: parent.width
        height: 46
        color: "#16162a"
        border.color: Qt.rgba(1, 1, 1, 0.08)
        border.width: 1

        RowLayout {
            anchors { fill: parent; leftMargin: 18; rightMargin: 12 }
            Text {
                text: "⚙  Preferences"
                font { pixelSize: 14; weight: Font.Medium }
                color: "#e2e8f0"
                Layout.fillWidth: true
            }
            Rectangle {
                width: 22; height: 22; radius: 5
                color: closeBtn.containsMouse ? "#e53e3e" : Qt.rgba(1,0,0,0.25)
                Behavior on color { ColorAnimation { duration: 120 } }
                Text { anchors.centerIn: parent; text: "✕"; font.pixelSize: 10; color: "white" }
                MouseArea { id: closeBtn; anchors.fill: parent; hoverEnabled: true; onClicked: settingsWin.close() }
            }
        }
    }

    // ── Scrollable body ──────────────────────────────────────────────────────
    Flickable {
        anchors { top: titleBar.bottom; left: parent.left; right: parent.right; bottom: buttonBar.top }
        contentHeight: mainCol.implicitHeight + 32
        clip: true

        ColumnLayout {
            id: mainCol
            width: parent.width
            anchors.top: parent.top
            anchors.topMargin: 16
            spacing: 0

            // ── Section helper ────────────────────────────────────────────────
            component SLabel: Text {
                font { pixelSize: 10; weight: Font.Bold; letterSpacing: 1.2 }
                color: "#64748b"
                Layout.leftMargin: 18
                Layout.topMargin: 16
                Layout.bottomMargin: 6
            }

            component Divider: Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Qt.rgba(1,1,1,0.06)
                Layout.topMargin: 4
            }

            // ── Window Mode ───────────────────────────────────────────────────
            SLabel { text: "WINDOW MODE" }

            RowLayout {
                Layout.leftMargin: 18; Layout.rightMargin: 18; Layout.fillWidth: true
                spacing: 10

                Repeater {
                    model: [
                        { label: "Desktop Widget", value: "desktop", x11Only: true },
                        { label: "Floating",       value: "floating", x11Only: false },
                        { label: "Always on Top",  value: "top",     x11Only: false }
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        Layout.fillWidth: true
                        height: 48
                        radius: 8
                        color: ConfigManager.windowMode === modelData.value
                               ? Qt.rgba(139/255, 92/255, 246/255, 0.22)
                               : Qt.rgba(1,1,1,0.04)
                        border.color: ConfigManager.windowMode === modelData.value
                                      ? "#8b5cf6" : Qt.rgba(1,1,1,0.10)
                        opacity: (modelData.x11Only && !ConfigManager.isX11) ? 0.4 : 1.0

                        Behavior on color { ColorAnimation { duration: 150 } }
                        Behavior on border.color { ColorAnimation { duration: 150 } }

                        Text {
                            anchors.centerIn: parent
                            text: modelData.label + (modelData.x11Only && !ConfigManager.isX11 ? "\n(X11 only)" : "")
                            font.pixelSize: 10
                            horizontalAlignment: Text.AlignHCenter
                            color: ConfigManager.windowMode === modelData.value ? "#c4b5fd" : "#94a3b8"
                        }

                        MouseArea {
                            anchors.fill: parent
                            enabled: !(modelData.x11Only && !ConfigManager.isX11)
                            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: ConfigManager.setWindowMode(modelData.value)
                        }
                    }
                }
            }

            // Click Through
            RowLayout {
                Layout.leftMargin: 18; Layout.rightMargin: 18
                Layout.topMargin: 10
                CheckBox {
                    id: ctCheck
                    text: "Click Through (Desktop mode only)"
                    checked: ConfigManager.clickThrough
                    enabled: ConfigManager.windowMode === "desktop"
                    onCheckedChanged: ConfigManager.setClickThrough(checked)
                    palette.windowText: "#94a3b8"
                }
            }

            Divider {}

            // ── UI Scale ──────────────────────────────────────────────────────
            SLabel { text: "UI SCALE" }

            RowLayout {
                Layout.leftMargin: 18; Layout.rightMargin: 18; Layout.fillWidth: true
                spacing: 10

                Slider {
                    id: scaleSlider
                    Layout.fillWidth: true
                    from: 0.6; to: 2.5; stepSize: 0.05
                    value: ConfigManager.uiScale
                    onMoved: ConfigManager.setUiScale(value)

                    background: Rectangle {
                        x: scaleSlider.leftPadding
                        y: scaleSlider.topPadding + scaleSlider.availableHeight / 2 - height / 2
                        implicitWidth: 200; implicitHeight: 6; height: 6
                        width: scaleSlider.availableWidth; radius: 3
                        color: Qt.rgba(1,1,1,0.12)
                        Rectangle {
                            width: scaleSlider.visualPosition * parent.width
                            height: parent.height; radius: 3
                            color: "#8b5cf6"
                        }
                    }
                    handle: Rectangle {
                        x: scaleSlider.leftPadding + scaleSlider.visualPosition * (scaleSlider.availableWidth - width)
                        y: scaleSlider.topPadding + scaleSlider.availableHeight / 2 - height / 2
                        implicitWidth: 18; implicitHeight: 18; radius: 9
                        color: "#8b5cf6"
                        border.color: "#c4b5fd"; border.width: 2
                    }
                }
                Text {
                    text: scaleSlider.value.toFixed(2) + "×"
                    font { pixelSize: 12; weight: Font.Bold }
                    color: "#c4b5fd"
                    Layout.minimumWidth: 48
                }
            }

            Divider {}

            // ── Opacity ───────────────────────────────────────────────────────
            SLabel { text: "OPACITY" }

            RowLayout {
                Layout.leftMargin: 18; Layout.rightMargin: 18; Layout.fillWidth: true
                spacing: 10

                Slider {
                    id: opacitySlider
                    Layout.fillWidth: true
                    from: 0.1; to: 1.0; stepSize: 0.01
                    value: ConfigManager.uiOpacity
                    onMoved: ConfigManager.setUiOpacity(value)

                    background: Rectangle {
                        x: opacitySlider.leftPadding
                        y: opacitySlider.topPadding + opacitySlider.availableHeight / 2 - height / 2
                        implicitWidth: 200; implicitHeight: 6; height: 6
                        width: opacitySlider.availableWidth; radius: 3
                        color: Qt.rgba(1,1,1,0.12)
                        Rectangle {
                            width: opacitySlider.visualPosition * parent.width
                            height: parent.height; radius: 3
                            color: "#06b6d4"
                        }
                    }
                    handle: Rectangle {
                        x: opacitySlider.leftPadding + opacitySlider.visualPosition * (opacitySlider.availableWidth - width)
                        y: opacitySlider.topPadding + opacitySlider.availableHeight / 2 - height / 2
                        implicitWidth: 18; implicitHeight: 18; radius: 9
                        color: "#06b6d4"
                        border.color: "#67e8f9"; border.width: 2
                    }
                }
                Text {
                    text: Math.round(opacitySlider.value * 100) + "%"
                    font { pixelSize: 12; weight: Font.Bold }
                    color: "#67e8f9"
                    Layout.minimumWidth: 40
                }
            }

            Divider {}

            // ── Accent Color ──────────────────────────────────────────────────
            SLabel { text: "ACCENT COLOR" }

            ColumnLayout {
                Layout.leftMargin: 18; Layout.rightMargin: 18; Layout.fillWidth: true
                spacing: 10

                // Preset swatches
                RowLayout {
                    spacing: 8
                    Repeater {
                        model: ["#8b5cf6", "#06b6d4", "#10b981", "#f97316", "#ec4899", "#f59e0b"]
                        delegate: Rectangle {
                            required property string modelData
                            width: 32; height: 32; radius: 6
                            color: modelData
                            border.color: ConfigManager.accentColor.toLowerCase() === modelData.toLowerCase()
                                          ? "#ffffff" : "transparent"
                            border.width: 2
                            Behavior on border.color { ColorAnimation { duration: 120 } }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    customColorField.text = modelData
                                    ConfigManager.setAccentColor(modelData)
                                }
                            }
                        }
                    }
                }

                // Custom hex field
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Rectangle {
                        width: 22; height: 22; radius: 4
                        color: Qt.color(ConfigManager.accentColor)
                        border.color: Qt.rgba(1,1,1,0.3); border.width: 1
                    }
                    TextField {
                        id: customColorField
                        Layout.fillWidth: true
                        text: ConfigManager.accentColor
                        placeholderText: "#rrggbb"
                        font { family: "monospace"; pixelSize: 12 }
                        color: "#e2e8f0"
                        background: Rectangle {
                            color: Qt.rgba(1,1,1,0.06)
                            radius: 6
                            border.color: customColorField.activeFocus ? "#8b5cf6" : Qt.rgba(1,1,1,0.12)
                            border.width: 1
                        }
                        onEditingFinished: {
                            const t = text.trim()
                            if (/^#[0-9a-fA-F]{6}$/.test(t)) ConfigManager.setAccentColor(t)
                            else text = ConfigManager.accentColor
                        }
                    }
                }
            }

            Divider {}

            // ── Autostart ─────────────────────────────────────────────────────
            SLabel { text: "SYSTEM" }

            RowLayout {
                Layout.leftMargin: 18; Layout.rightMargin: 18
                Layout.bottomMargin: 8
                CheckBox {
                    text: "Launch at login (autostart)"
                    checked: ConfigManager.autostartEnabled
                    onCheckedChanged: ConfigManager.setAutostartEnabled(checked)
                    palette.windowText: "#94a3b8"
                }
            }

            Item { implicitHeight: 8 }
        }
    }

    // ── Button bar ────────────────────────────────────────────────────────────
    Rectangle {
        id: buttonBar
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 58
        color: "#16162a"
        border.color: Qt.rgba(1,1,1,0.08)
        border.width: 1

        RowLayout {
            anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
            spacing: 10

            Item { Layout.fillWidth: true }

            // Cancel
            Rectangle {
                width: 90; height: 34; radius: 7
                color: cancelHov.containsMouse ? Qt.rgba(1,1,1,0.10) : Qt.rgba(1,1,1,0.05)
                border.color: Qt.rgba(1,1,1,0.15); border.width: 1
                Behavior on color { ColorAnimation { duration: 120 } }
                Text { anchors.centerIn: parent; text: "Cancel"; font.pixelSize: 13; color: "#94a3b8" }
                MouseArea {
                    id: cancelHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { settingsWin.revertToSnapshot(); settingsWin.close() }
                }
            }

            // Save
            Rectangle {
                width: 90; height: 34; radius: 7
                color: saveHov.containsMouse ? "#7c3aed" : "#8b5cf6"
                Behavior on color { ColorAnimation { duration: 120 } }
                Text { anchors.centerIn: parent; text: "Save"; font.pixelSize: 13; color: "white"; font.weight: Font.Medium }
                MouseArea {
                    id: saveHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        ConfigManager.setAndPersist("ui", "scale",       scaleSlider.value.toFixed(2))
                        ConfigManager.setAndPersist("ui", "opacity",     opacitySlider.value.toFixed(2))
                        ConfigManager.setAndPersist("colors", "accent",  ConfigManager.accentColor)
                        ConfigManager.setAndPersist("window", "mode",    ConfigManager.windowMode)
                        ConfigManager.setAndPersist("window", "click_through",
                                                    ConfigManager.clickThrough ? "true" : "false")
                        settingsWin.captureSnapshot()
                        settingsWin.close()
                    }
                }
            }
        }
    }
}
