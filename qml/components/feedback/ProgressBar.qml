import QtQuick
import Library

Item {
    id: root

    property real progress: 0
    property real startProgress: 0
    property alias trackColor: track.color
    property alias progressColor: fill.color

    readonly property real _progress: Math.max(0, Math.min(1, root.progress))
    readonly property real _start: Math.max(0, Math.min(root._progress, root.startProgress))

    implicitHeight: Styles.progressBar.sm

    Rectangle {
        id: track
        anchors.fill: parent
        radius: height / 2
        color: Qt.rgba(1, 1, 1, 0.55)
    }

    Rectangle {
        id: behind
        width: root._start * parent.width
        height: parent.height
        radius: height / 2
        visible: root._start > 0
        color: root.progressColor
        opacity: Styles.opacity.subdued
    }

    Rectangle {
        id: fill
        x: root._start > 0 ? Math.max(0, root._start * parent.width - height / 2) : 0
        visible: root._progress > root._start
        width: Math.max(0, root._progress * parent.width - fill.x)
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
