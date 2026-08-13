import QtQuick
import QtQuick.Layouts
import AxiomTTY

Item {
    id: root

    required property string title
    required property bool active
    required property bool running
    required property int tabIndex
    required property int paneCount

    signal selected()
    signal closeRequested()

    implicitWidth: 190
    implicitHeight: 32

    Rectangle {
        anchors.fill: parent
        color: root.active ? Theme.tabActive : (tabMouse.containsMouse ? Theme.hover : "transparent")
        border.width: root.active ? 1 : 0
        border.color: Theme.border
        radius: Theme.radiusSmall
    }

    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: 1
        anchors.verticalCenter: parent.verticalCenter
        width: 2
        height: 18
        color: root.active ? Theme.accent : "transparent"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 11
        anchors.rightMargin: 7
        spacing: 7

        Rectangle {
            Layout.preferredWidth: 5
            Layout.preferredHeight: 5
            radius: 3
            color: root.running ? Theme.success : Theme.textFaint
        }

        Text {
            Layout.fillWidth: true
            text: root.title.length > 0 ? root.title : "Shell"
            color: root.active ? Theme.text : Theme.textMuted
            elide: Text.ElideMiddle
            font.family: "sans-serif"
            font.pixelSize: 11
            font.weight: root.active ? Font.Medium : Font.Normal
        }

        Text {
            visible: root.paneCount > 1
            text: root.paneCount
            color: root.active ? Theme.accent : Theme.textFaint
            font.family: "sans-serif"
            font.pixelSize: 9
            font.weight: Font.Medium
        }

        Item {
            Layout.preferredWidth: 20
            Layout.preferredHeight: 20

            Rectangle {
                anchors.fill: parent
                radius: Theme.radiusSmall
                color: closeMouse.containsMouse ? Theme.hover : "transparent"
            }

            Text {
                anchors.centerIn: parent
                text: "×"
                color: closeMouse.containsMouse ? Theme.text : Theme.textFaint
                font.family: "sans-serif"
                font.pixelSize: 14
            }

            MouseArea {
                id: closeMouse
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                onClicked: root.closeRequested()
            }
        }
    }

    MouseArea {
        id: tabMouse
        anchors.fill: parent
        anchors.rightMargin: 26
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        onClicked: function(mouse) {
            if (mouse.button === Qt.MiddleButton)
                root.closeRequested()
            else
                root.selected()
        }
    }
}
