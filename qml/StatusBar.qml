import QtQuick
import QtQuick.Layouts
import TerminalCpp

Rectangle {
    id: root
    required property string shell
    required property int processId
    required property bool running

    implicitHeight: 28
    color: Theme.panel

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 10

        Text {
            text: root.running ? "RUNNING" : "STOPPED"
            color: root.running ? Theme.success : Theme.textMuted
            font.pixelSize: 9
            font.letterSpacing: 1.0
        }

        Rectangle { width: 1; height: 12; color: Theme.border }

        Text {
            text: root.shell.length > 0 ? root.shell : "shell"
            color: Theme.textMuted
            font.pixelSize: 11
            elide: Text.ElideMiddle
        }

        Item { Layout.fillWidth: true }

        Text {
            text: root.processId > 0 ? "PID " + root.processId : "NO PROCESS"
            color: Theme.textFaint
            font.pixelSize: 10
        }

        Rectangle { width: 1; height: 12; color: Theme.border }

        Text {
            text: "UTF-8"
            color: Theme.textFaint
            font.pixelSize: 10
        }
    }
}
