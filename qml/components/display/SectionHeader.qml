import QtQuick
import QtQuick.Layouts
import Library

Item {
    id: root

    property alias title: titleLabel.text
    property alias trailingText: trailingLabel.text
    property alias trailingColor: trailingLabel.color
    // Page sections use titleMedium; a header inside a card sits one step down.
    property alias titleSize: titleLabel.font.pixelSize

    implicitHeight: titleLabel.implicitHeight
    implicitWidth: layout.implicitWidth

    RowLayout {
        id: layout
        anchors.fill: parent
        spacing: Geometry.spacing.sm

        Text {
            id: titleLabel
            Layout.fillWidth: true
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.titleMedium
            font.weight: Styles.fontWeight.bold
            elide: Text.ElideRight
        }

        Text {
            id: trailingLabel
            visible: root.trailingText.length > 0
            color: Theme.primary
            font.pixelSize: Styles.fontSize.body
            font.weight: Styles.fontWeight.semibold
        }
    }
}
