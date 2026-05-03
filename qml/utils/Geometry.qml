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

        readonly property int searchHeight: 52
        readonly property int goalRing: 64
        readonly property int goalRingInner: goalRing - 2 * goalRingStroke
        readonly property int goalRingStroke: 4

        readonly property int readingCardWidth: 140
        readonly property int readingCoverHeight: 190

        readonly property int categoryRowHeight: 72
        readonly property int categoryIconBox: 48

        readonly property int bookRowHeight: 104
        readonly property int bookRowCoverWidth: 72
        readonly property int bookRowCoverHeight: 88

        readonly property int bookDetailCoverWidth: 100
        readonly property int bookDetailCoverHeight: 140

        readonly property int actionButtonHeight: 56
        readonly property int pillButtonHeight: 52
        readonly property int characterRowHeight: 64

        readonly property int bottomNavHeight: 68
        readonly property int bottomNavIcon: 22
    }

    readonly property WindowSpec window: WindowSpec {}
    readonly property SpacingSpec spacing: SpacingSpec {}
    readonly property RadiusSpec radius: RadiusSpec {}
    readonly property SizeSpec size: SizeSpec {}
}
