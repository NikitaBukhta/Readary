import QtQuick
import QtQuick.Layouts
import Library

PressableSurface {
    id: root

    property alias label: labelText.text
    property alias iconGlyph: glyph.glyph
    property alias iconSource: glyph.source
    property alias contentColor: labelText.color

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
            color: root.contentColor
            size: Geometry.size.iconMd
        }

        Text {
            id: labelText
            color: Theme.primaryContent
            font.pixelSize: Styles.fontSize.bodyLarge
            font.weight: Styles.fontWeight.semibold
        }
    }
}
