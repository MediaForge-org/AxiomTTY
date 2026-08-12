import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import AxiomTTY

Rectangle {
    id: root

    required property string title
    required property bool running
    required property var appWindow

    implicitHeight: Theme.headerHeight
    color: Theme.panel

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.preferredWidth: 116
            Layout.fillHeight: true

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.verticalCenter: parent.verticalCenter
                spacing: 0

                Text {
                    text: "Axiom"
                    color: Theme.text
                    font.family: "sans-serif"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.15
                }
                Text {
                    text: "TTY"
                    color: Theme.accent
                    font.family: "sans-serif"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    font.letterSpacing: 0.15
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 1
            Layout.preferredHeight: 20
            color: Theme.border
        }

        Item {
            Layout.leftMargin: 8
            Layout.preferredWidth: 270
            Layout.preferredHeight: 32

            Rectangle {
                anchors.fill: parent
                color: Theme.tabActive
                border.width: 1
                border.color: Theme.border
                radius: Theme.radiusSmall
            }

            Rectangle {
                width: 2
                height: 18
                anchors.left: parent.left
                anchors.leftMargin: 1
                anchors.verticalCenter: parent.verticalCenter
                color: root.running ? Theme.accent : Theme.textFaint
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 11
                spacing: 8

                Rectangle {
                    Layout.preferredWidth: 6
                    Layout.preferredHeight: 6
                    radius: 3
                    color: root.running ? Theme.success : Theme.textFaint
                }

                Text {
                    Layout.fillWidth: true
                    text: root.title.length > 0 ? root.title : "Shell"
                    color: Theme.text
                    elide: Text.ElideRight
                    font.family: "sans-serif"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
            }
        }

        // Empty header space doubles as the native drag area. Qt delegates the
        // actual move operation to the compositor, which keeps Wayland snapping.
        Item {
            id: dragRegion
            Layout.fillWidth: true
            Layout.fillHeight: true

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onPressed: root.appWindow.startSystemMove()
                onDoubleClicked: {
                    if (root.appWindow.visibility === Window.Maximized)
                        root.appWindow.showNormal()
                    else
                        root.appWindow.showMaximized()
                }
            }
        }

        Row {
            Layout.fillHeight: true
            spacing: 0

            WindowControlButton {
                kind: "minimize"
                onClicked: root.appWindow.showMinimized()
            }

            WindowControlButton {
                kind: root.appWindow.visibility === Window.Maximized ? "restore" : "maximize"
                onClicked: {
                    if (root.appWindow.visibility === Window.Maximized)
                        root.appWindow.showNormal()
                    else
                        root.appWindow.showMaximized()
                }
            }

            WindowControlButton {
                kind: "close"
                dangerous: true
                onClicked: root.appWindow.close()
            }
        }
    }
}
