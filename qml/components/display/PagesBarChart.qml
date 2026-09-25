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

    readonly property var _bars: root.values ?? []
    readonly property int _count: root._bars.length

    readonly property int _peak: {
        let peak = 0;
        for (let i = 0; i < root._count; ++i)
            peak = Math.max(peak, root._bars[i]);
        return peak;
    }

    readonly property real _barAvailableHeight: captions.dense ? plotArea.height : Math.max(0, plotArea.height - valueMetrics.height - Geometry.spacing.xxs)

    function _barHeight(pages: int): real {
        if (pages <= 0 || root._peak <= 0)
            return 0;
        return Math.max(Geometry.chart.barMinHeight, Math.round(root._barAvailableHeight * pages / root._peak));
    }

    TextMetrics {
        id: valueMetrics
        font.pixelSize: Styles.fontSize.caption
        font.weight: Styles.fontWeight.semibold
        text: "000"
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
            Layout.preferredHeight: Geometry.chart.barPlotHeight

            RowLayout {
                id: bars
                anchors.fill: parent
                spacing: captions.dense ? Geometry.spacing.xxs : Geometry.spacing.sm
                visible: root._peak > 0

                Repeater {
                    id: barRepeater
                    model: root._bars

                    delegate: Item {
                        id: barSlot

                        required property var modelData

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.preferredWidth: 0

                        Rectangle {
                            id: bar
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: root._barHeight(barSlot.modelData)
                            radius: Math.min(Geometry.radius.xs, height / 2, width / 2)
                            color: Theme.primary
                        }

                        Text {
                            id: valueLabel
                            anchors.horizontalCenter: bar.horizontalCenter
                            anchors.bottom: bar.top
                            anchors.bottomMargin: Geometry.spacing.xxs
                            visible: !captions.dense && barSlot.modelData > 0
                            text: barSlot.modelData
                            color: Theme.textSecondary
                            font.pixelSize: Styles.fontSize.caption
                            font.weight: Styles.fontWeight.semibold
                        }
                    }
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
            spacing: bars.spacing
            count: root._count
            labels: root.labels
        }
    }
}
