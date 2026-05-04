pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Library

ColumnLayout {
    id: root

    property string title: ""
    property string addLabel: ""
    property var model: []

    signal addClicked
    signal characterClicked(int characterId)

    spacing: Geometry.spacing.md

    RowLayout {
        id: headerRow
        Layout.fillWidth: true
        spacing: Geometry.spacing.sm

        Text {
            id: titleText
            Layout.fillWidth: true
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.titleMedium
            font.weight: Styles.fontWeight.bold
        }

        TextButton {
            id: addButton
            visible: root.addLabel.length > 0
            label: root.addLabel
            labelColor: Theme.primary
            labelSize: Styles.fontSize.body
            labelWeight: Styles.fontWeight.semibold
            onClicked: root.addClicked()
        }
    }

    Repeater {
        id: list
        model: root.model

        delegate: CharacterRow {
            id: row
            required property var modelData
            required property int index

            Layout.fillWidth: true
            name: modelData.name ?? ""
            role: modelData.role ?? ""
            onClicked: root.characterClicked(modelData.id ?? 0)
        }
    }
}
