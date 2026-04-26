import QtQuick
import Library

Item {
    id: root

    property real progress: 0           // 0..1
    property int strokeWidth: Geometry.size.goalRingStroke
    property color trackColor: Theme.ringTrack
    property color progressColor: Theme.primary

    implicitWidth: Geometry.size.goalRing
    implicitHeight: Geometry.size.goalRing

    onProgressChanged: canvas.requestPaint()
    onTrackColorChanged: canvas.requestPaint()
    onProgressColorChanged: canvas.requestPaint()
    onStrokeWidthChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const cx = width / 2
            const cy = height / 2
            const r = Math.min(cx, cy) - root.strokeWidth / 2

            ctx.lineWidth = root.strokeWidth
            ctx.lineCap = "round"

            ctx.strokeStyle = root.trackColor
            ctx.beginPath()
            ctx.arc(cx, cy, r, 0, Math.PI * 2)
            ctx.stroke()

            const clamped = Math.max(0, Math.min(1, root.progress))
            if (clamped <= 0)
                return

            ctx.strokeStyle = root.progressColor
            ctx.beginPath()
            ctx.arc(cx, cy, r, -Math.PI / 2,
                    -Math.PI / 2 + Math.PI * 2 * clamped)
            ctx.stroke()
        }
    }
}
