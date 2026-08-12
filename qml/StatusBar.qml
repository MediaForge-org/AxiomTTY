import QtQuick
import QtQuick.Layouts
import AxiomTTY

Rectangle {
    id: root

    required property string shell
    required property bool running

    readonly property string shellName: {
        if (root.shell.length === 0)
            return "shell"
        const parts = root.shell.split("/")
        return parts.length > 0 ? parts[parts.length - 1] : root.shell
    }

    implicitHeight: Theme.statusHeight
    color: Theme.background

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.borderInactive
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 8

        Rectangle {
            Layout.preferredWidth: 5
            Layout.preferredHeight: 5
            radius: 3
            color: root.running ? Theme.success : Theme.textFaint
        }

        Text {
            text: root.running ? root.shellName : "shell stopped"
            color: root.running ? Theme.textMuted : Theme.textFaint
            font.family: "sans-serif"
            font.pixelSize: 10
        }

        Item { Layout.fillWidth: true }

        Text {
            text: "UTF-8"
            color: Theme.textFaint
            font.family: "sans-serif"
            font.pixelSize: 9
            font.letterSpacing: 0.4
        }
    }
}
