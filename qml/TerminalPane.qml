import QtQuick
import TerminalCpp
import TerminalCpp.Native

FocusScope {
    id: root
    focus: true
    property bool copyNotice: false

    Timer {
        id: copyNoticeTimer
        interval: 900
        repeat: false
        onTriggered: root.copyNotice = false
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.background

        TerminalView {
            id: terminalView
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            anchors.topMargin: 10
            anchors.bottomMargin: 10
            session: terminalSession
            fontPixelSize: 14
            focus: true

            onSelectionCopied: {
                root.copyNotice = true
                copyNoticeTimer.restart()
            }
        }

        Rectangle {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 12
            anchors.bottomMargin: 8
            width: inputHint.implicitWidth + 14
            height: 22
            color: Theme.panel
            border.width: 1
            border.color: terminalView.activeFocus ? Theme.borderStrong : Theme.border
            radius: Theme.radiusSmall
            opacity: 0.80

            Text {
                id: inputHint
                anchors.centerIn: parent
                text: !terminalSession.running
                      ? "SHELL STOPPED"
                      : root.copyNotice
                        ? "COPIED"
                        : terminalView.hasSelection
                          ? "CTRL+C COPY"
                          : terminalView.scrollbackOffset > 0
                            ? "SCROLLBACK -" + terminalView.scrollbackOffset
                            : terminalView.activeFocus
                              ? terminalSession.columns + "×" + terminalSession.rows
                              : "CLICK TO TYPE"
                color: root.copyNotice || (terminalView.activeFocus && terminalSession.running)
                       ? Theme.accent
                       : Theme.textFaint
                font.pixelSize: 9
                font.letterSpacing: 0.8
            }
        }
    }

    Component.onCompleted: terminalView.forceActiveFocus()
}
