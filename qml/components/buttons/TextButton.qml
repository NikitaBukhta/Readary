import QtQuick
import Library

TouchTarget {
    id: root

    property string label: ""
    property color labelColor: Theme.textMuted
    property int labelSize: Styles.fontSize.bodyLarge
    property int labelWeight: Styles.fontWeight.medium

    padding: Geometry.spacing.md

    Text {
        id: labelText
        text: root.label
        color: root.labelColor
        font.pixelSize: root.labelSize
        font.weight: root.labelWeight
    }
}
