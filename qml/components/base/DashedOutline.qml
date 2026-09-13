import QtQuick
import Library

// Dashed rounded outline for an empty drop slot. Rectangle.border can only draw
// a solid line, so the stroke is painted on a Canvas the way ProgressRing does.
Item {
    id: root

    property color strokeColor: Theme.textMuted
    property int strokeWidth: Geometry.size.borderWidth
    property int radius: Geometry.radius.md
    property real dashLength: Geometry.spacing.sm
    property real dashGap: Geometry.spacing.xs

    onStrokeColorChanged: canvas.requestPaint()
    onStrokeWidthChanged: canvas.requestPaint()
    onRadiusChanged: canvas.requestPaint()
    onDashLengthChanged: canvas.requestPaint()
    onDashGapChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();

            // Inset by half the stroke so the dashes sit fully inside the item.
            const inset = root.strokeWidth / 2;
            ctx.lineWidth = root.strokeWidth;
            ctx.strokeStyle = root.strokeColor;
            ctx.setLineDash([root.dashLength, root.dashGap]);
            ctx.beginPath();
            ctx.roundedRect(inset, inset, width - root.strokeWidth, height - root.strokeWidth, root.radius, root.radius);
            ctx.stroke();
        }
    }
}
