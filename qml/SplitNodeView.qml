import QtQuick
import QtQuick.Controls
import AxiomTTY

Item {
    id: root
    clip: true

    // These are intentionally normal properties instead of `required` ones.
    // Child SplitNodeView instances are created dynamically by Loader below,
    // which avoids Qt's static recursive-type rejection.
    property var node: null
    property var sessionManager: null


    // SplitNode objects are mutated in-place when a pane is removed and its
    // sibling is promoted. Recreate this visual subtree on every structural
    // mutation so no Loader/painted-item from the removed pane can survive
    // into the expanded sibling area.
    Connections {
        target: root.node
        ignoreUnknownSignals: true
        function onStructureChanged() {
            contentLoader.active = false
            Qt.callLater(function() {
                if (root.node)
                    contentLoader.active = true
            })
        }
    }

    Loader {
        id: contentLoader
        anchors.fill: parent
        sourceComponent: !root.node ? emptyComponent : (root.node.leaf ? leafComponent : splitComponent)
    }

    Component {
        id: emptyComponent
        Item {}
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
        }
    }

    Component {
        id: splitComponent

        SplitView {
            id: splitView
            anchors.fill: parent
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

                property var childNode: root.node ? root.node.firstNode : null

                source: childNode ? Qt.resolvedUrl("SplitNodeView.qml") : ""

                SplitView.fillWidth: splitView.orientation === Qt.Horizontal
                SplitView.fillHeight: splitView.orientation === Qt.Vertical
                SplitView.minimumWidth: 180
                SplitView.minimumHeight: 120

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

                property var childNode: root.node ? root.node.secondNode : null

                source: childNode ? Qt.resolvedUrl("SplitNodeView.qml") : ""

                SplitView.preferredWidth: splitView.orientation === Qt.Horizontal ? Math.max(180, splitView.width / 2) : undefined
                SplitView.preferredHeight: splitView.orientation === Qt.Vertical ? Math.max(120, splitView.height / 2) : undefined
                SplitView.minimumWidth: 180
                SplitView.minimumHeight: 120

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
