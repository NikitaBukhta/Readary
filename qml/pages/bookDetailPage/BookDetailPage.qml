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

    Flickable {
        id: scroll
        anchors.fill: parent
        contentWidth: width
        contentHeight: content.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOff
        }

        ColumnLayout {
            id: content
            width: scroll.width
            spacing: 0 //Geometry.spacing.md

            BookDetailSummary {
                id: summary
                Layout.fillWidth: true
                sidePadding: root._sidePadding
                title: root._book.name
                author: root._book.author
                year: root._book.year
                publisher: root._book.publisher
                totalPages: root._book.totalPages
                pagesRead: root._book.pagesRead
                userRating: root._book.userRating
                globalRating: root._book.globalRating
                description: root._book.description
                coverSource: root._book.coverUrl
                status: root._book.status
                onBackRequested: NavigationController.goBack()
                onPdfRequested: console.log(root._logTag, "Open PDF")
                onStatsRequested: console.log(root._logTag, "Open statistics")
                onFavoriteRequested: console.log(root._logTag, "Toggle favorite")
            }

            ListView {
                id: charactersList
                Layout.fillWidth: true
                Layout.topMargin: Geometry.spacing.sm
                implicitHeight: contentHeight
                interactive: false
                spacing: Geometry.spacing.md
                model: BookController.charactersModel

                header: CharactersListHeader {
                    width: charactersList.width
                    sidePadding: root._sidePadding
                    onAddRequested: console.log(root._logTag, "Add character")
                }

                delegate: CharactersListDelegate {
                    id: characterDelegate

                    required property var model

                    width: charactersList.width

                    characterData: characterDelegate.model
                    sidePadding: root._sidePadding

                    onOpenRequested: characterId => console.log(root._logTag, "Open character id", characterId)
                }

                footer: CharactersToggle {
                    width: charactersList.width
                    charactersModel: BookController.charactersModel
                }
            }

            BookGenreTags {
                id: tagsFlow
                Layout.fillWidth: true
                Layout.leftMargin: root._sidePadding
                Layout.rightMargin: root._sidePadding
                Layout.topMargin: Geometry.spacing.lg
                Layout.bottomMargin: Geometry.spacing.xxl
                tags: root._book.genres
            }
        }
    }
}
