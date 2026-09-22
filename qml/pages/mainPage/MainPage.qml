import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Page {
    id: root

    property alias appTitle: welcome.appTitle
    property alias userName: welcome.userName

    property alias goalCurrent: goalCard.current
    property alias goalTotal: goalCard.total

    readonly property int _sidePadding: Geometry.spacing.xxl

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Flickable {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
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
                spacing: Geometry.spacing.xl

                WelcomeHeader {
                    id: welcome
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    Layout.topMargin: root._sidePadding
                    appTitle: "DariszBooks"
                }

                AppSearchField {
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    placeholderText: qsTr("Search books...")
                    text: BookController.searchModel.searchQuery
                    onTextEdited: text => BookController.searchModel.searchQuery = text
                    onAccepted: text => BookController.searchModel.searchQuery = text
                }

                GoalCard {
                    id: goalCard
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    label: qsTr("Monthly goal")
                    unit: qsTr("books")
                    current: 1
                    total: 5
                }

                CurrentlyReadingSection {
                    Layout.fillWidth: true
                    sidePadding: root._sidePadding
                    title: qsTr("Currently reading")
                    model: BookController.getSortFilterProxyForKind(BookController.InProgress)
                    onBookOpened: isbn => BookController.openBook(isbn)
                }

                CategoriesSection {
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    onCategoryOpened: categoryId => {
                        BookController.activeKind = categoryId;
                        NavigationController.currentPage = NavigationController.CategoryListPage;
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root._sidePadding
                }
            }
        }
    }
}
