import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
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
    signal duplicateRequested()
    signal renameRequested(string newTitle)
    signal resetTitleRequested()

    implicitWidth: 190
    implicitHeight: 32

    function beginRename() {
        renameField.text = root.title
        renamePopup.open()
        Qt.callLater(function() {
            renameField.forceActiveFocus()
            renameField.selectAll()
        })
    }

    function finishRename() {
        const value = renameField.text.trim()
        if (value.length > 0)
            root.renameRequested(value)
        else
            root.resetTitleRequested()
        renamePopup.close()
        root.selected()
    }

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
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton

        onClicked: function(mouse) {
            if (mouse.button === Qt.MiddleButton) {
                root.closeRequested()
            } else if (mouse.button === Qt.RightButton) {
                root.selected()
                tabMenu.popup()
            } else {
                root.selected()
            }
        }

        onDoubleClicked: function(mouse) {
            if (mouse.button === Qt.LeftButton) {
                root.selected()
                root.beginRename()
                mouse.accepted = true
            }
        }
    }

    Menu {
        id: tabMenu

        MenuItem {
            text: "Duplicate Tab"
            onTriggered: root.duplicateRequested()
        }
        MenuItem {
            text: "Rename Tab"
            onTriggered: Qt.callLater(root.beginRename)
        }
        MenuItem {
            text: "Use Automatic Title"
            onTriggered: root.resetTitleRequested()
        }
        MenuSeparator {}
        MenuItem {
            text: "Close Tab"
            onTriggered: root.closeRequested()
        }
    }

    Popup {
        id: renamePopup
        width: Math.max(220, root.width)
        height: 42
        x: 0
        y: root.height + 4
        padding: 5
        focus: true
        modal: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: Theme.raised
            border.width: 1
            border.color: Theme.borderStrong
            radius: Theme.radiusSmall
        }

        contentItem: TextField {
            id: renameField
            placeholderText: "Tab name"
            color: Theme.text
            selectionColor: Theme.accentMuted
            selectedTextColor: Theme.text
            font.family: "sans-serif"
            font.pixelSize: 11
            selectByMouse: true
            background: Rectangle {
                color: Theme.background
                border.width: 1
                border.color: renameField.activeFocus ? Theme.accent : Theme.border
                radius: Theme.radiusSmall
            }

            onAccepted: root.finishRename()
        }

        onOpened: Qt.callLater(function() {
            renameField.forceActiveFocus()
            renameField.selectAll()
        })
    }
}
