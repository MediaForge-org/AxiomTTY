import QtQuick
import AxiomTTY

Item {
    id: root

    required property string kind
    property bool dangerous: false
    property bool active: true
    signal clicked()

    implicitWidth: 44
    implicitHeight: Theme.headerHeight

    Rectangle {
        anchors.fill: parent
        color: mouseArea.containsMouse
               ? (root.dangerous ? Theme.closeHover : Theme.hover)
               : "transparent"
    }

    Item {
        id: icon
        width: 14
        height: 14
        anchors.centerIn: parent
        opacity: root.active ? 1.0 : 0.45

        // Minimize
        Rectangle {
            visible: root.kind === "minimize"
            width: 10
            height: 1
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 3
            color: mouseArea.containsMouse && root.dangerous ? "white" : Theme.textMuted
        }

        // Maximize
        Rectangle {
            visible: root.kind === "maximize"
            width: 10
            height: 8
            anchors.centerIn: parent
            color: "transparent"
            border.width: 1
            border.color: Theme.textMuted
        }

        // Restore
        Rectangle {
            visible: root.kind === "restore"
            width: 8
            height: 7
            x: 4
            y: 2
            color: "transparent"
            border.width: 1
            border.color: Theme.textMuted
        }
        Rectangle {
            visible: root.kind === "restore"
            width: 8
            height: 7
            x: 2
            y: 5
            color: "transparent"
            border.width: 1
            border.color: Theme.textMuted
        }

        // Close
        Rectangle {
            visible: root.kind === "close"
            width: 11
            height: 1
            anchors.centerIn: parent
            rotation: 45
            color: mouseArea.containsMouse ? "white" : Theme.textMuted
            antialiasing: true
        }
        Rectangle {
            visible: root.kind === "close"
            width: 11
            height: 1
            anchors.centerIn: parent
            rotation: -45
            color: mouseArea.containsMouse ? "white" : Theme.textMuted
            antialiasing: true
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.PointingHandCursor
        enabled: root.active
        onClicked: root.clicked()
    }
}
