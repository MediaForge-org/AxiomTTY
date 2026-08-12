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

    font.family: "sans-serif"
    font.pixelSize: Theme.uiFontSize

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TopBar {
            Layout.fillWidth: true
            title: terminalSession.title
            running: terminalSession.running
            appWindow: root
        }

        TerminalPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        StatusBar {
            Layout.fillWidth: true
            shell: terminalSession.shell
            running: terminalSession.running
        }
    }

    // One restrained 1px client border gives the frameless window a clean edge
    // without turning the UI into a card or adding decorative shadows.
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: root.visibility === Window.Maximized ? 0 : 1
        border.color: root.active ? Theme.borderStrong : Theme.borderInactive
        z: 900
    }

    // Frameless windows need resize hit zones. startSystemResize() delegates the
    // resize to the compositor/window manager and works correctly on Wayland.
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

    // Corners sit above edge zones so diagonal resize wins in the overlap.
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

    Shortcut {
        sequence: "Ctrl+Shift+L"
        onActivated: terminalSession.clearDisplay()
    }

    Shortcut {
        sequence: "Ctrl+Shift+R"
        onActivated: {
            if (!terminalSession.running)
                terminalSession.startDefaultShell()
        }
    }
}
