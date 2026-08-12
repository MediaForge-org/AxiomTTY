pragma Singleton
import QtQuick

QtObject {
    readonly property color background: "#0d0f12"
    readonly property color panel: "#11141a"
    readonly property color raised: "#171b22"
    readonly property color border: "#262c35"
    readonly property color borderStrong: "#343c48"

    readonly property color text: "#e9edf2"
    readonly property color textMuted: "#8e98a5"
    readonly property color textFaint: "#5f6874"

    readonly property color accent: "#5b8def"
    readonly property color success: "#75b798"
    readonly property color danger: "#d97878"

    readonly property int radiusSmall: 4
    readonly property int radiusMedium: 6
    readonly property int spacingSmall: 8
    readonly property int spacing: 12
    readonly property int spacingLarge: 18
}
