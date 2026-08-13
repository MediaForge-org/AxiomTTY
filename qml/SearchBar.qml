import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AxiomTTY

Rectangle {
    id: root

    required property var terminalView

    width: Math.max(150, Math.min(390, (parent ? parent.width : 414) - 24))
    height: 38
    radius: Theme.radiusMedium
    color: Theme.raised
    border.width: 1
    border.color: searchInput.activeFocus ? Theme.accentMuted : Theme.borderStrong

    function focusInput() {
        if (!root.terminalView)
            return
        if (searchInput.text !== root.terminalView.searchQuery)
            searchInput.text = root.terminalView.searchQuery
        Qt.callLater(function() {
            searchInput.forceActiveFocus()
            searchInput.selectAll()
        })
    }

    function closeSearch() {
        if (!root.terminalView)
            return
        root.terminalView.endSearch()
        Qt.callLater(function() { root.terminalView.forceActiveFocus() })
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 5
        spacing: 4

        TextField {
            id: searchInput
            Layout.fillWidth: true
            Layout.fillHeight: true
            leftPadding: 4
            rightPadding: 4
            placeholderText: "Search scrollback"
            color: Theme.text
            placeholderTextColor: Theme.textFaint
            selectionColor: Theme.accentMuted
            selectedTextColor: Theme.text
            font.family: "sans-serif"
            font.pixelSize: 12
            background: Item {}

            onTextChanged: {
                if (root.terminalView && root.terminalView.searchQuery !== text)
                    root.terminalView.searchQuery = text
            }

            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Escape) {
                    root.closeSearch()
                    event.accepted = true
                    return
                }

                if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_F3) {
                    if (event.modifiers & Qt.ShiftModifier)
                        root.terminalView.findPrevious()
                    else
                        root.terminalView.findNext()
                    event.accepted = true
                    return
                }

                if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_F) {
                    searchInput.selectAll()
                    event.accepted = true
                }
            }
        }

        Text {
            Layout.preferredWidth: 52
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: !root.terminalView || root.terminalView.searchQuery.length === 0
                  ? ""
                  : root.terminalView.searchMatchCount === 0
                    ? "0 / 0"
                    : root.terminalView.currentSearchMatch + " / " + root.terminalView.searchMatchCount
            color: root.terminalView && root.terminalView.searchMatchCount > 0 ? Theme.textMuted : Theme.danger
            font.family: "sans-serif"
            font.pixelSize: 10
        }

        Rectangle {
            id: caseButton
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            radius: Theme.radiusSmall
            color: root.terminalView && root.terminalView.searchCaseSensitive
                   ? Theme.accentMuted
                   : caseMouse.containsMouse ? Theme.hover : "transparent"

            Text {
                anchors.centerIn: parent
                text: "Aa"
                color: root.terminalView && root.terminalView.searchCaseSensitive ? Theme.text : Theme.textMuted
                font.family: "sans-serif"
                font.pixelSize: 10
                font.weight: Font.DemiBold
            }

            MouseArea {
                id: caseMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (root.terminalView)
                        root.terminalView.searchCaseSensitive = !root.terminalView.searchCaseSensitive
                    searchInput.forceActiveFocus()
                }
            }

            ToolTip.visible: caseMouse.containsMouse
            ToolTip.text: "Match case"
            ToolTip.delay: 450
        }

        Repeater {
            model: [
                { label: "↑", tip: "Previous match  ·  Shift+Enter", action: "previous" },
                { label: "↓", tip: "Next match  ·  Enter", action: "next" },
                { label: "×", tip: "Close search  ·  Esc", action: "close" }
            ]

            delegate: Rectangle {
                required property var modelData
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                radius: Theme.radiusSmall
                color: buttonMouse.containsMouse ? Theme.hover : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: modelData.label
                    color: buttonMouse.containsMouse ? Theme.text : Theme.textMuted
                    font.family: "sans-serif"
                    font.pixelSize: modelData.action === "close" ? 16 : 13
                }

                MouseArea {
                    id: buttonMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (modelData.action === "previous")
                            root.terminalView.findPrevious()
                        else if (modelData.action === "next")
                            root.terminalView.findNext()
                        else
                            root.closeSearch()

                        if (modelData.action !== "close")
                            searchInput.forceActiveFocus()
                    }
                }

                ToolTip.visible: buttonMouse.containsMouse
                ToolTip.text: modelData.tip
                ToolTip.delay: 450
            }
        }
    }
}
