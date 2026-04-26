pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

ColumnLayout {
    id: root

    property var model

    signal categoryOpened(string categoryId)

    spacing: Geometry.spacing.md

    SectionHeader {
        Layout.fillWidth: true
        title: qsTr("Categories")
    }

    Repeater {
        model: root.model

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
