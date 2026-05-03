pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Page {
    id: root

    readonly property string _logTag: "BookDetailPage.qml: "
    readonly property int _sidePadding: Geometry.spacing.xxl

    readonly property var _book: BookController.currentBookData

    readonly property string _title: root._book.name ?? ""
    readonly property string _author: root._book.author ?? ""
    readonly property int _year: root._book.year ?? 0
    readonly property string _publisher: root._book.publisher ?? ""
    readonly property int _totalPages: root._book.totalPages ?? 0
    readonly property int _pagesRead: root._book.pagesRead ?? 0
    readonly property int _userRating: root._book.userRating ?? 0
    readonly property real _globalRating: root._book.globalRating ?? 0
    readonly property string _description: root._book.description ?? ""
    readonly property url _cover: root._book.coverUrl ?? ""
    readonly property int _status: root._book.status ?? 0
    readonly property var _characters: root._book.characters ?? []
    readonly property var _tags: root._book.genres ?? []

    readonly property string _actionLabel: {
        switch (root._status) {
        case BookStatus.Finished:
            return qsTr("Read again");
        case BookStatus.InProgress:
            return qsTr("Continue reading");
        default:
            return qsTr("Start reading");
        }
    }

    Flickable {
        id: scroll
        anchors.fill: parent
        contentWidth: width
        contentHeight: content.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            id: scrollBar
            policy: ScrollBar.AsNeeded
        }

        ColumnLayout {
            id: content
            width: scroll.width
            spacing: Geometry.spacing.lg

            BookDetailHeader {
                id: header
                Layout.fillWidth: true
                Layout.leftMargin: root._sidePadding
                Layout.rightMargin: root._sidePadding
                Layout.topMargin: root._sidePadding
                title: root._title
                author: root._author
                year: root._year
                publisher: root._publisher
                totalPages: root._totalPages
                rating: root._userRating
                coverSource: root._cover
                onBackClicked: NavigationController.goBack()
            }

            ReadingProgressCard {
                id: progressCard
                Layout.fillWidth: true
                Layout.leftMargin: root._sidePadding
                Layout.rightMargin: root._sidePadding
                pagesRead: root._pagesRead
                pagesTotal: root._totalPages
                actionLabel: root._actionLabel
                onActionClicked: console.log(root._logTag, "Primary action:", root._actionLabel)
            }

            RatingsCard {
                id: ratings
                Layout.fillWidth: true
                Layout.leftMargin: root._sidePadding
                Layout.rightMargin: root._sidePadding
                userRating: root._userRating
                globalRating: root._globalRating
            }

            ColumnLayout {
                id: descriptionBlock
                Layout.fillWidth: true
                Layout.leftMargin: root._sidePadding
                Layout.rightMargin: root._sidePadding
                visible: root._description.length > 0
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
                    text: root._description
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
                Layout.leftMargin: root._sidePadding
                Layout.rightMargin: root._sidePadding
                spacing: Geometry.spacing.md

                ActionButton {
                    id: pdfAction
                    Layout.fillWidth: true
                    label: qsTr("PDF")
                    iconGlyph: "📄"
                    onClicked: console.log(root._logTag, "Open PDF")
                }
                ActionButton {
                    id: statsAction
                    Layout.fillWidth: true
                    label: qsTr("Statistics")
                    iconGlyph: "📊"
                    onClicked: console.log(root._logTag, "Open statistics")
                }
                ActionButton {
                    id: favoriteAction
                    Layout.fillWidth: true
                    label: qsTr("Favorite")
                    iconGlyph: "♡"
                    onClicked: console.log(root._logTag, "Toggle favorite")
                }
            }

            CharactersSection {
                id: charactersSection
                Layout.fillWidth: true
                Layout.leftMargin: root._sidePadding
                Layout.rightMargin: root._sidePadding
                title: qsTr("Characters")
                addLabel: qsTr("+ Add")
                model: root._characters
                onAddClicked: console.log(root._logTag, "Add character")
                onCharacterClicked: characterId => console.log(root._logTag, "Open character id", characterId)
            }

            Flow {
                id: tagsFlow
                Layout.fillWidth: true
                Layout.leftMargin: root._sidePadding
                Layout.rightMargin: root._sidePadding
                Layout.bottomMargin: Geometry.spacing.xxl
                visible: root._tags.length > 0
                spacing: Geometry.spacing.sm

                Repeater {
                    id: tagsRepeater
                    model: root._tags

                    delegate: TagPill {
                        id: tag
                        required property string modelData
                        label: modelData
                    }
                }
            }
        }
    }
}
