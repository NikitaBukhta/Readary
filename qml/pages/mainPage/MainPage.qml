import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Rectangle {
    id: root

    // App-level identity (shown in the header). Kept as property so callers can
    // swap branding without editing this file.
    property string appTitle: "DariszBooks"
    property string userName: ""

    // Currently-reading list. Expect items: { title, author, coverSource,
    // pagesRead, pagesTotal }. Falls back to an inline demo model.
    property var readingModel: _demoReadingModel

    // Category list. Expect items: { categoryId, title, subtitle, glyph, source }.
    property var categoryModel: _demoCategoryModel

    // Goal card data.
    property int goalCurrent: 1
    property int goalTotal: 5

    property string currentNavId: "library"

    // Navigation intents — wire from outside to NavigationController.
    signal searchRequested(string query)
    signal bookOpened(int bookIndex)
    signal categoryOpened(string categoryId)
    signal navItemSelected(string navId)

    color: Theme.background

    ListModel {
        id: _demoReadingModel
        ListElement {
            title: qsTr("Night Wanderer")
            author: qsTr("Elena Morozova")
            coverSource: ""
            pagesRead: 156
            pagesTotal: 384
        }
        ListElement {
            title: qsTr("Andromeda Constellation")
            author: qsTr("Igor Savchenko")
            coverSource: ""
            pagesRead: 89
            pagesTotal: 512
        }
    }

    ListModel {
        id: _demoCategoryModel
        ListElement {
            categoryId: "wantToRead"
            title: qsTr("Want to read")
            subtitle: qsTr("%n book(s)", "", 1)
            glyph: "📖"
            source: ""
        }
        ListElement {
            categoryId: "wantToBuy"
            title: qsTr("Want to buy")
            subtitle: qsTr("%n book(s)", "", 1)
            glyph: "🛒"
            source: ""
        }
        ListElement {
            categoryId: "finished"
            title: qsTr("Finished")
            subtitle: qsTr("%n book(s)", "", 1)
            glyph: "✅"
            source: ""
        }
    }

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
                    onTextEdited: root.searchRequested(text)
                    onAccepted:   root.searchRequested(text)
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
                    model: root.readingModel
                    onBookOpened: (index) => root.bookOpened(index)
                }

                CategoriesSection {
                    Layout.fillWidth: true
                    Layout.leftMargin: root._sidePadding
                    Layout.rightMargin: root._sidePadding
                    model: root.categoryModel
                    onCategoryOpened: (id) => root.categoryOpened(id)
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
