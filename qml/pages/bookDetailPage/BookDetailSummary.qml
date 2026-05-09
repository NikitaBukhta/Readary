import QtQuick
import QtQuick.Layouts
import Library

ColumnLayout {
    id: root

    property string title: ""
    property string author: ""
    property int year: 0
    property string publisher: ""
    property int totalPages: 0
    property int pagesRead: 0
    property int userRating: 0
    property real globalRating: 0
    property string description: ""
    property url coverSource
    property int status: 0
    property int sidePadding: 0

    signal backRequested
    signal pdfRequested
    signal statsRequested
    signal favoriteRequested

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
        title: root.title
        author: root.author
        year: root.year
        publisher: root.publisher
        totalPages: root.totalPages
        rating: root.userRating
        coverSource: root.coverSource
        onBackClicked: root.backRequested()
    }

    ReadingProgressCard {
        id: progressCard
        Layout.fillWidth: true
        Layout.leftMargin: root.sidePadding
        Layout.rightMargin: root.sidePadding
        pagesRead: root.pagesRead
        pagesTotal: root.totalPages
        actionLabel: root._actionLabel
    }

    RatingsCard {
        id: ratings
        Layout.fillWidth: true
        Layout.leftMargin: root.sidePadding
        Layout.rightMargin: root.sidePadding
        userRating: root.userRating
        globalRating: root.globalRating
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
            text: root.description
            color: Theme.textSecondary
            font.pixelSize: Styles.fontSize.body
            wrapMode: Text.WordWrap
            lineHeight: 1.4
            lineHeightMode: Text.ProportionalHeight
        }
    }

    RowLayout {
        id: actionsRow
        Layout.fillWidth: true
        Layout.leftMargin: root.sidePadding
        Layout.rightMargin: root.sidePadding
        spacing: Geometry.spacing.md

        ActionButton {
            id: pdfAction
            Layout.fillWidth: true
            label: qsTr("PDF")
            iconGlyph: "📄"
            onClicked: root.pdfRequested()
        }
        ActionButton {
            id: statsAction
            Layout.fillWidth: true
            label: qsTr("Statistics")
            iconGlyph: "📊"
            onClicked: root.statsRequested()
        }
        ActionButton {
            id: favoriteAction
            Layout.fillWidth: true
            label: qsTr("Favorite")
            iconGlyph: "♡"
            onClicked: root.favoriteRequested()
        }
    }
}
