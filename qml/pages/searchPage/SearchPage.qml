pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Page {
    id: root

    readonly property int _sidePadding: Geometry.spacing.xxl

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Text {
            id: titleText
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: root._sidePadding
            text: qsTr("Search books")
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.titleMedium
            font.weight: Styles.fontWeight.bold
            elide: Text.ElideRight
        }

        Text {
            id: subtitleText
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: Geometry.spacing.xs
            text: GlobalBookSearchController.searching ? qsTr("Searching...") : qsTr("%n result(s)", "", list.count)
            color: Theme.textMuted
            font.pixelSize: Styles.fontSize.body
        }

        RowLayout {
            id: searchRow
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: Geometry.spacing.lg
            spacing: Geometry.spacing.md

            AppSearchField {
                id: searchField
                Layout.fillWidth: true
                busy: GlobalBookSearchController.searching
                placeholderText: qsTr("Search by title, author or ISBN...")
                onTextEdited: text => GlobalBookSearchController.setPendingQuery(text)
                onAccepted: text => GlobalBookSearchController.search(text)
            }

            FilterButton {
                id: filterButton
                activeCount: BookFilterController.activeCount
                onClicked: {
                    BookFilterController.scope = BookFilterController.Search;
                    filterSheet.open();
                }
            }
        }

        Item {
            id: listArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: Geometry.spacing.lg
            Layout.bottomMargin: Geometry.spacing.lg

            ListView {
                id: list
                anchors.fill: parent
                clip: true
                spacing: Geometry.spacing.md
                model: GlobalBookSearchController.resultsModel
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                delegate: BookListRow {
                    required property var model

                    width: ListView.view.width
                    name: model.name ?? ""
                    author: model.author ?? ""
                    type: model.type ?? ""
                    year: model.year ?? 0
                    coverSource: model.coverUrl ?? ""
                    onClicked: GlobalBookSearchController.openBook(model.isbn)
                }

                footer: Item {
                    id: listFooter

                    readonly property bool _loadingMore: GlobalBookSearchController.searching && list.count > 0
                    readonly property bool _offersMore: GlobalBookSearchController.canLoadMore && !GlobalBookSearchController.searching

                    width: list.width
                    height: listFooter._loadingMore || listFooter._offersMore ? loadMoreButton.implicitHeight + Geometry.spacing.md : 0

                    LoadingSpinner {
                        id: footerSpinner
                        anchors.centerIn: parent
                        running: listFooter._loadingMore
                        diameter: Geometry.size.iconLg
                    }

                    TextButton {
                        id: loadMoreButton
                        anchors.centerIn: parent
                        visible: listFooter._offersMore
                        label: qsTr("Load more")
                        labelColor: Theme.primary
                        labelSize: Styles.fontSize.body
                        labelWeight: Styles.fontWeight.semibold
                        onClicked: GlobalBookSearchController.loadMore()
                    }
                }
            }

            LoadingSpinner {
                id: initialSpinner
                anchors.centerIn: parent
                running: GlobalBookSearchController.searching && list.count === 0
                diameter: Geometry.size.goalRing
            }
        }
    }

    FilterSheet {
        id: filterSheet
        showLibraryOnlyCriteria: false
    }
}
