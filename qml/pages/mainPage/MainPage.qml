import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Rectangle {
    id: root

    property string appTitle: "DariszBooks"
    property string userName: ""

    property int goalCurrent: 1
    property int goalTotal: 5

    property string currentNavId: "library"

    signal bookOpened(int bookIndex)
    signal navItemSelected(string navId)

    color: Theme.background

    function _activeListTitle() {
        switch (BookController.activeKind) {
        case BookController.WantToRead:  return qsTr("Want to read");
        case BookController.WantToBuy:   return qsTr("Want to buy");
        case BookController.AlreadyRead: return qsTr("Already read");
        }
        return "";
    }

    readonly property var _categories: [
        {
            categoryId: BookController.WantToRead,
            title: qsTr("Want to read"),
            subtitle: qsTr("%n book(s)", "", BookController.wantToReadModel.count),
            glyph: "📖",
            source: ""
        },
        {
            categoryId: BookController.WantToBuy,
            title: qsTr("Want to buy"),
            subtitle: qsTr("%n book(s)", "", BookController.wantToBuyModel.count),
            glyph: "🛒",
            source: ""
        },
        {
            categoryId: BookController.AlreadyRead,
            title: qsTr("Already read"),
            subtitle: qsTr("%n book(s)", "", BookController.alreadyReadModel.count),
            glyph: "✅",
            source: ""
        }
    ]

    readonly property var _navItems: [
        { id: "library",    label: qsTr("Library"),    glyph: "📚", source: "" },
        { id: "search",     label: qsTr("Search"),     glyph: "🔍", source: "" },
        { id: "goals",      label: qsTr("Goals"),      glyph: "🎯", source: "" },
        { id: "challenges", label: qsTr("Challenges"), glyph: "🏆", source: "" },
        { id: "profile",    label: qsTr("Profile"),    glyph: "👤", source: "" }
    ]

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

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

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
                    onTextEdited: BookController.searchModel.searchQuery = text
                    onAccepted:   BookController.searchModel.searchQuery = text
                }

                GoalCard {
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    label:   qsTr("Monthly goal")
                    unit:    qsTr("books")
                    current: root.goalCurrent
                    total:   root.goalTotal
                }

                CurrentlyReadingSection {
                    Layout.fillWidth: true
                    sidePadding: root._sidePadding
                    title: qsTr("Currently reading")
                    model: BookController.readInProgressModel
                    onBookOpened: (index) => root.bookOpened(index)
                }

                CategoriesSection {
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    model: root._categories
                    onCategoryOpened: (categoryId) => {
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

        BottomNavBar {
            Layout.fillWidth: true
            items: root._navItems
            currentId: root.currentNavId
            onItemSelected: (id) => {
                root.currentNavId = id
                root.navItemSelected(id)
            }
        }
    }
}
