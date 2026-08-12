import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import TerminalCpp

Rectangle {
    id: root
    required property string title
    required property bool running

    implicitHeight: 44
    color: Theme.panel

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 6

        ToolButton {
            id: addButton
            text: "+"
            enabled: false
            flat: true
            ToolTip.visible: hovered
            ToolTip.text: "Multiple sessions come in milestone 2"

            contentItem: Text {
                text: addButton.text
                color: addButton.enabled ? Theme.text : Theme.textFaint
                font.pixelSize: 20
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: addButton.hovered ? Theme.raised : "transparent"
                radius: Theme.radiusSmall
            }
        }

        Rectangle {
            Layout.preferredWidth: 210
            Layout.fillHeight: true
            color: Theme.raised

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 2
                color: Theme.accent
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 13
                anchors.rightMargin: 11
                spacing: 9

                Rectangle {
                    width: 7
                    height: 7
                    radius: 4
                    color: root.running ? Theme.success : Theme.textFaint
                }

                Text {
                    Layout.fillWidth: true
                    text: root.title
                    color: Theme.text
                    elide: Text.ElideRight
                    font.pixelSize: 13
                }

                Text {
                    text: "×"
                    color: Theme.textFaint
                    font.pixelSize: 15
                }
            }
        }

        Item { Layout.fillWidth: true }

        Text {
            text: "LINUX"
            color: Theme.textFaint
            font.pixelSize: 10
            font.letterSpacing: 1.4
        }
    }
}
