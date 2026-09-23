import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import AxiomTTY

ApplicationWindow {
    id: root

    width: 1180
    height: 760
    minimumWidth: 760
    minimumHeight: 480
    visible: true
    title: "AxiomTTY"
    color: Theme.background
    flags: Qt.Window | Qt.FramelessWindowHint
    property bool allowWindowClose: false

    onClosing: function(close) {
        if (allowWindowClose)
            return
        close.accepted = false
        sessions.requestCloseApplication()
    }

    font.family: "sans-serif"
    font.pixelSize: Theme.uiFontSize

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TopBar {
            Layout.fillWidth: true
            sessionManager: sessions
            appWindow: root
            settingsObject: appSettings
            onSettingsRequested: settingsDialog.open()
        }

        Item {
            id: terminalArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            function rebuildSplitTree() {
                // Destroy the complete visual split tree before recreating it.
                // Rebuilding only the mutated recursive branch can leave a
                // QQuickPaintedItem texture from a removed pane alive for a
                // frame (or longer on some scene-graph paths). A full rebuild
                // makes pane removal deterministic.
                splitTreeLoader.active = false
                Qt.callLater(function() {
                    splitTreeLoader.active = true
                })
            }

            Loader {
                id: splitTreeLoader
                anchors.fill: parent
                sourceComponent: splitTreeComponent
            }

            Component {
                id: splitTreeComponent
                SplitNodeView {
                    node: sessions.activeRoot
                    sessionManager: sessions
                }
            }

            Connections {
                target: sessions
                function onActivePaneCountChanged() { terminalArea.rebuildSplitTree() }
                function onActiveRootChanged() { terminalArea.rebuildSplitTree() }
            }
        }

        StatusBar {
            Layout.fillWidth: true
            session: sessions.activeSession
            tabCount: sessions.count
            paneCount: sessions.activePaneCount
            activePaneIndex: sessions.activePaneIndex
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: root.visibility === Window.Maximized ? 0 : 1
        border.color: root.active ? Theme.borderStrong : Theme.borderInactive
        z: 900
    }

    MouseArea {
        width: 6; anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
        enabled: root.visibility === Window.Windowed
        cursorShape: Qt.SizeHorCursor
        z: 1000
        onPressed: root.startSystemResize(Qt.LeftEdge)
    }
    MouseArea {
        width: 6; anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
        enabled: root.visibility === Window.Windowed
        cursorShape: Qt.SizeHorCursor
        z: 1000
        onPressed: root.startSystemResize(Qt.RightEdge)
    }
    MouseArea {
        height: 6; anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
        enabled: root.visibility === Window.Windowed
        cursorShape: Qt.SizeVerCursor
        z: 1000
        onPressed: root.startSystemResize(Qt.TopEdge)
    }
    MouseArea {
        height: 6; anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        enabled: root.visibility === Window.Windowed
        cursorShape: Qt.SizeVerCursor
        z: 1000
        onPressed: root.startSystemResize(Qt.BottomEdge)
    }

    MouseArea {
        width: 9; height: 9; anchors.left: parent.left; anchors.top: parent.top
        enabled: root.visibility === Window.Windowed
        cursorShape: Qt.SizeFDiagCursor
        z: 1001
        onPressed: root.startSystemResize(Qt.LeftEdge | Qt.TopEdge)
    }
    MouseArea {
        width: 9; height: 9; anchors.right: parent.right; anchors.top: parent.top
        enabled: root.visibility === Window.Windowed
        cursorShape: Qt.SizeBDiagCursor
        z: 1001
        onPressed: root.startSystemResize(Qt.RightEdge | Qt.TopEdge)
    }
    MouseArea {
        width: 9; height: 9; anchors.left: parent.left; anchors.bottom: parent.bottom
        enabled: root.visibility === Window.Windowed
        cursorShape: Qt.SizeBDiagCursor
        z: 1001
        onPressed: root.startSystemResize(Qt.LeftEdge | Qt.BottomEdge)
    }
    MouseArea {
        width: 9; height: 9; anchors.right: parent.right; anchors.bottom: parent.bottom
        enabled: root.visibility === Window.Windowed
        cursorShape: Qt.SizeFDiagCursor
        z: 1001
        onPressed: root.startSystemResize(Qt.RightEdge | Qt.BottomEdge)
    }


    SettingsDialog {
        id: settingsDialog
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 1900
        settingsObject: appSettings
    }

    CloseConfirmPopup {
        id: closeConfirmPopup
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        z: 2000
        onAccepted: sessions.confirmPendingClose()
        onRejected: sessions.cancelPendingClose()
    }

    Connections {
        target: sessions
        function onCloseConfirmationRequested(message) {
            closeConfirmPopup.message = message
            closeConfirmPopup.open()
        }
        function onApplicationCloseApproved() {
            root.allowWindowClose = true
            Qt.callLater(function() { root.close() })
        }
    }


    Shortcut {
        sequence: "Ctrl+,"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (settingsDialog.visible)
                settingsDialog.close()
            else
                settingsDialog.open()
        }
    }

    Shortcut {
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        enabled: settingsDialog.visible
        onActivated: settingsDialog.close()
    }

    Shortcut {
        sequence: "Ctrl+Shift+T"
        onActivated: sessions.newTab()
    }

    Shortcut {
        sequence: "Ctrl+Shift+W"
        onActivated: sessions.requestCloseTab(sessions.currentIndex)
    }

    Shortcut {
        sequence: "Ctrl+Tab"
        onActivated: sessions.nextTab()
    }

    Shortcut {
        sequence: "Ctrl+Shift+Tab"
        onActivated: sessions.previousTab()
    }

    Shortcut {
        sequence: "Ctrl+PageDown"
        onActivated: sessions.nextTab()
    }

    Shortcut {
        sequence: "Ctrl+PageUp"
        onActivated: sessions.previousTab()
    }

    Shortcut {
        sequence: "Ctrl+Shift+L"
        onActivated: {
            if (sessions.activeSession)
                sessions.activeSession.clearDisplay()
        }
    }

    Shortcut {
        sequence: "Ctrl+Shift+R"
        onActivated: {
            if (sessions.activeSession && !sessions.activeSession.running)
                sessions.activeSession.startDefaultShellInDirectory(sessions.activeSession.workingDirectory)
        }
    }

    Shortcut { sequence: "Ctrl+Shift+D"; onActivated: sessions.splitRight() }
    Shortcut { sequence: "Ctrl+Shift+E"; onActivated: sessions.splitDown() }
    Shortcut { sequence: "Alt+Shift+D"; onActivated: sessions.duplicateActivePaneRight() }
    Shortcut { sequence: "Alt+Shift+E"; onActivated: sessions.duplicateActivePaneDown() }
    Shortcut { sequence: "Ctrl+Shift+X"; onActivated: sessions.requestCloseActivePane() }
    Shortcut { sequence: "Ctrl+Shift+Left"; onActivated: sessions.focusPaneLeft() }
    Shortcut { sequence: "Ctrl+Shift+Right"; onActivated: sessions.focusPaneRight() }
    Shortcut { sequence: "Ctrl+Shift+Up"; onActivated: sessions.focusPaneUp() }
    Shortcut { sequence: "Ctrl+Shift+Down"; onActivated: sessions.focusPaneDown() }

    Shortcut { sequence: "Alt+1"; onActivated: sessions.activateTabNumber(1) }
    Shortcut { sequence: "Alt+2"; onActivated: sessions.activateTabNumber(2) }
    Shortcut { sequence: "Alt+3"; onActivated: sessions.activateTabNumber(3) }
    Shortcut { sequence: "Alt+4"; onActivated: sessions.activateTabNumber(4) }
    Shortcut { sequence: "Alt+5"; onActivated: sessions.activateTabNumber(5) }
    Shortcut { sequence: "Alt+6"; onActivated: sessions.activateTabNumber(6) }
    Shortcut { sequence: "Alt+7"; onActivated: sessions.activateTabNumber(7) }
    Shortcut { sequence: "Alt+8"; onActivated: sessions.activateTabNumber(8) }
    Shortcut { sequence: "Alt+9"; onActivated: sessions.activateTabNumber(9) }
}
