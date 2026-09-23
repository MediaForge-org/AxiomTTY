import QtQuick
import QtQuick.Controls
import AxiomTTY

Item {
    id: root

    required property string kind
    property string toolTipText: ""
    signal clicked()

    implicitWidth: 30
    implicitHeight: 30
    opacity: enabled ? 1.0 : 0.35

    Rectangle {
        anchors.centerIn: parent
        width: 26
        height: 26
        radius: Theme.radiusSmall
        color: mouse.containsMouse ? Theme.hover : "transparent"
    }

    Item {
        anchors.centerIn: parent
        width: 15
        height: 13

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: 1
            border.color: mouse.containsMouse ? Theme.text : Theme.textMuted
            radius: 1
        }

        Rectangle {
            visible: root.kind === "right"
            width: 1
            height: parent.height - 2
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            color: mouse.containsMouse ? Theme.text : Theme.textMuted
        }

        Rectangle {
            visible: root.kind === "down"
            height: 1
            width: parent.width - 2
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            color: mouse.containsMouse ? Theme.text : Theme.textMuted
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true
        onClicked: root.clicked()
    }

    ToolTip.visible: mouse.containsMouse && root.toolTipText.length > 0
    ToolTip.delay: 500
    ToolTip.text: root.toolTipText
}
