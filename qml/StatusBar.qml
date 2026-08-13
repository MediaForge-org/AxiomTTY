import QtQuick
import QtQuick.Layouts
import AxiomTTY

Rectangle {
    id: root

    required property var session
    required property int tabCount
    required property int paneCount

    readonly property string shellName: {
        if (!root.session || root.session.shell.length === 0)
            return "shell"
        const parts = root.session.shell.split("/")
        return parts.length > 0 ? parts[parts.length - 1] : root.session.shell
    }

    readonly property string directoryLabel: {
        if (!root.session)
            return ""
        return root.session.workingDirectory
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
            color: root.session && root.session.running ? Theme.success : Theme.textFaint
        }

        Text {
            text: root.session && root.session.running ? root.shellName : "shell stopped"
            color: root.session && root.session.running ? Theme.textMuted : Theme.textFaint
            font.family: "sans-serif"
            font.pixelSize: 10
        }

        Text {
            text: root.directoryLabel
            visible: text.length > 0
            color: Theme.textFaint
            elide: Text.ElideMiddle
            Layout.maximumWidth: 420
            font.family: "sans-serif"
            font.pixelSize: 9
        }

        Item { Layout.fillWidth: true }

        Text {
            visible: root.paneCount > 1
            text: root.paneCount + (root.paneCount === 1 ? " PANE" : " PANES")
            color: Theme.accent
            font.family: "sans-serif"
            font.pixelSize: 9
            font.letterSpacing: 0.35
        }

        Rectangle {
            visible: root.paneCount > 1
            Layout.preferredWidth: 1
            Layout.preferredHeight: 10
            color: Theme.border
        }

        Text {
            text: root.tabCount + (root.tabCount === 1 ? " TAB" : " TABS")
            color: Theme.textFaint
            font.family: "sans-serif"
            font.pixelSize: 9
            font.letterSpacing: 0.35
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 10
            color: Theme.border
        }

        Text {
            text: "UTF-8"
            color: Theme.textFaint
            font.family: "sans-serif"
            font.pixelSize: 9
            font.letterSpacing: 0.4
        }
    }
}
