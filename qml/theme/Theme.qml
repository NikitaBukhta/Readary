pragma Singleton

import QtQuick
import Library

QtObject {
    id: root

    readonly property var availableThemes: [
        {
            id: "pink",
            label: qsTr("Pink")
        },
        {
            id: "blue",
            label: qsTr("Blue")
        },
        {
            id: "yellow",
            label: qsTr("Yellow")
        },
        {
            id: "purple",
            label: qsTr("Purple")
        }
    ]

    property string currentTheme: "pink"

    function setTheme(themeId) {
        for (let i = 0; i < availableThemes.length; ++i) {
            if (availableThemes[i].id === themeId) {
                currentTheme = themeId;
                return true;
            }
        }
        return false;
    }

    readonly property QtObject _pink: PinkPalette {}
    readonly property QtObject _blue: BluePalette {}
    readonly property QtObject _yellow: YellowPalette {}
    readonly property QtObject _purple: PurplePalette {}

    readonly property QtObject palette: {
        switch (currentTheme) {
        case "blue":
            return _blue;
        case "yellow":
            return _yellow;
        case "purple":
            return _purple;
        case "pink":
        default:
            return _pink;
        }
    }

    readonly property color background: palette.background
    readonly property color surface: palette.surface
    readonly property color surfaceVariant: palette.surfaceVariant
    readonly property color searchBackground: palette.searchBackground

    readonly property color textPrimary: palette.textPrimary
    readonly property color textSecondary: palette.textSecondary
    readonly property color textMuted: palette.textMuted

    readonly property color primary: palette.primary
    readonly property color primarySoft: palette.primarySoft
    readonly property color primaryContent: palette.primaryContent

    readonly property color divider: palette.divider
    readonly property color shadow: palette.shadow
    readonly property color ringTrack: palette.ringTrack

    readonly property color starColor: "#F59E0B"
    readonly property color starColorEmpty: "#5AF59E0B" // 35% alpha
}
