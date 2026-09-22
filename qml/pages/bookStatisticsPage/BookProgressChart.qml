pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

PaddedCard {
    id: root

    property alias title: header.title
    property alias subtitle: subtitleText.text
    property alias valueAxisCaption: valueAxisText.text
    property alias stepAxisCaption: stepAxisText.text
    property string pointTemplate: "%1"
    property var points: []

    // Not knobs for the caller — they exist so a theme switch repaints the
    // Canvas, which nothing else would notice.
    readonly property color lineColor: Theme.primary
    readonly property color areaColor: Theme.primarySoft
    readonly property color gridColor: Theme.divider

    readonly property int _count: root.points ? root.points.length : 0
    readonly property int _peakPage: {
        let peak = 0;
        for (let i = 0; i < root._count; ++i)
            peak = Math.max(peak, root.points[i].page);
        return peak;
    }
    // A book read but never past page 0 would divide by zero; one grid step is
    // the smallest scale that still draws.
    readonly property int _scale: Math.max(1, root._peakPage)

    // The plot rect is inset from the canvas on the right by the marker radius
    // and on top by half a label, so the newest point and the topmost tick are
    // not sliced in half by the edge. Shared by the Canvas and the tick labels
    // so the two can never drift apart.
    readonly property real _plotLeft: Geometry.chart.axisGutterLeft
    readonly property real _plotRight: plotArea.width - Geometry.chart.pointRadius
    readonly property real _plotTop: Math.ceil(Styles.fontSize.caption / 2)
    readonly property real _plotBottom: plotArea.height - Geometry.chart.axisGutterBottom
    readonly property real _plotWidth: Math.max(0, root._plotRight - root._plotLeft)
    readonly property real _plotHeight: Math.max(0, root._plotBottom - root._plotTop)

    // Which sessions get an x tick. The newest is always one of them: it is
    // the number the reader came to the page for.
    readonly property var _stepTicks: {
        const ticks = [];
        if (root._count === 0)
            return ticks;
        const step = Math.max(1, Math.ceil(root._count / Geometry.chart.maxAxisLabels));
        for (let i = 0; i < root._count; i += step)
            ticks.push(i);
        if (ticks[ticks.length - 1] !== root._count - 1)
            ticks.push(root._count - 1);
        return ticks;
    }

    // The point the tooltip describes, and whether a tap pinned it there. A
    // pinned tooltip ignores the pointer until it is dismissed.
    property int _activeIndex: -1
    property bool _pinned: false

    // Matched on x alone: the reader aims at a column of the chart, not at the
    // dot itself, and on a line chart the y is whatever the curve says.
    function _nearestIndex(pointerX: real): int {
        let nearest = -1;
        let best = Geometry.chart.pointHitRadius;
        for (let i = 0; i < root._count; ++i) {
            const distance = Math.abs(root._xAt(i) - pointerX);
            if (distance <= best) {
                best = distance;
                nearest = i;
            }
        }
        return nearest;
    }

    function _showTooltip(index: int, pinned: bool): void {
        if (index < 0) {
            tooltip.close();
            return;
        }
        root._activeIndex = index;
        root._pinned = pinned;
        tooltip.open();
    }

    function _xAt(pointIndex: int): real {
        if (root._count <= 1)
            return root._plotLeft + (root._plotWidth / 2);
        return root._plotLeft + (root._plotWidth * pointIndex / (root._count - 1));
    }

    function _yAt(page: int): real {
        return root._plotBottom - (root._plotHeight * Math.min(page, root._scale) / root._scale);
    }

    function _rowY(row: int): real {
        return root._plotBottom - (root._plotHeight * row / Geometry.chart.gridRows);
    }

    function _rowValue(row: int): int {
        return Math.round(root._scale * row / Geometry.chart.gridRows);
    }

    onPointsChanged: {
        plot.requestPaint();
        // A tooltip left over from the previous book would point at a session
        // that is no longer on the chart.
        tooltip.close();
    }
    onLineColorChanged: plot.requestPaint()
    onAreaColorChanged: plot.requestPaint()
    onGridColorChanged: plot.requestPaint()

    // Nothing here paints text. Canvas takes a CSS font string, and the only
    // family token this app exposes is the bundled emoji face, which carries
    // no digits — the ticks came out blank. They are Text items below instead,
    // picking up the real application font the way every other label does.
    function _paint(ctx): void {
        ctx.reset();
        if (root._count === 0)
            return;

        // The curve is walked twice rather than once and reused: closing the
        // path down to the baseline for the fill would make that baseline part
        // of the stroked line.
        const linePath = () => {
            ctx.beginPath();
            ctx.moveTo(root._xAt(0), root._yAt(root.points[0].page));
            for (let i = 1; i < root._count; ++i)
                ctx.lineTo(root._xAt(i), root._yAt(root.points[i].page));
        };

        linePath();
        ctx.lineTo(root._xAt(root._count - 1), root._plotBottom);
        ctx.lineTo(root._xAt(0), root._plotBottom);
        ctx.closePath();
        ctx.fillStyle = root.areaColor;
        ctx.fill();

        // Grid over the fill, not under it: with only a couple of sessions the
        // filled area covers most of the plot and would bury every line.
        ctx.lineWidth = Geometry.size.borderWidth;
        ctx.strokeStyle = root.gridColor;
        ctx.setLineDash(Geometry.chart.gridDash);
        for (let row = 0; row <= Geometry.chart.gridRows; ++row) {
            const y = root._rowY(row);
            ctx.beginPath();
            ctx.moveTo(root._plotLeft, y);
            ctx.lineTo(root._plotRight, y);
            ctx.stroke();
        }
        ctx.setLineDash([]);

        linePath();
        ctx.strokeStyle = root.lineColor;
        ctx.lineWidth = Geometry.chart.lineWidth;
        ctx.lineJoin = "round";
        ctx.stroke();

        ctx.fillStyle = root.lineColor;
        for (let i = 0; i < root._count; ++i) {
            ctx.beginPath();
            ctx.arc(root._xAt(i), root._yAt(root.points[i].page), Geometry.chart.pointRadius, 0, Math.PI * 2);
            ctx.fill();
        }
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.lg

        ColumnLayout {
            id: heading
            Layout.fillWidth: true
            spacing: Geometry.spacing.xxs

            SectionHeader {
                id: header
                Layout.fillWidth: true
                titleSize: Styles.fontSize.title
            }

            Text {
                id: subtitleText
                Layout.fillWidth: true
                visible: root.subtitle.length > 0
                color: Theme.textMuted
                font.pixelSize: Styles.fontSize.small
                wrapMode: Text.WordWrap
            }
        }

        Item {
            id: plotArea
            Layout.fillWidth: true
            Layout.preferredHeight: Geometry.chart.linePlotHeight

            Canvas {
                id: plot
                anchors.fill: parent
                antialiasing: true
                onPaint: root._paint(plot.getContext("2d"))
            }

            Repeater {
                id: valueTicks
                model: Geometry.chart.gridRows + 1

                delegate: Text {
                    id: valueTick

                    required property int index

                    // Kept clear of the first x tick, which is centred on the
                    // plot's left edge and reaches back past it.
                    x: root._plotLeft - width - Geometry.spacing.md
                    y: root._rowY(valueTick.index) - (height / 2)
                    text: root._rowValue(valueTick.index)
                    color: Theme.textMuted
                    font.pixelSize: Styles.fontSize.caption
                }
            }

            Repeater {
                id: stepTicks
                model: root._stepTicks

                delegate: Text {
                    id: stepTick

                    required property var modelData

                    x: root._xAt(stepTick.modelData) - (width / 2)
                    y: root._plotBottom + Geometry.spacing.xxs
                    text: root.points[stepTick.modelData].session
                    color: Theme.textMuted
                    font.pixelSize: Styles.fontSize.caption
                }
            }

            // Captions are Text items for the same reason as the ticks, plus
            // they need qsTr.
            Item {
                id: valueAxisSlot
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Geometry.chart.axisGutterBottom
                width: Geometry.chart.axisCaptionSlot

                Text {
                    id: valueAxisText
                    anchors.centerIn: parent
                    rotation: -90
                    color: Theme.textMuted
                    font.pixelSize: Styles.fontSize.caption
                    font.weight: Styles.fontWeight.semibold
                }
            }

            Text {
                id: stepAxisText
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                color: Theme.textMuted
                font.pixelSize: Styles.fontSize.caption
                font.weight: Styles.fontWeight.semibold
            }

            MouseArea {
                id: pointer
                anchors.fill: parent
                hoverEnabled: true
                onPositionChanged: mouse => {
                    if (!root._pinned)
                        root._showTooltip(root._nearestIndex(mouse.x), false);
                }
                onExited: {
                    if (!root._pinned)
                        tooltip.close();
                }
                // -1 when the tap landed nowhere near the curve, which doubles
                // as "dismiss" for a tap inside the chart.
                onClicked: mouse => root._showTooltip(root._nearestIndex(mouse.x), true)
            }

            // A Popup rather than a plain item: Controls then owns the
            // press-anywhere-to-dismiss and the escape key, the same way
            // ActionMenu and ConfirmDialog get them.
            Popup {
                id: tooltip

                readonly property int _page: root._activeIndex >= 0 ? root.points[root._activeIndex].page : 0

                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                padding: Geometry.spacing.xs
                leftPadding: Geometry.spacing.sm
                rightPadding: Geometry.spacing.sm
                // Centred on the point but kept inside the plot, so the newest
                // session's tooltip does not hang off the card.
                x: root._activeIndex < 0 ? 0 : Math.max(0, Math.min(plotArea.width - width, root._xAt(root._activeIndex) - (width / 2)))
                y: root._tooltipY(height)

                onClosed: {
                    root._pinned = false;
                    root._activeIndex = -1;
                }

                background: SurfaceCard {
                    color: Theme.primary
                    radius: Geometry.radius.sm
                    shadowOffset: Styles.elevation.subtleOffset
                    shadowBlur: Styles.elevation.subtleBlur
                }

                contentItem: Text {
                    text: root.pointTemplate.arg(tooltip._page)
                    color: Theme.primaryContent
                    font.pixelSize: Styles.fontSize.caption
                    font.weight: Styles.fontWeight.semibold
                }
            }
        }
    }

    // Above the marker, unless the point sits so high that the tooltip would
    // be clipped by the top of the plot — then it flips underneath.
    function _tooltipY(tooltipHeight: real): real {
        if (root._activeIndex < 0)
            return 0;
        const marker = root._yAt(root.points[root._activeIndex].page);
        const above = marker - Geometry.chart.pointRadius - Geometry.spacing.xs - tooltipHeight;
        return above >= 0 ? above : marker + Geometry.chart.pointRadius + Geometry.spacing.xs;
    }
}
