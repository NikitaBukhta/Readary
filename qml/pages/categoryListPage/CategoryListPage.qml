pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Library

Page {
    id: root

    property string currentNavId: "library"

    signal bookOpened(real isbn)

    function _title() {
        switch (BookController.activeKind) {
        case BookController.WantToRead:
            return qsTr("Want to read");
        case BookController.WantToBuy:
            return qsTr("Want to buy");
        case BookController.AlreadyRead:
            return qsTr("Already read");
        }
        return "";
    }

    readonly property int _sidePadding: Geometry.spacing.xxl

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            id: header
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: root._sidePadding
            spacing: Geometry.spacing.md

            Item {
                id: backRow
                Layout.preferredWidth: Geometry.size.iconLg
                Layout.preferredHeight: Geometry.size.iconLg
                Layout.alignment: Qt.AlignVCenter

                IconGlyph {
                    id: backIcon
                    anchors.centerIn: parent
                    glyph: "←"
                    color: Theme.primary
                    size: Geometry.size.iconLg
                }

                MouseArea {
                    id: backArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: NavigationController.goBack()
                }
            }

            Text {
                id: titleText
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: root._title()
                color: Theme.textPrimary
                font.pixelSize: Styles.fontSize.titleMedium
                font.weight: Styles.fontWeight.bold
                elide: Text.ElideRight
            }
        }

        Text {
            id: subtitleText
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: Geometry.spacing.xs
            text: qsTr("%n book(s)", "", list.count)
            color: Theme.textMuted
            font.pixelSize: Styles.fontSize.body
        }

        AppSearchField {
            id: searchField
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: Geometry.spacing.lg
            placeholderText: qsTr("Search in this list...")
            text: BookController.searchModel.searchQuery
            onTextEdited: text => BookController.searchModel.searchQuery = text
            onAccepted: text => BookController.searchModel.searchQuery = text
        }

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

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: BookListRow {
                required property var model
                required property int index

                width: ListView.view.width
                name: model.name ?? ""
                author: model.author ?? ""
                type: model.type ?? ""
                year: model.year ?? 0
                coverSource: model.coverUrl ?? ""
                onClicked: BookController.openBook(model.isbn)
            }
        }
    }
}
