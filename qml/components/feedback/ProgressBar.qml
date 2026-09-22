import QtQuick
import Library

Item {
    id: root

    property real progress: 0            // 0..1
    property alias trackColor: track.color
    property alias progressColor: fill.color

    implicitHeight: Styles.progressBar.sm

    Rectangle {
        id: track
        anchors.fill: parent
        radius: height / 2
        color: Qt.rgba(1, 1, 1, 0.55)
    }

    Rectangle {
        id: fill
        width: Math.max(0, Math.min(1, root.progress)) * parent.width
        height: parent.height
        radius: height / 2
        color: Theme.primary

        Behavior on width {
            NumberAnimation {
                duration: Styles.duration.normal
                easing.type: Easing.OutCubic
            }
        }
    }
}
