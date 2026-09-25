pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

PaddedCard {
    id: root

    property alias title: header.title
    property alias emptyText: emptyPlot.text
    property var values: []
    property var labels: []

    readonly property color lineColor: Theme.primary
    readonly property color areaColor: Theme.primarySoft

    readonly property var _values: root.values ?? []
    readonly property int _count: root._values.length
    readonly property int _peak: {
        let peak = 0;
        for (let i = 0; i < root._count; ++i)
            peak = Math.max(peak, root._values[i]);
        return peak;
    }

    readonly property real _plotTop: captions.dense ? Geometry.chart.lineWidth : valueMetrics.height + Geometry.spacing.xxs
    readonly property real _plotBottom: plotArea.height - Geometry.chart.lineWidth
    readonly property real _plotHeight: Math.max(0, root._plotBottom - root._plotTop)

    function _xAt(index: int): real {
        return plotArea.width * (index + 0.5) / Math.max(1, root._count);
    }

    function _yAt(books: int): real {
        return root._plotBottom - (root._plotHeight * books / Math.max(1, root._peak));
    }

    onValuesChanged: plot.requestPaint()
    onLineColorChanged: plot.requestPaint()
    onAreaColorChanged: plot.requestPaint()

    function _paint(ctx): void {
        ctx.reset();
        if (root._count === 0 || root._peak <= 0)
            return;

        if (root._count === 1) {
            ctx.fillStyle = root.lineColor;
            ctx.beginPath();
            ctx.arc(root._xAt(0), root._yAt(root._values[0]), Geometry.chart.pointRadius, 0, Math.PI * 2);
            ctx.fill();
            return;
        }

        const curvePath = () => {
            ctx.beginPath();
            ctx.moveTo(root._xAt(0), root._yAt(root._values[0]));
            for (let i = 1; i < root._count; ++i) {
                const x0 = root._xAt(i - 1);
                const x1 = root._xAt(i);
                const y0 = root._yAt(root._values[i - 1]);
                const y1 = root._yAt(root._values[i]);
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

            Repeater {
                id: valueLabels
                model: root._peak > 0 && !captions.dense ? root._count : 0

                delegate: Text {
                    id: valueLabel

                    required property int index
                    readonly property int _books: root._values[valueLabel.index]

                    x: root._xAt(valueLabel.index) - (width / 2)
                    y: root._yAt(valueLabel._books) - height - Geometry.spacing.xxs
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

        ChartCaptions {
            id: captions
            Layout.fillWidth: true
            spacing: 0
            count: root._count
            labels: root.labels
        }
    }
}
