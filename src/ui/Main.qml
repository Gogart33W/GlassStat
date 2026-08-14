import QtQuick
import QtQuick.Layouts
import QtCore
import GlassStat 1.0

Window {
    id: root

    readonly property int padH: 20
    readonly property int padV: 16

    // State for collapsible sections & compact mode
    property bool miniMode:        false
    property bool cpuExpanded:     true
    property bool ramExpanded:     true
    property bool netExpanded:     true
    property bool thermalExpanded: true
    property bool gpuExpanded:     true
    property bool scriptsExpanded: true

    width:   300
    height:  padV + colMain.implicitHeight
    flags:   Qt.FramelessWindowHint | Qt.WindowStaysOnBottomHint | Qt.Tool
    color:   "transparent"
    title:   "GlassStat"
    visible: TrayController.isWidgetVisible

    Settings {
        category: "window"
        property alias winX: root.x
        property alias winY: root.y
    }

    // ── 1. DRAG & DROP (Native Window Move) ───────────────────────────────────
    MouseArea {
        anchors.fill: parent
        enabled:      !TrayController.isLocked
        onPressed:    root.startSystemMove()
    }

    // ── glassmorphic panel — colors driven by ConfigManager ───────────────────
    Rectangle {
        anchors.fill: parent
        radius:       16
        color: {
            const c = Qt.color(ConfigManager.bgColor)
            return Qt.rgba(c.r, c.g, c.b, ConfigManager.uiOpacity)
        }
        border.color: {
            const c = Qt.color(ConfigManager.accentColor)
            return Qt.rgba(c.r, c.g, c.b, 0.28)
        }
        border.width: 1

        // top inner glass shimmer
        Rectangle {
            x: 10; y: 1; height: 1
            width:  parent.width - 20
            radius: 1
            color:  Qt.rgba(1, 1, 1, 0.10)
        }

        ColumnLayout {
            id: colMain
            anchors { top: parent.top; left: parent.left; right: parent.right }
            anchors.topMargin:   root.padV
            anchors.leftMargin:  root.padH
            anchors.rightMargin: root.padH
            spacing: 12

            // ── HEADER ────────────────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Rectangle {
                    width: 6; height: 6; radius: 3
                    color: ConfigManager.accentColor
                    SequentialAnimation on opacity {
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.25; duration: 1100; easing.type: Easing.InOutQuad }
                        NumberAnimation { to: 1.0;  duration: 1100; easing.type: Easing.InOutQuad }
                    }
                }
                Text {
                    text: "GlassStat"
                    leftPadding: 4
                    font {
                        family:        ConfigManager.fontFamily + ", Fira Mono, monospace"
                        pixelSize:     12
                        weight:        Font.Bold
                        letterSpacing: 1.8
                    }
                    color: ConfigManager.accentColor
                }
                Item { Layout.fillWidth: true }

                // Mini Mode Toggle Button
                Rectangle {
                    width: 16; height: 16; radius: 4
                    color: miniMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(1, 1, 1, 0.06)
                    Behavior on color { ColorAnimation { duration: 150 } }

                    Text {
                        anchors.centerIn: parent
                        text: root.miniMode ? "⤢" : "─"
                        font { family: "monospace"; pixelSize: 10; weight: Font.Bold }
                        color: root.miniMode ? ConfigManager.accentColor : "#94a3b8"
                    }
                    MouseArea {
                        id: miniMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape:  Qt.PointingHandCursor
                        onClicked:    root.miniMode = !root.miniMode
                    }
                }

                // Close Button
                Rectangle {
                    width: 16; height: 16; radius: 4
                    color: closeMouse.containsMouse ? "#ff4444" : Qt.rgba(1, 0.33, 0.33, 0.35)
                    Behavior on color { ColorAnimation { duration: 150 } }

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        font { family: "monospace"; pixelSize: 9; weight: Font.Bold }
                        color: closeMouse.containsMouse ? "#ffffff" : "#fca5a5"
                    }
                    MouseArea {
                        id: closeMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape:  Qt.PointingHandCursor
                        onClicked:    Qt.quit()
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Qt.rgba(1,1,1,0.06) }

            // ── CPU SECTION ───────────────────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                MetricRow {
                    label:       "CPU"
                    value:       SystemMonitor.cpuTotal
                    detail:      SystemMonitor.cpuTemperature > 0
                                 ? "(" + SystemMonitor.cpuTemperature.toFixed(0) + "°C)"
                                 : ""
                    detailColor: SystemMonitor.cpuTemperature > 85 ? "#f87171" : "#475569"
                    barClr:      ConfigManager.accentColor
                    collapsible: true
                    expanded:    root.cpuExpanded && !root.miniMode
                    onToggled:   root.cpuExpanded = !root.cpuExpanded
                    Layout.fillWidth: true
                }

                SparklineCanvas {
                    Layout.fillWidth: true
                    implicitHeight:   22
                    visible:          root.cpuExpanded && !root.miniMode
                    historyData:      SystemMonitor.cpuHistory
                    lineColor:        ConfigManager.accentColor
                }

                GridLayout {
                    visible: root.cpuExpanded && !root.miniMode
                    Layout.fillWidth: true
                    columns: 2; columnSpacing: 10; rowSpacing: 6

                    Repeater {
                        model: SystemMonitor.cpuCores
                        delegate: RowLayout {
                            required property var modelData
                            required property int index
                            spacing: 5
                            Layout.fillWidth: true
                            Text {
                                text: "c" + index
                                font { family: "monospace"; pixelSize: 9 }
                                color: "#4a5568"
                                Layout.minimumWidth: 16
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                height: 5; radius: 3
                                color:  Qt.rgba(1, 1, 1, 0.07)
                                Rectangle {
                                    width: parent.width * Math.min(modelData, 100.0) / 100.0
                                    height: parent.height; radius: parent.radius
                                    color: modelData < 50 ? "#5b21b6" : modelData < 80 ? "#b45309" : "#991b1b"
                                    Behavior on width { NumberAnimation { duration: 500; easing.type: Easing.OutCubic } }
                                    Behavior on color { ColorAnimation { duration: 300 } }
                                    Rectangle {
                                        anchors { top: parent.top; left: parent.left; right: parent.right }
                                        height: 2; radius: parent.radius
                                        color: Qt.rgba(1, 1, 1, 0.22)
                                    }
                                }
                            }
                            Text {
                                text: modelData.toFixed(0) + "%"
                                font { family: "monospace"; pixelSize: 9 }
                                color: "#64748b"
                                horizontalAlignment: Text.AlignRight
                                Layout.minimumWidth: 28
                            }
                        }
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Qt.rgba(1,1,1,0.06) }

            // ── RAM / SWAP SECTION ────────────────────────────────────────────
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                MetricRow {
                    label:     "RAM"
                    value:     SystemMonitor.ramPercent
                    detail:    formatGiB(SystemMonitor.ramUsedMiB) + " / " + formatGiB(SystemMonitor.ramTotalMiB)
                    barClr:    "#06b6d4"
                    collapsible: true
                    expanded:  root.ramExpanded && !root.miniMode
                    onToggled: root.ramExpanded = !root.ramExpanded
                    Layout.fillWidth: true
                }

                SparklineCanvas {
                    Layout.fillWidth: true
                    implicitHeight:   22
                    visible:          root.ramExpanded && !root.miniMode
                    historyData:      SystemMonitor.ramHistory
                    lineColor:        "#06b6d4"
                }

                MetricRow {
                    visible:   root.ramExpanded && !root.miniMode
                    label:     "Swap"
                    value:     SystemMonitor.swapPercent
                    barClr:    "#f59e0b"
                    Layout.fillWidth: true
                }
            }

            // ── NETWORK SECTION ───────────────────────────────────────────────
            ColumnLayout {
                visible: activeNetworkIfaces().length > 0
                Layout.fillWidth: true
                spacing: 8

                Rectangle { Layout.fillWidth: true; height: 1; color: Qt.rgba(1,1,1,0.06) }

                SectionHeader {
                    title:     "NET"
                    expanded:  root.netExpanded && !root.miniMode
                    onToggled: root.netExpanded = !root.netExpanded
                }

                ColumnLayout {
                    visible: root.netExpanded && !root.miniMode
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: activeNetworkIfaces()
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 6
                            Text {
                                text: modelData.name
                                font { family: "monospace"; pixelSize: 10; weight: Font.Bold }
                                color: "#cbd5e1"
                                Layout.minimumWidth: 52
                            }
                            Text {
                                text: "↓ " + formatSpeed(modelData.rx)
                                font { family: "monospace"; pixelSize: 9 }
                                color: "#34d399"
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: "↑ " + formatSpeed(modelData.tx)
                                font { family: "monospace"; pixelSize: 9 }
                                color: "#60a5fa"
                            }
                        }
                    }
                }
            }

            // ── THERMAL SECTION (Compact 2-Column Grid) ───────────────────────
            ColumnLayout {
                visible: SystemMonitor.thermalSensors.length > 0
                Layout.fillWidth: true
                spacing: 8

                Rectangle { Layout.fillWidth: true; height: 1; color: Qt.rgba(1,1,1,0.06) }

                SectionHeader {
                    title:     "TEMP"
                    expanded:  root.thermalExpanded && !root.miniMode
                    onToggled: root.thermalExpanded = !root.thermalExpanded
                }

                GridLayout {
                    visible: root.thermalExpanded && !root.miniMode
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 12
                    rowSpacing: 6

                    Repeater {
                        model: SystemMonitor.thermalSensors
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: modelData.name
                                font { family: "monospace"; pixelSize: 9 }
                                color: "#94a3b8"
                                Layout.maximumWidth: 64
                                elide: Text.ElideRight
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: modelData.temp.toFixed(0) + "°C"
                                font { family: "monospace"; pixelSize: 10; weight: Font.Bold }
                                color: modelData.temp < 60 ? "#34d399" : modelData.temp < 80 ? "#fbbf24" : "#f87171"
                            }
                        }
                    }
                }
            }

            // ── GPU SECTION (Conditional) ─────────────────────────────────────
            ColumnLayout {
                visible: SystemMonitor.hasGpu
                Layout.fillWidth: true
                spacing: 8

                Rectangle { Layout.fillWidth: true; height: 1; color: Qt.rgba(1,1,1,0.06) }

                MetricRow {
                    label:     SystemMonitor.gpuVendor + " GPU"
                    value:     Math.max(0, SystemMonitor.gpuUsage)
                    barClr:    ConfigManager.accentColor
                    collapsible: true
                    expanded:  root.gpuExpanded && !root.miniMode
                    onToggled: root.gpuExpanded = !root.gpuExpanded
                    Layout.fillWidth: true
                }

                ColumnLayout {
                    visible: root.gpuExpanded && !root.miniMode
                    Layout.fillWidth: true
                    spacing: 6

                    MetricRow {
                        visible: SystemMonitor.gpuVramTotal > 0
                        label:   "VRAM"
                        value:   SystemMonitor.gpuVramTotal > 0
                                 ? SystemMonitor.gpuVramUsed / SystemMonitor.gpuVramTotal * 100
                                 : 0
                        detail:  formatGiB(SystemMonitor.gpuVramUsed) + " / " + formatGiB(SystemMonitor.gpuVramTotal)
                        barClr:  "#ec4899"
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        visible: SystemMonitor.gpuTemp > 0
                        Layout.fillWidth: true
                        Text {
                            text: "GPU Temp"
                            font { family: "monospace"; pixelSize: 10 }
                            color: "#94a3b8"
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: SystemMonitor.gpuTemp.toFixed(1) + "°C"
                            font { family: "monospace"; pixelSize: 11; weight: Font.Bold }
                            color: SystemMonitor.gpuTemp < 60 ? "#34d399"
                                 : SystemMonitor.gpuTemp < 80 ? "#fbbf24" : "#f87171"
                            Behavior on color { ColorAnimation { duration: 300 } }
                        }
                    }
                }
            }

            // ── SCRIPTS SECTION ───────────────────────────────────────────────
            ColumnLayout {
                visible: ScriptRunner.results.length > 0
                Layout.fillWidth: true
                spacing: 8

                Rectangle { Layout.fillWidth: true; height: 1; color: Qt.rgba(1,1,1,0.06) }

                SectionHeader {
                    title:     "SCRIPTS"
                    expanded:  root.scriptsExpanded && !root.miniMode
                    onToggled: root.scriptsExpanded = !root.scriptsExpanded
                }

                ColumnLayout {
                    visible: root.scriptsExpanded && !root.miniMode
                    Layout.fillWidth: true
                    spacing: 6

                    Repeater {
                        model: ScriptRunner.results
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 6
                            Text {
                                text: modelData.name
                                font { family: "monospace"; pixelSize: 10; weight: Font.Bold }
                                color: ConfigManager.textColor
                                Layout.minimumWidth: 56
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text:  modelData.output
                                font { family: "monospace"; pixelSize: 9 }
                                color: modelData.exitCode === 0 ? "#34d399" : "#f87171"
                                elide: Text.ElideLeft
                                Layout.maximumWidth: 160
                            }
                        }
                    }
                }
            }

            Item { implicitHeight: root.padV }
        }
    }

    // ── HELPERS ───────────────────────────────────────────────────────────────
    function formatGiB(mib) {
        if (mib <= 0) return "0 MiB"
        return mib >= 1024 ? (mib / 1024).toFixed(1) + " GiB" : mib + " MiB"
    }

    function formatSpeed(kbps) {
        if (kbps < 1024)    return kbps.toFixed(1)          + " KB/s"
        if (kbps < 1048576) return (kbps / 1024).toFixed(2) + " MB/s"
        return (kbps / 1048576).toFixed(2) + " GB/s"
    }

    function activeNetworkIfaces() {
        const list = SystemMonitor.networkIfaces
        if (!list || list.length === 0) return []
        const active = list.filter(iface => iface.rx > 0 || iface.tx > 0)
        return active.length > 0 ? active : [list[0]]
    }

    // ── INLINE COMPONENT: SECTION HEADER ──────────────────────────────────────
    component SectionHeader: Item {
        id: hdr
        required property string title
        required property bool   expanded
        signal toggled()

        implicitHeight: rowHdr.implicitHeight
        Layout.fillWidth: true

        RowLayout {
            id: rowHdr
            anchors.fill: parent
            spacing: 6

            Text {
                text: hdr.title
                font {
                    family:    ConfigManager.fontFamily + ", Fira Mono, monospace"
                    pixelSize: 10
                    weight:    Font.Bold
                }
                color: "#94a3b8"
            }

            Item { Layout.fillWidth: true }

            Text {
                text: hdr.expanded ? "▾" : "▸"
                font { family: "monospace"; pixelSize: 10 }
                color: "#64748b"
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape:  Qt.PointingHandCursor
            onClicked:    hdr.toggled()
        }
    }

    // ── INLINE COMPONENT: ANIMATED METRIC ROW ─────────────────────────────────
    component MetricRow: ColumnLayout {
        id: self
        required property string label
        required property real   value
        property  string detail:      ""
        property  color  detailColor: "#475569"
        property  string unit:        "%"
        required property color  barClr
        property  bool   collapsible: false
        property  bool   expanded:    true
        signal toggled()

        spacing: 6

        Item {
            implicitHeight: rowMetric.implicitHeight
            Layout.fillWidth: true

            RowLayout {
                id: rowMetric
                anchors.fill: parent

                Text {
                    text: self.label
                    font {
                        family:    ConfigManager.fontFamily + ", Fira Mono, monospace"
                        pixelSize: 10
                        weight:    Font.Medium
                    }
                    color: "#94a3b8"
                    Layout.minimumWidth: 54
                    elide: Text.ElideRight
                }

                Text {
                    visible: self.collapsible
                    text: self.expanded ? "▾" : "▸"
                    font { family: "monospace"; pixelSize: 9 }
                    color: "#475569"
                }

                Item { Layout.fillWidth: true }

                Text {
                    visible: self.detail !== ""
                    text:    self.detail
                    font { family: "monospace"; pixelSize: 9 }
                    color: self.detailColor
                    rightPadding: 8
                    Behavior on color { ColorAnimation { duration: 300 } }
                }

                Text {
                    text:  self.value.toFixed(1) + self.unit
                    font {
                        family:    ConfigManager.fontFamily + ", Fira Mono, monospace"
                        pixelSize: 11
                        weight:    Font.Bold
                    }
                    color: self.value < 50 ? "#34d399" : self.value < 80 ? "#fbbf24" : "#f87171"
                    Behavior on color { ColorAnimation { duration: 300 } }
                }
            }

            MouseArea {
                anchors.fill: parent
                enabled:     self.collapsible
                cursorShape: self.collapsible ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked:   self.toggled()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 8; radius: 4
            color: Qt.rgba(1, 1, 1, 0.06)

            Rectangle {
                width: parent.width * Math.min(self.value, 100.0) / 100.0
                height: parent.height; radius: parent.radius
                color:  self.barClr
                opacity: 0.82
                Behavior on width { NumberAnimation { duration: 600; easing.type: Easing.OutCubic } }

                Rectangle {
                    anchors { top: parent.top; left: parent.left; right: parent.right }
                    height: 3; radius: parent.radius
                    color:  Qt.rgba(1, 1, 1, 0.22)
                    visible: parent.width > 6
                }
            }
        }
    }
}
