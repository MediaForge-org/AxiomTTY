import QtQuick
import QtQuick.Controls
import AxiomTTY
import AxiomTTY.Native

FocusScope {
    id: root
    focus: true

    required property var session
    property bool paneActive: true

    property bool copyNotice: false
    property bool resizeNotice: false

    signal activated()
    signal closeRequested(var session)

    function focusTerminal() {
        terminalView.forceActiveFocus()
    }

    function syncSessionAppearance() {
        terminalView.colorScheme = root.session ? root.session.colorScheme : "Axiom Dark"
    }

    onSessionChanged: {
        syncSessionAppearance()
        if (paneActive)
            Qt.callLater(function() { terminalView.forceActiveFocus() })
    }
    onPaneActiveChanged: {
        if (paneActive)
            Qt.callLater(function() { terminalView.forceActiveFocus() })
    }

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
        target: root.session
        function onTerminalSizeChanged() {
            root.resizeNotice = true
            resizeNoticeTimer.restart()
        }
        function onProfileChanged() {
            root.syncSessionAppearance()
        }
    }

    HoverHandler {
        id: paneHover
    }

    Rectangle {
        anchors.fill: parent
        color: terminalView.terminalBackground

        TerminalView {
            id: terminalView
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.topMargin: 12
            anchors.bottomMargin: 12
            session: root.session
            fontFamily: appSettings.terminalFontFamily
            fontPixelSize: appSettings.terminalFontSize
            colorScheme: "Axiom Dark"
            focus: true

            onActiveFocusChanged: {
                if (activeFocus)
                    root.activated()
            }

            onSelectionCopied: {
                root.copyNotice = true
                copyNoticeTimer.restart()
            }

            onSearchRequested: searchBar.focusInput()
        }

        SearchBar {
            id: searchBar
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 10
            anchors.rightMargin: 12
            terminalView: terminalView
            visible: terminalView.searchActive
            z: 40
        }

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.width: root.paneActive ? 2 : 0
            border.color: root.paneActive ? Theme.accentMuted : "transparent"
            visible: root.paneActive
        }

        Rectangle {
            width: 3
            height: 26
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 10
            color: root.paneActive ? Theme.accent : "transparent"
            visible: root.paneActive
        }


        Rectangle {
            id: paneCloseButton
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 7
            anchors.rightMargin: 7
            width: 22
            height: 22
            radius: Theme.radiusSmall
            color: closeMouse.containsMouse ? Theme.hover : Theme.panel
            border.width: 1
            border.color: closeMouse.containsMouse ? Theme.borderStrong : Theme.border
            visible: root.paneActive || paneHover.hovered
            opacity: visible ? 0.92 : 0.0
            z: 60

            Behavior on opacity { NumberAnimation { duration: 80 } }

            Text {
                anchors.centerIn: parent
                text: "×"
                color: closeMouse.containsMouse ? Theme.danger : Theme.textMuted
                font.family: "sans-serif"
                font.pixelSize: 14
            }

            MouseArea {
                id: closeMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onPressed: function(mouse) {
                    mouse.accepted = true
                    root.activated()
                }
                onClicked: function(mouse) {
                    mouse.accepted = true
                    root.closeRequested(root.session)
                }
            }

            ToolTip.visible: closeMouse.containsMouse
            ToolTip.text: "Close pane  Ctrl+Shift+X"
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
                text: !root.session
                      ? "NO SESSION"
                      : !root.session.running
                        ? "SHELL STOPPED"
                        : root.copyNotice
                          ? "COPIED"
                          : terminalView.hasSelection
                            ? "CTRL+C  COPY"
                            : terminalView.scrollbackOffset > 0
                              ? "SCROLLBACK  -" + terminalView.scrollbackOffset
                              : root.resizeNotice
                                ? root.session.columns + " × " + root.session.rows
                                : ""
                color: root.copyNotice ? Theme.accent : Theme.textFaint
                font.family: "sans-serif"
                font.pixelSize: 9
                font.weight: Font.Medium
                font.letterSpacing: 0.45
            }
        }
    }

    Component.onCompleted: {
        syncSessionAppearance()
        if (paneActive)
            terminalView.forceActiveFocus()
    }
}
