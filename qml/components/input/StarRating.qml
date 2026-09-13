pragma ComponentBehavior: Bound
import QtQuick
import Library

Row {
    id: root

    property real value: 0
    property int total: 10
    property int starSize: Geometry.size.iconMd
    property color filledColor: Theme.starColor
    property color emptyColor: Theme.starColorEmpty

    readonly property int _filled: Math.max(0, Math.min(root.total, Math.round(root.value)))

    spacing: 1

    Repeater {
        model: root.total

        delegate: Text {
            id: star
            required property int index

            text: index < root._filled ? "★" : "☆"
            color: index < root._filled ? root.filledColor : root.emptyColor
            font.pixelSize: root.starSize
        }
    }
}
