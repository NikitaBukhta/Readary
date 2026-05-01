import QtQuick
import Library

Item {
    id: root

    property real progress: 0            // 0..1
    property color trackColor: Qt.rgba(1, 1, 1, 0.55)
    property color progressColor: Theme.primary

    implicitHeight: 4

    Rectangle {
        id: track
        anchors.fill: parent
        radius: height / 2
        color: root.trackColor
    }

    Rectangle {
        id: fill
        width: Math.max(0, Math.min(1, root.progress)) * parent.width
        height: parent.height
        radius: height / 2
        color: root.progressColor

        Behavior on width {
            NumberAnimation {
                duration: Styles.duration.normal
                easing.type: Easing.OutCubic
            }
        }
    }
}
