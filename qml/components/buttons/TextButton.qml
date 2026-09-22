import QtQuick
import Library

TouchTarget {
    id: root

    property alias label: labelText.text
    property alias labelColor: labelText.color
    property alias labelSize: labelText.font.pixelSize
    property alias labelWeight: labelText.font.weight

    padding: Geometry.spacing.md

    Text {
        id: labelText
        color: Theme.textMuted
        font.pixelSize: Styles.fontSize.bodyLarge
        font.weight: Styles.fontWeight.medium
    }
}
