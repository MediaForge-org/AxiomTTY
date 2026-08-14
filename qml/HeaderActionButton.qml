import QtQuick
import QtQuick.Controls
import AxiomTTY

Item {
    id: root

    property string label: ""
    property string toolTipText: ""
    signal clicked()

    implicitWidth: 36
    implicitHeight: 32

    Rectangle {
        anchors.centerIn: parent
        width: 28
        height: 28
        radius: Theme.radiusSmall
        color: mouse.containsMouse ? Theme.hover : "transparent"
    }

    Text {
        anchors.centerIn: parent
        text: root.label
        color: mouse.containsMouse ? Theme.text : Theme.textMuted
        font.family: "sans-serif"
        font.pixelSize: 15
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    ToolTip.visible: mouse.containsMouse && root.toolTipText.length > 0
    ToolTip.text: root.toolTipText
    ToolTip.delay: 550
}
