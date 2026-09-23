pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

PaddedCard {
    id: root

    property alias title: header.title
    property alias emptyText: emptyPlot.text
    // [{year, month, books}], oldest first — the order
    // ReadingStatisticsController hands over.
    property var months: []

    // Not knobs for the caller — they exist so a theme switch repaints the
    // Canvas, which nothing else would notice.
    readonly property color lineColor: Theme.primary
    readonly property color areaColor: Theme.primarySoft

    readonly property var _months: root.months ?? []
    readonly property int _count: root._months.length
    readonly property int _peak: {
        let peak = 0;
        for (let i = 0; i < root._count; ++i)
            peak = Math.max(peak, root._months[i].books);
        return peak;
    }

    // Each month owns an equal column and its point sits at the column's
    // centre, so the curve stays in line with the month labels below it the
    // same way WeeklyPagesChart's bars do. The top keeps a count label's worth
    // of headroom, measured because the app font is a user setting.
    readonly property real _plotTop: valueMetrics.height + Geometry.spacing.xxs
    readonly property real _plotBottom: plotArea.height - Geometry.chart.lineWidth
    readonly property real _plotHeight: Math.max(0, root._plotBottom - root._plotTop)

    function _xAt(index: int): real {
        return plotArea.width * (index + 0.5) / Math.max(1, root._count);
    }

    function _yAt(books: int): real {
        return root._plotBottom - (root._plotHeight * books / Math.max(1, root._peak));
    }

    onMonthsChanged: plot.requestPaint()
    onLineColorChanged: plot.requestPaint()
    onAreaColorChanged: plot.requestPaint()

    function _paint(ctx): void {
        ctx.reset();
        if (root._count === 0 || root._peak <= 0)
            return;

        // Each segment is a cubic whose control points share the x halfway
        // between the two months, so the curve eases in and out of every
        // point and never overshoots it — a month with no book stays on the
        // baseline instead of dipping below it.
        const curvePath = () => {
            ctx.beginPath();
            ctx.moveTo(root._xAt(0), root._yAt(root._months[0].books));
            for (let i = 1; i < root._count; ++i) {
                const x0 = root._xAt(i - 1);
                const x1 = root._xAt(i);
                const y0 = root._yAt(root._months[i - 1].books);
                const y1 = root._yAt(root._months[i].books);
                const midX = (x0 + x1) / 2;
                ctx.bezierCurveTo(midX, y0, midX, y1, x1, y1);
            }
        };

        curvePath();
        ctx.lineTo(root._xAt(root._count - 1), root._plotBottom);
        ctx.lineTo(root._xAt(0), root._plotBottom);
        ctx.closePath();
        const fade = ctx.createLinearGradient(0, root._plotTop, 0, root._plotBottom);
        fade.addColorStop(0, root.areaColor);
        fade.addColorStop(1, Qt.rgba(root.areaColor.r, root.areaColor.g, root.areaColor.b, 0));
        ctx.fillStyle = fade;
        ctx.fill();

        curvePath();
        ctx.strokeStyle = root.lineColor;
        ctx.lineWidth = Geometry.chart.lineWidth;
        ctx.lineJoin = "round";
        ctx.lineCap = "round";
        ctx.stroke();
    }

    TextMetrics {
        id: valueMetrics
        font.pixelSize: Styles.fontSize.caption
        font.weight: Styles.fontWeight.semibold
        text: "00"
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.lg

        SectionHeader {
            id: header
            Layout.fillWidth: true
            titleSize: Styles.fontSize.title
        }

        Item {
            id: plotArea
            Layout.fillWidth: true
            Layout.preferredHeight: Geometry.chart.curvePlotHeight

            onWidthChanged: plot.requestPaint()
            onHeightChanged: plot.requestPaint()

            Canvas {
                id: plot
                anchors.fill: parent
                antialiasing: true
                visible: root._peak > 0
                onPaint: root._paint(plot.getContext("2d"))
            }

            // Text items rather than Canvas text, for the reason
            // BookProgressChart gives: the Canvas font has no digits.
            Repeater {
                id: valueLabels
                model: root._peak > 0 ? root._count : 0

                delegate: Text {
                    id: valueLabel

                    required property int index
                    readonly property int _books: root._months[valueLabel.index].books

                    x: root._xAt(valueLabel.index) - (width / 2)
                    y: root._yAt(valueLabel._books) - height - Geometry.spacing.xxs
                    // A month with nothing finished sits on the baseline, and a
                    // bare 0 there would read as an axis tick.
                    visible: valueLabel._books > 0
                    text: valueLabel._books
                    color: Theme.textSecondary
                    font.pixelSize: Styles.fontSize.caption
                    font.weight: Styles.fontWeight.semibold
                }
            }

            Text {
                id: emptyPlot
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: root._peak <= 0
                color: Theme.textMuted
                font.pixelSize: Styles.fontSize.body
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }

        RowLayout {
            id: monthLabels
            Layout.fillWidth: true
            spacing: 0

            Repeater {
                id: monthRepeater
                model: root._count

                delegate: Text {
                    id: monthLabel

                    required property int index

                    Layout.fillWidth: true
                    // Zero, so fillWidth splits the row evenly and each label
                    // stays centred under its point whatever the locale's
                    // month abbreviations weigh.
                    Layout.preferredWidth: 0
                    // QDate counts months from 1, Locale.standaloneMonthName from 0.
                    text: Qt.locale().standaloneMonthName(root._months[monthLabel.index].month - 1, Locale.ShortFormat)
                    color: Theme.textSecondary
                    font.pixelSize: Styles.fontSize.caption
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
            }
        }
    }
}
