pragma Singleton
import QtQuick

QtObject {
    // AxiomTTY UI palette. Keep the application chrome neutral and let
    // terminal ANSI colors remain independent from the UI theme.
    readonly property color background: "#0b0d10"
    readonly property color panel: "#0f1217"
    readonly property color raised: "#15191f"
    readonly property color hover: "#1a2028"
    readonly property color tabActive: "#151a21"

    readonly property color border: "#222832"
    readonly property color borderStrong: "#303946"
    readonly property color borderInactive: "#1a1f27"

    readonly property color text: "#e6eaf0"
    readonly property color textMuted: "#98a2af"
    readonly property color textFaint: "#626d7b"

    readonly property color accent: "#7aa2f7"
    readonly property color accentMuted: "#344967"
    readonly property color success: "#73c59b"
    readonly property color danger: "#e47777"
    readonly property color closeHover: "#c94f55"

    readonly property int headerHeight: 42
    readonly property int statusHeight: 24
    readonly property int radiusSmall: 3
    readonly property int radiusMedium: 5

    readonly property int spacingTiny: 4
    readonly property int spacingSmall: 8
    readonly property int spacing: 12
    readonly property int spacingLarge: 18

    readonly property int uiFontSize: 12
    readonly property int terminalFontSize: 14
}
