import QtQuick
import Library

SurfaceCard {
    id: root

    property color restColor: Theme.surface
    property color pressedColor: Theme.primarySoft
    readonly property alias pressed: pressArea.pressed

    signal clicked

    color: pressArea.pressed ? root.pressedColor : root.restColor

    MouseArea {
        id: pressArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
