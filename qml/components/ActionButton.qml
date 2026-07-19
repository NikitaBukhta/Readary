import QtQuick
import QtQuick.Layouts
import Library

PressableSurface {
    id: root

    property string label: ""
    property string iconGlyph: ""
    property url iconSource
    property color iconColor: Theme.primary
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
            glyph: root.iconGlyph
            source: root.iconSource
            color: root.iconColor
            size: Geometry.size.iconMd
        }

        Text {
            id: labelText
            text: root.label
            color: root.active ? Theme.primary : Theme.textPrimary
            font.pixelSize: Styles.fontSize.body
            font.weight: Styles.fontWeight.semibold
        }
    }
}
