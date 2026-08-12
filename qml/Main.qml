import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TerminalCpp

ApplicationWindow {
    id: root
    width: 1180
    height: 760
    minimumWidth: 760
    minimumHeight: 480
    visible: true
    title: "TerminalCpp"
    color: Theme.background

    font.family: "sans-serif"
    font.pixelSize: 14

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TopBar {
            Layout.fillWidth: true
            title: terminalSession.title
            running: terminalSession.running
        }

        TerminalPane {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        StatusBar {
            Layout.fillWidth: true
            shell: terminalSession.shell
            processId: terminalSession.processId
            running: terminalSession.running
        }
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
