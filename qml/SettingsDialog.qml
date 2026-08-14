import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AxiomTTY

Dialog {
    id: root

    required property var settingsObject
    property bool syncing: false
    property bool resetPending: false

    width: 560
    height: 570
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    padding: 0
    title: "Settings"

    function syncFromSettings() {
        syncing = true
        fontFamilyField.text = settingsObject.terminalFontFamily
        fontSizeBox.value = settingsObject.terminalFontSize
        shellField.text = settingsObject.defaultShell
        startDirectoryField.text = settingsObject.startDirectory
        inheritCwd.checked = settingsObject.inheritWorkingDirectory
        confirmClose.checked = settingsObject.confirmCloseRunningProcesses
        syncing = false
    }

    onOpened: {
        resetPending = false
        syncFromSettings()
        fontFamilyField.forceActiveFocus()
    }

    onClosed: resetPending = false

    Connections {
        target: root.settingsObject

        function onDefaultsReset() {
            root.syncFromSettings()
            root.resetPending = false
            resetButton.forceActiveFocus()
        }
    }

    background: Rectangle {
        color: Theme.raised
        border.width: 1
        border.color: Theme.borderStrong
        radius: Theme.radiusMedium
    }

    header: Rectangle {
        implicitHeight: 48
        color: Theme.panel
        radius: Theme.radiusMedium

        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.border
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            text: "AxiomTTY Settings"
            color: Theme.text
            font.family: "sans-serif"
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }
    }

    contentItem: Flickable {
        clip: true
        contentWidth: width
        contentHeight: form.implicitHeight + 36

        ColumnLayout {
            id: form
            width: parent.width
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 18
            spacing: 14

            Item { Layout.preferredHeight: 2 }

            Text {
                text: "TERMINAL"
                color: Theme.textFaint
                font.family: "sans-serif"
                font.pixelSize: 9
                font.weight: Font.DemiBold
                font.letterSpacing: 0.8
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 18
                rowSpacing: 10

                Text { text: "Font family"; color: Theme.textMuted; font.pixelSize: 11 }
                TextField {
                    id: fontFamilyField
                    Layout.fillWidth: true
                    color: Theme.text
                    selectionColor: Theme.accentMuted
                    selectedTextColor: Theme.text
                    placeholderText: "monospace"
                    onEditingFinished: settingsObject.setTerminalFontFamily(text)
                    background: Rectangle {
                        color: Theme.background
                        border.width: 1
                        border.color: fontFamilyField.activeFocus ? Theme.accentMuted : Theme.border
                        radius: Theme.radiusSmall
                    }
                }

                Text { text: "Font size"; color: Theme.textMuted; font.pixelSize: 11 }
                SpinBox {
                    id: fontSizeBox
                    from: 8
                    to: 48
                    editable: true
                    onValueModified: settingsObject.setTerminalFontSize(value)
                }

            }

            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

            Text {
                text: "SESSIONS"
                color: Theme.textFaint
                font.family: "sans-serif"
                font.pixelSize: 9
                font.weight: Font.DemiBold
                font.letterSpacing: 0.8
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 18
                rowSpacing: 10

                Text { text: "Default shell"; color: Theme.textMuted; font.pixelSize: 11 }
                TextField {
                    id: shellField
                    Layout.fillWidth: true
                    color: Theme.text
                    placeholderText: "/bin/bash"
                    onEditingFinished: { settingsObject.setDefaultShell(text); text = settingsObject.defaultShell }
                    background: Rectangle {
                        color: Theme.background
                        border.width: 1
                        border.color: shellField.activeFocus ? Theme.accentMuted : Theme.border
                        radius: Theme.radiusSmall
                    }
                }

                Text { text: "Start directory"; color: Theme.textMuted; font.pixelSize: 11 }
                TextField {
                    id: startDirectoryField
                    Layout.fillWidth: true
                    color: Theme.text
                    placeholderText: "~"
                    onEditingFinished: { settingsObject.setStartDirectory(text); text = settingsObject.startDirectory }
                    background: Rectangle {
                        color: Theme.background
                        border.width: 1
                        border.color: startDirectoryField.activeFocus ? Theme.accentMuted : Theme.border
                        radius: Theme.radiusSmall
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text {
                    Layout.fillWidth: true
                    text: "New tabs inherit the active pane's working directory"
                    color: Theme.textMuted
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
                Switch {
                    id: inheritCwd
                    onToggled: if (!root.syncing) settingsObject.setInheritWorkingDirectory(checked)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text {
                    Layout.fillWidth: true
                    text: "Confirm before closing sessions with running child processes"
                    color: Theme.textMuted
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                }
                Switch {
                    id: confirmClose
                    onToggled: if (!root.syncing) settingsObject.setConfirmCloseRunningProcesses(checked)
                }
            }

            Text {
                Layout.fillWidth: true
                text: "Font changes apply immediately. Shell and start-directory changes apply to new tabs. Settings are saved automatically."
                color: Theme.textFaint
                font.pixelSize: 10
                wrapMode: Text.WordWrap
            }
        }
    }

    footer: Rectangle {
        implicitHeight: 58
        color: Theme.panel

        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.border }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 18
            spacing: 8

            Button {
                id: resetButton
                text: root.resetPending ? "Resetting…" : "Reset defaults"
                enabled: !root.resetPending
                onClicked: {
                    if (root.resetPending)
                        return
                    root.resetPending = true
                    Qt.callLater(function() {
                        root.settingsObject.resetDefaults()
                    })
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Close"
                enabled: !root.resetPending
                onClicked: root.accept()
            }
        }
    }
}
