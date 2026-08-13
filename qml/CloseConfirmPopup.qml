import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AxiomTTY

Dialog {
    id: root

    property string message: "A program is still running. Close anyway?"

    width: 410
    height: 170
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    padding: 0

    onOpened: {
        cancelButton.forceActiveFocus()
    }

    background: Rectangle {
        color: Theme.raised
        border.width: 1
        border.color: Theme.borderStrong
        radius: Theme.radiusMedium
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        anchors.topMargin: 18
        anchors.bottomMargin: 8
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: "Close running session?"
            color: Theme.text
            font.family: "sans-serif"
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        Text {
            Layout.fillWidth: true
            text: root.message
            color: Theme.textMuted
            wrapMode: Text.WordWrap
            font.family: "sans-serif"
            font.pixelSize: 11
        }

        Item { Layout.fillHeight: true }
    }

    footer: DialogButtonBox {
        id: buttonBox
        spacing: 8
        leftPadding: 18
        rightPadding: 18
        topPadding: 8
        bottomPadding: 14

        background: Rectangle {
            color: "transparent"
        }

        Button {
            id: cancelButton
            text: "Cancel"
            focusPolicy: Qt.StrongFocus
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }

        Button {
            id: closeButton
            text: "Close anyway"
            focusPolicy: Qt.StrongFocus
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }

        onAccepted: root.accept()
        onRejected: root.reject()
    }
}
