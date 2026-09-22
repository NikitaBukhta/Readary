import QtQuick
import QtQuick.Layouts
import Library

PressableSurface {
    id: root

    property alias label: labelText.text
    property alias iconGlyph: glyph.glyph
    property alias iconSource: glyph.source
    property alias iconColor: glyph.color
    property bool active: false

    implicitHeight: Geometry.size.actionButtonHeight
    shadowOffset: Styles.elevation.subtleOffset
    shadowBlur: Styles.elevation.subtleBlur

    restColor: active ? Theme.primarySoft : Theme.surface
    border.width: active ? 1 : 0
    border.color: Theme.primary

    RowLayout {
        id: content
        anchors.centerIn: parent
        spacing: Geometry.spacing.sm

        IconGlyph {
            id: glyph
            color: Theme.primary
            size: Geometry.size.iconMd
        }

        Text {
            id: labelText
            color: root.active ? Theme.primary : Theme.textPrimary
            font.pixelSize: Styles.fontSize.body
            font.weight: Styles.fontWeight.semibold
        }
    }
}
