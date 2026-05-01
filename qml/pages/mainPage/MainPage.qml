import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Page {
    id: root

    property string appTitle: "DariszBooks"
    property string userName: ""

    property int goalCurrent: 1
    property int goalTotal: 5

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
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    Layout.topMargin: root._sidePadding
                    appTitle: root.appTitle
                    userName: root.userName
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
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    label: qsTr("Monthly goal")
                    unit: qsTr("books")
                    current: root.goalCurrent
                    total: root.goalTotal
                }

                CurrentlyReadingSection {
                    Layout.fillWidth: true
                    sidePadding: root._sidePadding
                    title: qsTr("Currently reading")
                    model: BookController.getSortFilterProxyForKind(BookController.InProgress)
                    onBookOpened: index => console.log("MainPage.qml: ", "Open book at index", index) // TODO: open book details page
                }

                CategoriesSection {
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    onCategoryOpened: categoryId => {
                        BookController.activeKind = categoryId;
                        NavigationController.currentPage = NavigationController.CATEGORY_LIST_PAGE;
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
