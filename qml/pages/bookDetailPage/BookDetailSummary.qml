import QtQuick
import QtQuick.Layouts
import Library

ColumnLayout {
    id: root

    property alias title: header.title
    property alias author: header.author
    property alias year: header.year
    property alias publisher: header.publisher
    property alias totalPages: header.totalPages
    property alias pagesRead: progressCard.pagesRead
    property alias userRating: ratings.userRating
    property alias globalRating: ratings.globalRating
    property alias description: descriptionBody.text
    property alias coverSource: header.coverSource
    property int status: 0
    property alias inWishList: wantToBuyAction.active
    property alias hasMenu: header.hasMenu
    property int sidePadding: 0

    signal backRequested
    signal menuRequested
    signal statsRequested
    signal wantToReadRequested
    signal wantToBuyRequested
    signal startReadingRequested

    function beginReading() {
        progressCard.beginReading();
    }

    function stopReading() {
        progressCard.stopReading();
    }

    readonly property string _actionLabel: {
        switch (root.status) {
        case BookStatus.Finished:
            return qsTr("Read again");
        case BookStatus.InProgress:
            return qsTr("Continue reading");
        default:
            return qsTr("Start reading");
        }
    }

    spacing: Geometry.spacing.lg

    BookDetailHeader {
        id: header
        Layout.fillWidth: true
        Layout.leftMargin: root.sidePadding
        Layout.rightMargin: root.sidePadding
        Layout.topMargin: root.sidePadding
        rating: root.userRating
        onBackClicked: root.backRequested()
        onMenuClicked: root.menuRequested()
    }

    ReadingProgressCard {
        id: progressCard
        Layout.fillWidth: true
        Layout.leftMargin: root.sidePadding
        Layout.rightMargin: root.sidePadding
        pagesTotal: root.totalPages
        actionLabel: root._actionLabel
        onStartReadingRequested: root.startReadingRequested()
    }

    RatingsCard {
        id: ratings
        Layout.fillWidth: true
        Layout.leftMargin: root.sidePadding
        Layout.rightMargin: root.sidePadding
    }

    ColumnLayout {
        id: descriptionBlock
        Layout.fillWidth: true
        Layout.leftMargin: root.sidePadding
        Layout.rightMargin: root.sidePadding
        visible: root.description.length > 0
        spacing: Geometry.spacing.sm

        Text {
            id: descriptionTitle
            Layout.fillWidth: true
            text: qsTr("Description")
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.titleMedium
            font.weight: Styles.fontWeight.bold
        }

        Text {
            id: descriptionBody
            Layout.fillWidth: true
            color: Theme.textSecondary
            font.pixelSize: Styles.fontSize.body
            wrapMode: Text.WordWrap
            lineHeight: 1.4
            lineHeightMode: Text.ProportionalHeight
        }
    }

    GridLayout {
        id: actionsGrid
        Layout.fillWidth: true
        Layout.leftMargin: root.sidePadding
        Layout.rightMargin: root.sidePadding
        columns: 2
        columnSpacing: Geometry.spacing.md
        rowSpacing: Geometry.spacing.md

        ActionButton {
            id: wantToReadAction
            Layout.fillWidth: true
            label: qsTr("Want to read")
            iconGlyph: "📖"
            active: root.status === BookStatus.WantToRead
            onClicked: root.wantToReadRequested()
        }

        ActionButton {
            id: wantToBuyAction
            Layout.fillWidth: true
            label: qsTr("Want to buy")
            iconGlyph: "🛒"
            onClicked: root.wantToBuyRequested()
        }

        ActionButton {
            id: statsAction
            Layout.fillWidth: true
            Layout.columnSpan: 2
            label: qsTr("Statistics")
            iconGlyph: "📊"
            onClicked: root.statsRequested()
        }
    }
}
