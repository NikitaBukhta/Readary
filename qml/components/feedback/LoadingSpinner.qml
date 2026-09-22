import QtQuick
import Library

Item {
    id: root

    property bool running: true
    property int diameter: Geometry.size.spinner
    property alias arcColor: ring.progressColor
    property alias trackColor: ring.trackColor

    readonly property real _arcSweep: 0.25

    implicitWidth: root.diameter
    implicitHeight: root.diameter
    visible: root.running

    ProgressRing {
        id: ring
        anchors.fill: parent
        progress: root._arcSweep
        strokeWidth: Geometry.size.spinnerStroke
        transformOrigin: Item.Center

        RotationAnimator on rotation {
            from: 0
            to: 360
            duration: Styles.duration.spin
            loops: Animation.Infinite
            running: root.running
        }
    }
}
