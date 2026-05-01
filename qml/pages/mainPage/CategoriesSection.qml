pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

ColumnLayout {
    id: root

    readonly property var _categories: [
        {
            categoryId: BookController.WantToRead,
            title: qsTr("Want to read"),
            subtitle: qsTr("%n book(s)", "", BookController.getSortFilterProxyForKind(BookController.WantToRead).count),
            glyph: "📖",
            source: ""
        },
        {
            categoryId: BookController.WantToBuy,
            title: qsTr("Want to buy"),
            subtitle: qsTr("%n book(s)", "", BookController.getSortFilterProxyForKind(BookController.WantToBuy).count),
            glyph: "🛒",
            source: ""
        },
        {
            categoryId: BookController.AlreadyRead,
            title: qsTr("Already read"),
            subtitle: qsTr("%n book(s)", "", BookController.getSortFilterProxyForKind(BookController.AlreadyRead).count),
            glyph: "✅",
            source: ""
        }
    ]

    signal categoryOpened(var categoryId)

    spacing: Geometry.spacing.md

    SectionHeader {
        Layout.fillWidth: true
        title: qsTr("Categories")
    }

    Repeater {
        model: root._categories

        delegate: CategoryRow {
            required property var model
            Layout.fillWidth: true
            title: model.title
            subtitle: model.subtitle
            iconGlyph: model.glyph
            iconSource: model.source
            onClicked: root.categoryOpened(model.categoryId)
        }
    }
}
