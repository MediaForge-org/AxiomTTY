import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import AxiomTTY

Rectangle {
    id: root

    required property var sessionManager
    required property var appWindow
    required property var settingsObject
    signal settingsRequested()

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
            Layout.preferredWidth: 108
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
            id: addButton
            Layout.preferredWidth: 36
            Layout.preferredHeight: 32
            Layout.leftMargin: 6

            Rectangle {
                anchors.centerIn: parent
                width: 28
                height: 28
                radius: Theme.radiusSmall
                color: addMouse.containsMouse ? Theme.hover : "transparent"
            }

            Text {
                anchors.centerIn: parent
                text: "+"
                color: addMouse.containsMouse ? Theme.text : Theme.textMuted
                font.family: "sans-serif"
                font.pixelSize: 17
                font.weight: Font.Light
            }

            MouseArea {
                id: addMouse
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: function(mouse) {
                    if (mouse.button === Qt.RightButton)
                        profileMenu.popup()
                    else
                        root.sessionManager.newTab()
                }
            }

            ToolTip.visible: addMouse.containsMouse
            ToolTip.text: "New tab · Ctrl+Shift+T  ·  Right-click for profile"

            Menu {
                id: profileMenu

                Instantiator {
                    model: root.settingsObject.profileNames
                    delegate: MenuItem {
                        text: modelData
                        onTriggered: root.sessionManager.newTabWithProfile(modelData)
                    }
                    onObjectAdded: function(index, object) { profileMenu.insertItem(index, object) }
                    onObjectRemoved: function(index, object) { profileMenu.removeItem(object) }
                }
            }
        }

        Row {
            Layout.leftMargin: 2
            spacing: 0

            SplitControlButton {
                kind: "right"
                enabled: root.sessionManager.canSplitActivePane
                toolTipText: enabled
                    ? "Split right  ·  Ctrl+Shift+D"
                    : "Pane limit reached (" + root.sessionManager.maxPanesPerTab + ")"
                onClicked: root.sessionManager.splitRight()
            }

            SplitControlButton {
                kind: "down"
                enabled: root.sessionManager.canSplitActivePane
                toolTipText: enabled
                    ? "Split down  ·  Ctrl+Shift+E"
                    : "Pane limit reached (" + root.sessionManager.maxPanesPerTab + ")"
                onClicked: root.sessionManager.splitDown()
            }
        }

        ListView {
            id: tabList
            Layout.preferredWidth: Math.min(contentWidth, Math.max(190, root.width - 108 - 36 - 60 - 180 - 100))
            Layout.preferredHeight: 32
            Layout.leftMargin: 4
            orientation: ListView.Horizontal
            spacing: 4
            clip: true
            model: root.sessionManager
            currentIndex: root.sessionManager.currentIndex
            boundsBehavior: Flickable.StopAtBounds
            interactive: contentWidth > width

            onCurrentIndexChanged: {
                if (currentIndex >= 0)
                    positionViewAtIndex(currentIndex, ListView.Contain)
            }

            delegate: SessionTab {
                required property int index
                required property string displayTitle
                required property bool isRunning

                title: displayTitle
                running: isRunning
                active: index === root.sessionManager.currentIndex
                tabIndex: index

                onSelected: root.sessionManager.activateTab(index)
                onCloseRequested: root.sessionManager.requestCloseTab(index)
                onDuplicateRequested: root.sessionManager.duplicateTab(index)
                onRenameRequested: function(newTitle) { root.sessionManager.renameTab(index, newTitle) }
                onResetTitleRequested: root.sessionManager.resetTabTitle(index)
            }
        }

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

        HeaderActionButton {
            label: "⚙"
            toolTipText: "Settings  ·  Ctrl+,"
            onClicked: root.settingsRequested()
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
