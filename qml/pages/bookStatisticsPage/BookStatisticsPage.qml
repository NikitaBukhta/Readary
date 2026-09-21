pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Page {
    id: root

    readonly property int _sidePadding: Geometry.spacing.xxl
    readonly property var _stats: BookStatisticsController.statistics

    // The page lives behind Main.qml's Loader, so it is rebuilt on every open
    // while the controller's figures are not. The weekly buckets are relative
    // to today, so a set computed last week would otherwise still be on
    // screen. The controller stays quiet when nothing actually changed.
    Component.onCompleted: BookStatisticsController.refresh()

    ColumnLayout {
        id: page
        anchors.fill: parent
        spacing: 0

        RowLayout {
            id: header
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: root._sidePadding
            spacing: Geometry.spacing.md

            TouchTarget {
                id: backTarget
                Layout.alignment: Qt.AlignVCenter
                onClicked: NavigationController.goBack()

                IconGlyph {
                    id: backIcon
                    glyph: "←"
                    color: Theme.primary
                    size: Geometry.size.iconLg
                }
            }

            Text {
                id: titleText
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Statistics")
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.titleMedium
                font.weight: Styles.fontWeight.bold
                elide: Text.ElideRight
            }
        }

        Flickable {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Geometry.spacing.lg
            contentWidth: width
            contentHeight: content.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                id: scrollBar
                policy: ScrollBar.AlwaysOff
            }

            // Inset once here rather than per card — every child spans the
            // same column.
            ColumnLayout {
                id: content
                x: root._sidePadding
                width: scroll.width - (2 * root._sidePadding)
                spacing: Geometry.spacing.lg

                RowLayout {
                    id: tiles
                    Layout.fillWidth: true
                    spacing: Geometry.spacing.md

                    StatTile {
                        id: pagesTile
                        Layout.fillWidth: true
                        iconGlyph: "📈"
                        value: root._stats.pagesRead.toString()
                        label: qsTr("Pages")
                    }

                    StatTile {
                        id: timeTile
                        Layout.fillWidth: true
                        iconGlyph: "🕐"
                        value: root._stats.totalSeconds > 0 ? Format.duration(root._stats.totalSeconds) : Format.blank
                        label: qsTr("Reading")
                        valueColor: Theme.primary
                    }

                    StatTile {
                        id: speedTile
                        Layout.fillWidth: true
                        iconGlyph: "⚡"
                        value: root._stats.timedSessionCount > 0 ? Math.round(root._stats.averagePagesPerHour).toString() : Format.blank
                        label: qsTr("Pages/h")
                    }
                }

                // One empty state for the whole page: three cards each saying
                // "no data" reads worse than one line that says it once.
                PaddedCard {
                    id: emptyState
                    Layout.fillWidth: true
                    Layout.bottomMargin: Geometry.spacing.xxl
                    visible: !BookStatisticsController.hasData

                    // Wrapped in a Layout rather than anchored directly:
                    // PaddedCard derives its implicit size from its first
                    // child, and a wrapping Text reports the whole unwrapped
                    // sentence as its implicit width.
                    ColumnLayout {
                        id: emptyStateLayout
                        anchors.fill: parent

                        Text {
                            id: emptyStateText
                            Layout.fillWidth: true
                            text: qsTr("No reading sessions yet. Start the timer on the book page and your statistics will appear here.")
                            color: Theme.textSecondary
                            font.pixelSize: Styles.fontSize.body
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }

                ColumnLayout {
                    id: charts
                    Layout.fillWidth: true
                    Layout.bottomMargin: Geometry.spacing.xxl
                    spacing: Geometry.spacing.lg
                    visible: BookStatisticsController.hasData

                    WeeklyPagesChart {
                        id: weeklyChart
                        Layout.fillWidth: true
                        title: qsTr("Pages this week")
                        values: root._stats.weeklyPages
                    }

                    ReadingSpeedCard {
                        id: speedCard
                        Layout.fillWidth: true
                        title: qsTr("Reading speed")
                        minimum: root._stats.minPagesPerHour
                        average: root._stats.averagePagesPerHour
                        maximum: root._stats.maxPagesPerHour
                        hasSpeed: root._stats.timedSessionCount > 0
                    }

                    BookProgressChart {
                        id: progressChart
                        Layout.fillWidth: true
                        title: qsTr("Progress through the book")
                        subtitle: qsTr("The page you had reached when each session ended")
                        valueAxisCaption: qsTr("Page")
                        stepAxisCaption: qsTr("Session")
                        pointTemplate: qsTr("Page %1")
                        points: root._stats.progressPoints
                    }
                }
            }
        }
    }
}
