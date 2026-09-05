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

    property string _pendingSessionId: ""

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
                inWishList: root._book.inWishList
                onBackRequested: NavigationController.goBack()
                onPdfRequested: console.log(root._logTag, "Open PDF")
                onStatsRequested: console.log(root._logTag, "Open statistics")
                onWantToReadRequested: {
                    if (root._book.status === BookStatus.InProgress)
                        moveWarningDialog.open();
                    else
                        BookController.toggleWantToRead();
                }
                onWantToBuyRequested: BookController.toggleWishList()
                onStartReadingRequested: {
                    if (root._book.status === BookStatus.WantToRead && BookController.hasCachedProgress()) {
                        restoreProgressDialog.open();
                    } else {
                        BookController.discardCachedProgress();
                        summary.beginReading();
                    }
                }
            }

            ReadingHistorySection {
                id: historyList
                Layout.fillWidth: true
                Layout.topMargin: Geometry.spacing.lg
                sidePadding: root._sidePadding
                onDeleteRequested: sessionId => {
                    root._pendingSessionId = sessionId;
                    deleteSessionDialog.open();
                }
            }

            ListView {
                id: charactersList
                Layout.fillWidth: true
                Layout.topMargin: Geometry.spacing.lg
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

                footer: PagedListToggle {
                    width: charactersList.width
                    pagedModel: BookController.charactersModel
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

    ConfirmDialog {
        id: moveWarningDialog
        title: qsTr("Move to Want to Read?")
        message: qsTr("You are currently reading this book. Moving it to Want to Read resets your reading progress. Your current progress is saved and can be restored the next time you start reading.")
        confirmLabel: qsTr("Move anyway")
        cancelLabel: qsTr("Keep reading")
        onConfirmed: {
            summary.stopReading();
            BookController.moveInProgressToWantToRead();
        }
    }

    ConfirmDialog {
        id: deleteSessionDialog
        title: qsTr("Delete this session?")
        message: qsTr("The entry disappears from your reading history. Your current page stays where it is.")
        confirmLabel: qsTr("Delete")
        onConfirmed: {
            BookController.deleteReadingSession(root._pendingSessionId);
            root._pendingSessionId = "";
        }
        onCancelled: root._pendingSessionId = ""
    }

    ConfirmDialog {
        id: restoreProgressDialog
        title: qsTr("Restore previous progress?")
        message: qsTr("It looks like you were reading this book before. Restore your saved progress, or start over from the beginning?")
        confirmLabel: qsTr("Restore")
        cancelLabel: qsTr("Start over")
        onConfirmed: {
            BookController.restoreCachedProgress();
            summary.beginReading();
        }
        onCancelled: {
            BookController.discardCachedProgress();
            summary.beginReading();
        }
    }
}
