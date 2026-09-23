import QtQuick
import QtQuick.Controls
import AxiomTTY

Item {
    id: root
    clip: true

    // Child nodes are loaded dynamically to keep recursive split trees valid in
    // QML. The C++ tree remains binary, while span metadata lets the renderer
    // distribute nested same-direction splits as equal visual slots.
    property var node: null
    property var sessionManager: null

    function childSpan(childNode, orientation) {
        if (!childNode)
            return 1
        return orientation === Qt.Horizontal
                ? Math.max(1, childNode.horizontalSpan)
                : Math.max(1, childNode.verticalSpan)
    }

    function preferredFirstExtent() {
        if (!root.node || root.node.leaf)
            return 0

        const orientation = root.node.orientation
        const firstSpan = childSpan(root.node.firstNode, orientation)
        const secondSpan = childSpan(root.node.secondNode, orientation)
        const totalSpan = Math.max(1, firstSpan + secondSpan)
        const handleExtent = 5
        const available = orientation === Qt.Horizontal
                ? Math.max(0, root.width - handleExtent)
                : Math.max(0, root.height - handleExtent)
        return available * firstSpan / totalSpan
    }

    Loader {
        id: contentLoader
        anchors.fill: parent
        sourceComponent: !root.node ? emptyComponent : (root.node.leaf ? leafComponent : splitComponent)
    }

    Component {
        id: emptyComponent
        Rectangle { color: Theme.background }
    }

    Component {
        id: leafComponent

        TerminalPane {
            session: root.node ? root.node.sessionObject : null
            paneActive: session !== null && root.sessionManager && root.sessionManager.activeSession === session

            onActivated: {
                if (session && root.sessionManager)
                    root.sessionManager.activatePane(session)
            }

            onCloseRequested: function(sessionToClose) {
                if (sessionToClose && root.sessionManager)
                    root.sessionManager.requestClosePane(sessionToClose)
            }
        }
    }

    Component {
        id: splitComponent

        SplitView {
            id: splitView
            anchors.fill: parent
            clip: true
            orientation: root.node ? root.node.orientation : Qt.Horizontal

            handle: Rectangle {
                implicitWidth: splitView.orientation === Qt.Horizontal ? 5 : 1
                implicitHeight: splitView.orientation === Qt.Vertical ? 5 : 1
                color: SplitHandle.hovered || SplitHandle.pressed ? Theme.borderStrong : Theme.border

                Rectangle {
                    anchors.centerIn: parent
                    width: splitView.orientation === Qt.Horizontal ? 1 : 30
                    height: splitView.orientation === Qt.Horizontal ? 30 : 1
                    color: SplitHandle.hovered || SplitHandle.pressed ? Theme.accentMuted : Theme.borderStrong
                }
            }

            Loader {
                id: firstChildLoader
                clip: true

                property var childNode: root.node ? root.node.firstNode : null

                source: childNode ? Qt.resolvedUrl("SplitNodeView.qml") : ""

                // The first branch gets exactly its share of the logical slots;
                // the second branch fills the remainder. With a horizontal chain
                // of 8 leaves this produces 1/8 + 1/8 + ... instead of the old
                // 1/2 + 1/4 + 1/8 cascade.
                SplitView.preferredWidth: splitView.orientation === Qt.Horizontal ? root.preferredFirstExtent() : undefined
                SplitView.preferredHeight: splitView.orientation === Qt.Vertical ? root.preferredFirstExtent() : undefined
                SplitView.minimumWidth: 96
                SplitView.minimumHeight: 72

                onLoaded: {
                    item.node = childNode
                    item.sessionManager = root.sessionManager
                }

                onChildNodeChanged: {
                    if (item)
                        item.node = childNode
                }
            }

            Loader {
                id: secondChildLoader
                clip: true

                property var childNode: root.node ? root.node.secondNode : null

                source: childNode ? Qt.resolvedUrl("SplitNodeView.qml") : ""

                SplitView.fillWidth: splitView.orientation === Qt.Horizontal
                SplitView.fillHeight: splitView.orientation === Qt.Vertical
                SplitView.minimumWidth: 96
                SplitView.minimumHeight: 72

                onLoaded: {
                    item.node = childNode
                    item.sessionManager = root.sessionManager
                }

                onChildNodeChanged: {
                    if (item)
                        item.node = childNode
                }
            }
        }
    }
}
