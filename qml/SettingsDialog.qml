import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AxiomTTY

Dialog {
    id: root

    required property var settingsObject
    property bool syncing: false
    property bool resetPending: false
    property bool profileDirty: false
    property int pendingFontSize: 14
    property string selectedProfile: "Default"

    width: 650
    height: 700
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    padding: 0
    title: "Settings"

    function indexOfValue(values, value) {
        for (let i = 0; i < values.length; ++i) {
            if (values[i] === value)
                return i
        }
        return 0
    }

    function schemePreviewColor(name) {
        if (name === "Midnight") return "#07111f"
        if (name === "Graphite") return "#151515"
        if (name === "Forest") return "#07150c"
        return "#0b0d10"
    }

    function applyProfileFields() {
        if (!profileDirty || selectedProfile.length === 0)
            return
        settingsObject.setProfileSettings(selectedProfile, profileShellField.text,
                                          profileDirectoryField.text, profileThemeBox.currentText)
        profileDirty = false
        // Do not rebuild the Settings form during the button click. Normalized
        // values are reflected on the next event-loop turn so pointer/focus
        // handling stays responsive even after profile path changes.
        Qt.callLater(function() {
            if (root.visible && !root.profileDirty)
                root.syncProfileFields()
        })
    }

    function syncProfileFields() {
        const names = settingsObject.profileNames
        if (names.length === 0)
            return
        if (names.indexOf(selectedProfile) < 0)
            selectedProfile = settingsObject.activeProfile
        if (names.indexOf(selectedProfile) < 0)
            selectedProfile = names[0]

        profileSelector.currentIndex = indexOfValue(names, selectedProfile)
        profileShellField.text = settingsObject.profileShell(selectedProfile)
        profileDirectoryField.text = settingsObject.profileStartDirectory(selectedProfile)
        const schemes = settingsObject.colorSchemeNames
        profileThemeBox.currentIndex = indexOfValue(schemes, settingsObject.profileColorScheme(selectedProfile))
        removeProfileButton.enabled = settingsObject.profileRemovable(selectedProfile)
        profileDirty = false
    }

    function createRequestedProfile() {
        const requested = newProfileName.text.trim()
        if (requested.length === 0)
            return
        if (settingsObject.createProfile(requested)) {
            selectedProfile = requested
            newProfileName.clear()
            syncFromSettings()
        }
    }

    function syncFromSettings() {
        syncing = true
        fontFamilyField.text = settingsObject.terminalFontFamily
        fontSizeTimer.stop()
        pendingFontSize = settingsObject.terminalFontSize
        fontSizeBox.value = settingsObject.terminalFontSize
        confirmClose.checked = settingsObject.confirmCloseRunningProcesses
        const names = settingsObject.profileNames
        activeProfileBox.currentIndex = indexOfValue(names, settingsObject.activeProfile)
        if (selectedProfile.length === 0 || names.indexOf(selectedProfile) < 0)
            selectedProfile = settingsObject.activeProfile
        syncProfileFields()
        syncing = false
    }

    onOpened: {
        resetPending = false
        selectedProfile = settingsObject.activeProfile
        syncFromSettings()
        fontFamilyField.forceActiveFocus()
    }

    onClosed: {
        resetPending = false
        if (fontSizeTimer.running) {
            fontSizeTimer.stop()
            settingsObject.setTerminalFontSize(pendingFontSize)
        }
    }

    Timer {
        id: fontSizeTimer
        interval: 110
        repeat: false
        onTriggered: settingsObject.setTerminalFontSize(root.pendingFontSize)
    }

    Connections {
        target: root.settingsObject

        function onDefaultsReset() {
            root.selectedProfile = root.settingsObject.activeProfile
            root.syncFromSettings()
            root.resetPending = false
            resetButton.forceActiveFocus()
        }

        function onProfilesChanged() {
            if (!root.visible)
                return
            Qt.callLater(function() {
                if (!root.profileDirty)
                    root.syncFromSettings()
            })
        }

        function onActiveProfileChanged() {
            if (!root.visible || root.syncing)
                return
            Qt.callLater(function() { root.syncFromSettings() })
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
        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: form
            width: parent.width - 36
            x: 18
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
                    onValueModified: {
                        if (root.syncing)
                            return
                        root.pendingFontSize = value
                        fontSizeTimer.restart()
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

            Text {
                text: "PROFILES"
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

                Text { text: "Default for new tabs"; color: Theme.textMuted; font.pixelSize: 11 }
                ComboBox {
                    id: activeProfileBox
                    Layout.fillWidth: true
                    model: settingsObject.profileNames
                    onActivated: if (!root.syncing) settingsObject.setActiveProfile(currentText)
                }

                Text { text: "Edit profile"; color: Theme.textMuted; font.pixelSize: 11 }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    ComboBox {
                        id: profileSelector
                        Layout.fillWidth: true
                        model: settingsObject.profileNames
                        onActivated: {
                            if (root.syncing)
                                return
                            if (root.profileDirty)
                                root.applyProfileFields()
                            root.selectedProfile = currentText
                            root.syncProfileFields()
                        }
                    }
                    Button {
                        id: removeProfileButton
                        text: "Remove"
                        enabled: false
                        onClicked: {
                            if (settingsObject.removeProfile(root.selectedProfile)) {
                                root.selectedProfile = settingsObject.activeProfile
                                root.syncFromSettings()
                            }
                        }
                    }
                }

                Text { text: "Shell"; color: Theme.textMuted; font.pixelSize: 11 }
                TextField {
                    id: profileShellField
                    Layout.fillWidth: true
                    color: Theme.text
                    placeholderText: "/bin/bash"
                    onTextEdited: if (!root.syncing) root.profileDirty = true
                    background: Rectangle {
                        color: Theme.background
                        border.width: 1
                        border.color: profileShellField.activeFocus ? Theme.accentMuted : Theme.border
                        radius: Theme.radiusSmall
                    }
                }

                Text { text: "Start directory"; color: Theme.textMuted; font.pixelSize: 11 }
                TextField {
                    id: profileDirectoryField
                    Layout.fillWidth: true
                    color: Theme.text
                    placeholderText: "~"
                    onTextEdited: if (!root.syncing) root.profileDirty = true
                    background: Rectangle {
                        color: Theme.background
                        border.width: 1
                        border.color: profileDirectoryField.activeFocus ? Theme.accentMuted : Theme.border
                        radius: Theme.radiusSmall
                    }
                }

                Text { text: "Color scheme"; color: Theme.textMuted; font.pixelSize: 11 }
                ComboBox {
                    id: profileThemeBox
                    Layout.fillWidth: true
                    model: settingsObject.colorSchemeNames
                    onActivated: if (!root.syncing) root.profileDirty = true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Rectangle {
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 28
                    radius: Theme.radiusSmall
                    color: root.schemePreviewColor(profileThemeBox.currentText)
                    border.width: 1
                    border.color: Theme.borderStrong
                }

                Text {
                    Layout.fillWidth: true
                    text: root.profileDirty ? "Profile changes are ready to apply" : "Profile is up to date"
                    color: root.profileDirty ? Theme.accent : Theme.textFaint
                    font.pixelSize: 10
                }

                Button {
                    text: "Apply profile"
                    enabled: root.profileDirty && !root.resetPending
                    onClicked: root.applyProfileFields()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                TextField {
                    id: newProfileName
                    Layout.fillWidth: true
                    placeholderText: "New profile name"
                    color: Theme.text
                    background: Rectangle {
                        color: Theme.background
                        border.width: 1
                        border.color: newProfileName.activeFocus ? Theme.accentMuted : Theme.border
                        radius: Theme.radiusSmall
                    }
                    onAccepted: root.createRequestedProfile()
                }
                Button {
                    id: addProfileButton
                    text: "Add profile"
                    enabled: newProfileName.text.trim().length > 0
                    onClicked: root.createRequestedProfile()
                }
            }

            Text {
                Layout.fillWidth: true
                text: "A profile controls shell, start directory and terminal colors. Edit the fields, then press Apply profile. Existing tabs that use this profile update their colors without restarting the shell."
                color: Theme.textFaint
                font.pixelSize: 10
                wrapMode: Text.WordWrap
            }

            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border }

            Text {
                text: "BEHAVIOR"
                color: Theme.textFaint
                font.family: "sans-serif"
                font.pixelSize: 9
                font.weight: Font.DemiBold
                font.letterSpacing: 0.8
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
                text: "New tabs always start in the selected profile's configured start directory. Duplicate Tab and split/duplicate pane actions intentionally keep the current working directory. Font changes are lightly debounced; profile edits are applied in-memory as one batch and persisted only when AxiomTTY exits, keeping the Settings UI free of synchronous disk I/O."
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
                    Qt.callLater(function() { root.settingsObject.resetDefaults() })
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Close"
                enabled: !root.resetPending
                onClicked: {
                    root.applyProfileFields()
                    root.accept()
                }
            }
        }
    }
}
