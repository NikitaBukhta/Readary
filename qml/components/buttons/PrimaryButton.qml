import QtQuick
import QtQuick.Layouts
import Library

PressableSurface {
    id: root

    property string label: ""
    property string iconGlyph: ""
    property url iconSource
    property color contentColor: Theme.primaryContent

    implicitHeight: Geometry.size.pillButtonHeight
    radius: Geometry.radius.pill
    restColor: Theme.primary
    pressedColor: Theme.primary // press feedback via content opacity, not bg tint
    shadowOffset: Styles.elevation.subtleOffset
    shadowBlur: Styles.elevation.subtleBlur

    RowLayout {
        id: content
        anchors.centerIn: parent
        spacing: Geometry.spacing.sm
        opacity: root.pressed ? Styles.opacity.pressed : 1.0

        IconGlyph {
            id: glyph
            glyph: root.iconGlyph
            source: root.iconSource
            color: root.contentColor
            size: Geometry.size.iconMd
        }

        Text {
            id: labelText
            text: root.label
            color: root.contentColor
            font.pixelSize: Styles.fontSize.bodyLarge
            font.weight: Styles.fontWeight.semibold
        }
    }
}
