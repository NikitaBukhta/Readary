pragma Singleton

import QtQuick

QtObject {
    component FontSizeSpec: QtObject {
        readonly property int caption:      11
        readonly property int small:        12
        readonly property int bodySmall:    13
        readonly property int body:         14
        readonly property int bodyLarge:    16
        readonly property int title:        18
        readonly property int titleMedium:  22
        readonly property int titleLarge:   28
        readonly property int display:      36
    }

    component FontWeightSpec: QtObject {
        readonly property int regular:  Font.Normal
        readonly property int medium:   Font.Medium
        readonly property int semibold: Font.DemiBold
        readonly property int bold:     Font.Bold
        readonly property int black:    Font.Black
    }

    component ElevationSpec: QtObject {
        readonly property int card:  8
        readonly property int sheet: 16
    }

    component OpacitySpec: QtObject {
        readonly property real pressed:  0.7
        readonly property real disabled: 0.4
    }

    component DurationSpec: QtObject {
        readonly property int fast:   120
        readonly property int normal: 200
        readonly property int slow:   320
    }

    readonly property FontSizeSpec   fontSize:   FontSizeSpec   {}
    readonly property FontWeightSpec fontWeight: FontWeightSpec {}
    readonly property ElevationSpec  elevation:  ElevationSpec  {}
    readonly property OpacitySpec    opacity:    OpacitySpec    {}
    readonly property DurationSpec   duration:   DurationSpec   {}
}
