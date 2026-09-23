pragma Singleton

import QtQuick

QtObject {
    component WindowSpec: QtObject {
        readonly property int defaultWidth: 412
        readonly property int defaultHeight: 892
        readonly property int minimumWidth: 360
        readonly property int minimumHeight: 640
        readonly property int contentMaxWidth: 520
    }

    component SpacingSpec: QtObject {
        readonly property int xxs: 2
        readonly property int xs: 4
        readonly property int sm: 8
        readonly property int md: 12
        readonly property int lg: 16
        readonly property int xl: 20
        readonly property int xxl: 24
        readonly property int huge: 32
    }

    component RadiusSpec: QtObject {
        readonly property int xs: 6
        readonly property int sm: 10
        readonly property int md: 14
        readonly property int lg: 18
        readonly property int xl: 24
        readonly property int pill: 999
    }

    component SizeSpec: QtObject {
        readonly property int iconSm: 16
        readonly property int iconMd: 20
        readonly property int iconLg: 24
        readonly property int iconXl: 28
        readonly property int iconHuge: 36

        readonly property int avatarSm: 36
        readonly property int avatarMd: 48
        readonly property int avatarLg: 64

        readonly property int searchHeight: 52
        readonly property int goalRing: 64
        readonly property int goalRingInner: goalRing - 2 * goalRingStroke
        readonly property int goalRingStroke: 4
        readonly property int spinner: 28
        readonly property int spinnerStroke: 3

        readonly property int readingCardWidth: 140
        readonly property int readingCoverHeight: 190

        readonly property int timelineGutter: 24
        readonly property int timelineDot: 12
        readonly property int timelineRail: 2
        readonly property int iconTile: 36

        readonly property int categoryRowHeight: 72
        readonly property int categoryIconBox: 48

        readonly property int bookRowHeight: 104
        readonly property int bookRowCoverWidth: 72
        readonly property int bookRowCoverHeight: 88

        readonly property int bookDetailCoverWidth: 100
        readonly property int bookDetailCoverHeight: 140

        readonly property int borderWidth: 1

        readonly property int coverPickerWidth: 140
        readonly property int coverPickerHeight: 180
        readonly property int formTextAreaHeight: 120

        readonly property int menuWidth: 220
        readonly property int menuItemHeight: 44

        readonly property int actionButtonHeight: 56
        readonly property int pillButtonHeight: 52
        readonly property int characterRowHeight: 64
        readonly property int dialogPrimaryWidth: 160

        readonly property int bottomNavHeight: 68
        readonly property int bottomNavIcon: 22
    }

    component ChartSpec: QtObject {
        readonly property int barPlotHeight: 120
        readonly property int barMinHeight: 3
        readonly property int linePlotHeight: 190
        readonly property int curvePlotHeight: 110
        // Both gutters carry a rotated/centred axis caption beside the tick
        // labels, so they are wider than the ticks alone would need.
        readonly property int axisGutterLeft: 48
        readonly property int axisGutterBottom: 38
        readonly property int axisCaptionSlot: 14
        readonly property int pointRadius: 4
        // Generous next to the 4px marker: on a line chart the reader aims at
        // the curve, not at the dot, and a finger is nowhere near that precise.
        readonly property int pointHitRadius: 24
        readonly property int lineWidth: 2
        readonly property var gridDash: [2, 3]
        readonly property int gridRows: 4
        readonly property int maxAxisLabels: 5
    }

    readonly property WindowSpec window: WindowSpec {}
    readonly property SpacingSpec spacing: SpacingSpec {}
    readonly property RadiusSpec radius: RadiusSpec {}
    readonly property SizeSpec size: SizeSpec {}
    readonly property ChartSpec chart: ChartSpec {}
}
