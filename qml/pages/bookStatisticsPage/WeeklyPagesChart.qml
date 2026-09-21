pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

PaddedCard {
    id: root

    property string title: ""
    // One bucket per weekday, Monday first — the order BookStatisticsController
    // hands over. The chart renders however many it is given.
    property var values: []

    readonly property var _bars: root.values ?? []

    readonly property int _peak: {
        let peak = 0;
        for (let i = 0; i < root._bars.length; ++i)
            peak = Math.max(peak, root._bars[i]);
        return peak;
    }

    // Every bar carries its page count above it, so the plot keeps a label's
    // worth of headroom: scaling against the full height would push the
    // tallest day's number off the top of the card. Measured rather than
    // guessed, because the app font is a user setting.
    readonly property real _barAvailableHeight: Math.max(0, plotArea.height - valueMetrics.height - Geometry.spacing.xxs)

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
            title: root.title
            titleSize: Styles.fontSize.title
        }

        Item {
            id: plotArea
            Layout.fillWidth: true
            Layout.preferredHeight: Geometry.chart.barPlotHeight

            RowLayout {
                id: bars
                anchors.fill: parent
                spacing: Geometry.spacing.sm
                visible: root._peak > 0

                Repeater {
                    id: barRepeater
                    model: root._bars

                    delegate: Item {
                        id: barSlot

                        required property var modelData

                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Rectangle {
                            id: bar
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: root._barHeight(barSlot.modelData)
                            // Clamped, or a one-page day against a big week
                            // renders as a squashed lens instead of a bar.
                            radius: Math.min(Geometry.radius.xs, height / 2)
                            color: Theme.primary
                        }

                        Text {
                            id: valueLabel
                            anchors.horizontalCenter: bar.horizontalCenter
                            anchors.bottom: bar.top
                            anchors.bottomMargin: Geometry.spacing.xxs
                            // A day with nothing read has no bar, so a bare 0
                            // would float on the axis with nothing under it.
                            visible: barSlot.modelData > 0
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
                anchors.centerIn: parent
                visible: root._peak <= 0
                text: qsTr("Nothing read this week")
                color: Theme.textMuted
                font.pixelSize: Styles.fontSize.body
            }
        }

        RowLayout {
            id: dayLabels
            Layout.fillWidth: true
            spacing: Geometry.spacing.sm

            Repeater {
                id: dayRepeater
                // The count, not the values: these labels depend only on the
                // locale, so they must not be rebuilt when the bars change.
                model: root._bars.length

                delegate: Text {
                    id: dayLabel

                    required property int index

                    Layout.fillWidth: true
                    // Zero, so fillWidth splits the row evenly instead of
                    // adding surplus on top of each label's own width — the
                    // labels have to stay in column with the bars above, and
                    // day abbreviations differ in length per locale.
                    Layout.preferredWidth: 0
                    // The buckets start on Monday; Locale.dayName counts from Sunday.
                    text: Qt.locale().dayName((dayLabel.index + 1) % root._bars.length, Locale.ShortFormat)
                    color: Theme.textSecondary
                    font.pixelSize: Styles.fontSize.caption
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
            }
        }
    }
}
