import QtQuick
import AxiomTTY
import AxiomTTY.Native

FocusScope {
    id: root
    focus: true

    property bool copyNotice: false
    property bool resizeNotice: false

    Timer {
        id: copyNoticeTimer
        interval: 850
        repeat: false
        onTriggered: root.copyNotice = false
    }

    Timer {
        id: resizeNoticeTimer
        interval: 700
        repeat: false
        onTriggered: root.resizeNotice = false
    }

    Connections {
        target: terminalSession
        function onTerminalSizeChanged() {
            root.resizeNotice = true
            resizeNoticeTimer.restart()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.background

        TerminalView {
            id: terminalView
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.topMargin: 12
            anchors.bottomMargin: 12
            session: terminalSession
            fontPixelSize: Theme.terminalFontSize
            focus: true

            onSelectionCopied: {
                root.copyNotice = true
                copyNoticeTimer.restart()
            }
        }

        Rectangle {
            id: hintBadge
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 12
            anchors.bottomMargin: 9
            width: hintText.implicitWidth + 14
            height: 21
            visible: hintText.text.length > 0
            opacity: visible ? 0.92 : 0.0
            color: Theme.panel
            border.width: 1
            border.color: Theme.border
            radius: Theme.radiusSmall

            Behavior on opacity {
                NumberAnimation { duration: 90 }
            }

            Text {
                id: hintText
                anchors.centerIn: parent
                text: !terminalSession.running
                      ? "SHELL STOPPED"
                      : root.copyNotice
                        ? "COPIED"
                        : terminalView.hasSelection
                          ? "CTRL+C  COPY"
                          : terminalView.scrollbackOffset > 0
                            ? "SCROLLBACK  -" + terminalView.scrollbackOffset
                            : root.resizeNotice
                              ? terminalSession.columns + " × " + terminalSession.rows
                              : !terminalView.activeFocus
                                ? "CLICK TO TYPE"
                                : ""
                color: root.copyNotice ? Theme.accent : Theme.textFaint
                font.family: "sans-serif"
                font.pixelSize: 9
                font.weight: Font.Medium
                font.letterSpacing: 0.45
            }
        }
    }

    Component.onCompleted: terminalView.forceActiveFocus()
}
