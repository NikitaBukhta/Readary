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
            text: qsTr("%n result(s)", "", list.count)
            color: Theme.textMuted
            font.pixelSize: Styles.fontSize.body
        }

        AppSearchField {
            id: searchField
            Layout.fillWidth: true
            Layout.leftMargin: root._sidePadding
            Layout.rightMargin: root._sidePadding
            Layout.topMargin: Geometry.spacing.lg
            placeholderText: qsTr("Search by title, author or ISBN...")
            onAccepted: text => GlobalBookSearchController.search(text)
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
        }
    }
}
