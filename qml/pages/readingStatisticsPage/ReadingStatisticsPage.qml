pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Page {
    id: root

    readonly property int _sidePadding: Geometry.spacing.xxl
    readonly property var _stats: ReadingStatisticsController.statistics

    readonly property var _buckets: root._stats.buckets
    readonly property var _captions: root._bucketCaptions()

    Component.onCompleted: ReadingStatisticsController.refresh()

    function _rangeText(): string {
        if (ReadingStatisticsController.period === ReadingStatisticsController.AllTime || root._stats.rangeStart.length === 0)
            return "";
        return Format.dateRange(root._stats.rangeStart, root._stats.rangeEnd);
    }

    function _bucketCaptions(): var {
        const buckets = root._buckets ?? [];
        const count = buckets.length;
        const spansYears = count > 0 && buckets[0].year !== buckets[count - 1].year;
        const monthFormat = count > 6 ? Locale.NarrowFormat : Locale.ShortFormat;
        return buckets.map(bucket => {
            switch (root._stats.granularity) {
            case ReadingStatisticsController.ByHour:
                return String(bucket.hour);
            case ReadingStatisticsController.ByDay:
                if (count <= 7)
                    return Qt.locale().dayName(new Date(bucket.year, bucket.month - 1, bucket.day).getDay(), Locale.ShortFormat);
                return String(bucket.day);
            case ReadingStatisticsController.ByMonth:
                if (spansYears && bucket.month === 1)
                    return String(bucket.year);
                return Qt.locale().standaloneMonthName(bucket.month - 1, monthFormat);
            default:
                return String(bucket.year);
            }
        });
    }

    function _openCustomRange(): void {
        const preselect = ReadingStatisticsController.period !== ReadingStatisticsController.AllTime;
        rangeSheet.startIso = preselect ? root._stats.rangeStart : "";
        rangeSheet.endIso = preselect ? root._stats.rangeEnd : "";
        rangeSheet.open();
    }

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
                text: qsTr("Reading statistics")
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

            ColumnLayout {
                id: content
                x: root._sidePadding
                width: scroll.width - (2 * root._sidePadding)
                spacing: Geometry.spacing.lg

                PeriodSelector {
                    id: periodSelector
                    Layout.fillWidth: true
                    visible: ReadingStatisticsController.hasData
                    period: ReadingStatisticsController.period
                    rangeText: root._rangeText()
                    onPeriodPicked: picked => ReadingStatisticsController.period = picked
                    onCustomRequested: root._openCustomRange()
                }

                RowLayout {
                    id: tiles
                    Layout.fillWidth: true
                    spacing: Geometry.spacing.md

                    StatTile {
                        id: booksTile
                        Layout.fillWidth: true
                        iconGlyph: "📈"
                        value: root._stats.booksFinished.toString()
                        label: qsTr("Books")
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

                PaddedCard {
                    id: emptyState
                    Layout.fillWidth: true
                    Layout.bottomMargin: Geometry.spacing.xxl
                    visible: !ReadingStatisticsController.hasData

                    ColumnLayout {
                        id: emptyStateLayout
                        anchors.fill: parent

                        Text {
                            id: emptyStateText
                            Layout.fillWidth: true
                            text: qsTr("Nothing read yet. Start a book and time your reading sessions — your statistics will appear here.")
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
                    visible: ReadingStatisticsController.hasData

                    PagesBarChart {
                        id: pagesChart
                        Layout.fillWidth: true
                        title: qsTr("Pages read")
                        emptyText: qsTr("Nothing read in this period")
                        values: root._buckets.map(bucket => bucket.pages)
                        labels: root._captions
                    }

                    BooksCurveChart {
                        id: booksChart
                        Layout.fillWidth: true
                        visible: root._stats.granularity !== ReadingStatisticsController.ByHour
                        title: qsTr("Books finished")
                        emptyText: qsTr("No books finished in this period")
                        values: root._buckets.map(bucket => bucket.books)
                        labels: root._captions
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

                    BooksReadCard {
                        id: booksReadCard
                        Layout.fillWidth: true
                        title: qsTr("Progress by book")
                        emptyText: qsTr("No books read in this period")
                        books: root._stats.booksRead
                        onBookClicked: isbn => BookController.openBook(isbn)
                    }
                }
            }
        }
    }

    DateRangeSheet {
        id: rangeSheet
        title: qsTr("Custom period")
        onApplied: (fromIso, toIso) => ReadingStatisticsController.setCustomRange(fromIso, toIso)
    }
}
