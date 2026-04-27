pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Rectangle {
    id: root

    property string currentNavId: "library"

    signal bookOpened(int bookId)
    signal navItemSelected(string navId)

    color: Theme.background

    function _title() {
        switch (BookController.activeKind) {
        case BookController.WantToRead:  return qsTr("Want to read");
        case BookController.WantToBuy:   return qsTr("Want to buy");
        case BookController.AlreadyRead: return qsTr("Already read");
        }
        return "";
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

        // Header: back button + title
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: root._sidePadding
            spacing: Geometry.spacing.md

            Item {
                Layout.preferredWidth: Geometry.size.iconLg
                Layout.preferredHeight: Geometry.size.iconLg
                Layout.alignment: Qt.AlignVCenter

                IconGlyph {
                    anchors.centerIn: parent
                    glyph: "←"
                    color: Theme.primary
                    size: Geometry.size.iconLg
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: NavigationController.goBack()
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: root._title()
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.titleMedium
                font.weight: Styles.fontWeight.bold
                elide: Text.ElideRight
            }
        }

        // Subtitle: book count
        Text {
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: Geometry.spacing.xs
            text: qsTr("%n book(s)", "", BookController.searchModel.count)
            color: Theme.textMuted
            font.pixelSize: Styles.fontSize.body
        }

        // Search field
        AppSearchField {
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: Geometry.spacing.lg
            placeholderText: qsTr("Search in this list...")
            text: BookController.searchModel.searchQuery
            onTextEdited: BookController.searchModel.searchQuery = text
            onAccepted:   BookController.searchModel.searchQuery = text
        }

        // List
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: Geometry.spacing.lg
            Layout.bottomMargin: Geometry.spacing.lg
            clip: true
            spacing: Geometry.spacing.md
            model: BookController.searchModel
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: BookListRow {
                required property var model
                width: ListView.view.width
                name: model.name ?? ""
                author: model.author ?? ""
                type: model.type ?? ""
                year: model.year ?? 0
                onClicked: root.bookOpened(model.bookId)
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
