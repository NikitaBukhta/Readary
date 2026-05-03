import QtQuick
import QtQuick.Layouts
import Library

PressableSurface {
    id: root

    property string label: ""
    property string iconGlyph: ""
    property url iconSource
    property color iconColor: Theme.primary

    implicitHeight: Geometry.size.actionButtonHeight
    shadowOffset: Styles.elevation.subtleOffset
    shadowBlur: Styles.elevation.subtleBlur

    RowLayout {
        id: content
        anchors.centerIn: parent
        spacing: Geometry.spacing.sm

        IconGlyph {
            id: glyph
            glyph: root.iconGlyph
            source: root.iconSource
            color: root.iconColor
            size: Geometry.size.iconMd
        }

        Text {
            id: labelText
            text: root.label
            color: Theme.textPrimary
            font.pixelSize: Styles.fontSize.body
            font.weight: Styles.fontWeight.semibold
        }
    }
}
